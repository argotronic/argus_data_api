#ifndef ARGUS_MONITOR_DATA_API_ACCESSOR_H
#define ARGUS_MONITOR_DATA_API_ACCESSOR_H

#include "argus_monitor_data_api.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace argus_monitor {
namespace data_api {

    namespace detail {
        enum class PollResult { kNoChange, kNewData, kInvalidData, kLockTimeout, kLockAbandoned, kLockFailed };

        // One polling step (used by ArgusMonitorDataAccessor, public for tests). Waits up to lock_timeout_ms for `mutex`
        // (a Win32 mutex HANDLE). If shared.CycleCounter differs from last_cycle_counter, it copies `shared` into
        // `snapshot` and stores the counter; kNewData only if the copy passes IsValidSensorData(). An abandoned mutex (the
        // writer died while holding it) marks the current counter as seen without copying, because the data may be torn.
        [[nodiscard]] PollResult PollSnapshot(ArgusMonitorData const& shared, void* mutex, std::uint32_t lock_timeout_ms,
                                              std::uint32_t& last_cycle_counter, ArgusMonitorData& snapshot) noexcept;
    }

    // Reads the sensor data of a running Argus Monitor from its shared memory and passes every new, valid snapshot to a
    // callback. Open(), Close() and IsOpen() are meant to be called from one thread. The callback runs on an internal
    // polling thread; it must not throw and must not call Close() or destroy the accessor.
    class ArgusMonitorDataAccessor {
    public:
        using SensorDataCallback = std::function<void(argus_monitor::data_api::ArgusMonitorData const&)>;

        ArgusMonitorDataAccessor() = default;
        // other object names than the ones of Argus Monitor (used by tests)
        ArgusMonitorDataAccessor(wchar_t const* mapping_name, wchar_t const* mutex_name);

        ArgusMonitorDataAccessor(ArgusMonitorDataAccessor const&) = delete;
        ArgusMonitorDataAccessor(ArgusMonitorDataAccessor&&)      = delete;
        ArgusMonitorDataAccessor& operator=(ArgusMonitorDataAccessor const&) = delete;
        ArgusMonitorDataAccessor& operator=(ArgusMonitorDataAccessor&&) = delete;

        ~ArgusMonitorDataAccessor();

        // Opens the shared memory and the mutex and starts polling; the current snapshot is delivered right away.
        // Returns false while Argus Monitor does not provide them (try again later). Open() after Close() is fine.
        [[nodiscard]] bool Open();
        [[nodiscard]] bool IsOpen() const noexcept { return is_open_; }
        // Stops polling (waits for a running callback) and releases all handles; does nothing if not open.
        void Close();

        // Sets the function that gets each new, valid snapshot; can be called at any time, also before Open().
        void RegisterSensorCallbackOnDataChanged(SensorDataCallback callback);

    private:
        void PollingLoop();
        void NotifySensorDataChanged(argus_monitor::data_api::ArgusMonitorData const& sensor_data);
        void ReleaseHandles() noexcept;

        std::wstring const mapping_name_{ kMappingName() };
        std::wstring const mutex_name_{ kMutexName() };

        // Win32 HANDLEs and the mapped view (void*, so this header does not need <windows.h>)
        void* file_mapping_{ nullptr };
        void* mapped_view_{ nullptr };
        void* mutex_{ nullptr };
        void* stop_event_{ nullptr };
        bool  is_open_{ false };

        std::unique_ptr<argus_monitor::data_api::ArgusMonitorData> snapshot_{};    // ~105 KB, only used by the polling thread
        std::mutex                                                 callback_mutex_{};
        SensorDataCallback                                         new_sensor_data_callback_{};
        std::thread                                                polling_thread_{};
    };


}
}

#endif    // ARGUS_MONITOR_DATA_API_ACCESSOR_H
