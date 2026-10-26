#include "argus_monitor_data_accessor.h"

#include <cstring>
#include <exception>
#include <utility>
#include <windows.h>


namespace argus_monitor {
namespace data_api {

    namespace {
        constexpr DWORD kPollingIntervalMs{ 100U };
        // Argus Monitor holds the mutex only for one copy of ArgusMonitorData; skip the cycle if it takes longer
        constexpr DWORD kLockTimeoutMs{ 100U };

        // owns the mutex only if the wait succeeded
        class ScopedMutexLock {
        public:
            ScopedMutexLock(HANDLE const mutex_handle, DWORD const timeout_ms) noexcept
                : mutex_handle_{ mutex_handle }
                , wait_result_{ WaitForSingleObject(mutex_handle, timeout_ms) }
            {
            }
            ~ScopedMutexLock()
            {
                if (wait_result_ == WAIT_OBJECT_0 || wait_result_ == WAIT_ABANDONED) {
                    ReleaseMutex(mutex_handle_);
                }
            }
            ScopedMutexLock(ScopedMutexLock const&)            = delete;
            ScopedMutexLock& operator=(ScopedMutexLock const&) = delete;

            [[nodiscard]] DWORD WaitResult() const noexcept { return wait_result_; }

        private:
            HANDLE const mutex_handle_;
            DWORD const  wait_result_;
        };

        void CloseAndReset(void*& handle) noexcept
        {
            if (handle != nullptr) {
                CloseHandle(handle);
                handle = nullptr;
            }
        }
    }

    namespace detail {
        PollResult PollSnapshot(ArgusMonitorData const& shared, void* const mutex, std::uint32_t const lock_timeout_ms,
                                std::uint32_t& last_cycle_counter, ArgusMonitorData& snapshot) noexcept
        {
            ScopedMutexLock const lock{ mutex, lock_timeout_ms };
            switch (lock.WaitResult()) {
            case WAIT_OBJECT_0:
                break;
            case WAIT_ABANDONED:
                last_cycle_counter = shared.CycleCounter;
                return PollResult::kLockAbandoned;
            case WAIT_TIMEOUT:
                return PollResult::kLockTimeout;
            default:
                return PollResult::kLockFailed;
            }

            if (shared.CycleCounter == last_cycle_counter) {
                return PollResult::kNoChange;
            }
            last_cycle_counter = shared.CycleCounter;
            std::memcpy(&snapshot, &shared, sizeof(ArgusMonitorData));
            return IsValidSensorData(snapshot) ? PollResult::kNewData : PollResult::kInvalidData;
        }
    }

    ArgusMonitorDataAccessor::ArgusMonitorDataAccessor(wchar_t const* const mapping_name, wchar_t const* const mutex_name)
        : mapping_name_{ mapping_name }
        , mutex_name_{ mutex_name }
    {
    }

    ArgusMonitorDataAccessor::~ArgusMonitorDataAccessor() { Close(); }

    bool ArgusMonitorDataAccessor::Open()
    {
        if (is_open_) {
            return true;
        }

        // read access is all a reader needs
        file_mapping_ = OpenFileMappingW(FILE_MAP_READ, FALSE, mapping_name_.c_str());
        if (file_mapping_ != nullptr) {
            // only ArgusMonitorData, not kMappingSize(): fails if the mapping is too small for this layout
            mapped_view_ = MapViewOfFile(file_mapping_, FILE_MAP_READ, 0, 0, sizeof(ArgusMonitorData));
        }
        // opened here and not in the polling thread, so Open() fails while the mutex does not exist yet
        mutex_      = OpenMutexW(READ_CONTROL | MUTANT_QUERY_STATE | SYNCHRONIZE, FALSE, mutex_name_.c_str());
        stop_event_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (mapped_view_ == nullptr || mutex_ == nullptr || stop_event_ == nullptr) {
            ReleaseHandles();
            return false;
        }

        try {
            if (!snapshot_) {
                snapshot_ = std::make_unique<ArgusMonitorData>();
            }
            polling_thread_ = std::thread{ [this] { PollingLoop(); } };
        } catch (std::exception const&) {
            ReleaseHandles();
            return false;
        }

        is_open_ = true;
        return true;
    }

    void ArgusMonitorDataAccessor::Close()
    {
        if (stop_event_ != nullptr) {
            SetEvent(stop_event_);
        }
        if (polling_thread_.joinable()) {
            polling_thread_.join();
        }
        ReleaseHandles();
        is_open_ = false;
    }

    void ArgusMonitorDataAccessor::ReleaseHandles() noexcept
    {
        if (mapped_view_ != nullptr) {
            UnmapViewOfFile(mapped_view_);
            mapped_view_ = nullptr;
        }
        CloseAndReset(file_mapping_);
        CloseAndReset(mutex_);
        CloseAndReset(stop_event_);
    }

    void ArgusMonitorDataAccessor::RegisterSensorCallbackOnDataChanged(SensorDataCallback callback)
    {
        std::scoped_lock const lock{ callback_mutex_ };
        new_sensor_data_callback_ = std::move(callback);
    }

    void ArgusMonitorDataAccessor::PollingLoop()
    {
        auto const& shared = *static_cast<ArgusMonitorData const*>(mapped_view_);
        // 0 is never a valid CycleCounter, so the current snapshot is delivered right away
        std::uint32_t last_cycle_counter{ 0U };
        // polls once before looking at the stop event
        do {
            if (detail::PollSnapshot(shared, mutex_, kLockTimeoutMs, last_cycle_counter, *snapshot_) == detail::PollResult::kNewData) {
                NotifySensorDataChanged(*snapshot_);
            }
        } while (WaitForSingleObject(stop_event_, kPollingIntervalMs) == WAIT_TIMEOUT);
    }

    void ArgusMonitorDataAccessor::NotifySensorDataChanged(ArgusMonitorData const& sensor_data)
    {
        SensorDataCallback callback{};
        {
            std::scoped_lock const lock{ callback_mutex_ };
            callback = new_sensor_data_callback_;
        }
        // called without the lock, so the callback may register another callback
        if (callback) {
            callback(sensor_data);
        }
    }
}
}
