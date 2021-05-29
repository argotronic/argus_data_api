#include "argus_monitor_data_accessor.h"


namespace argus_monitor {
namespace data_api {

    namespace {
        class Lock {
        public:
            explicit Lock(HANDLE mutex_handle)
                : mutex_handle_{ mutex_handle }
            {
                WaitForSingleObject(mutex_handle_, INFINITE);
            }
            ~Lock() { ReleaseMutex(mutex_handle_); }

        private:
            HANDLE mutex_handle_;
        };
    }

    bool ArgusMonitorDataAccessor::Open()
    {
        if (is_open_) {
            return true;
        }
        handle_file_mapping = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,              // read/write access
                                              FALSE,                                       // do not inherit the name
                                              argus_monitor::data_api::kMappingName());    // name of mapping object

        if (handle_file_mapping == nullptr) {
            return false;
        }

        pointer_to_mapped_data = MapViewOfFile(handle_file_mapping,               // handle to map object
                                               FILE_MAP_READ | FILE_MAP_WRITE,    // read/write permission
                                               0, 0, argus_monitor::data_api::kMappingSize());

        if (pointer_to_mapped_data == nullptr) {
            CloseHandle(handle_file_mapping);
            return false;
        }

        sensor_data_ = reinterpret_cast<argus_monitor::data_api::ArgusMontorData const*>(pointer_to_mapped_data);
        is_open_     = true;

        StartThread();

        return true;
    }

    void ArgusMonitorDataAccessor::Close()
    {
        is_open_      = false;
        keep_polling_ = false;
        if (polling_thread.joinable()) {
            polling_thread.join();
        }

        sensor_data_ = nullptr;
        if (pointer_to_mapped_data) {
            UnmapViewOfFile(pointer_to_mapped_data);
            pointer_to_mapped_data = nullptr;
        }
        if (handle_file_mapping) {
            CloseHandle(handle_file_mapping);
            handle_file_mapping = nullptr;
        }
    }
    void ArgusMonitorDataAccessor::StartThread() { polling_thread = std::thread{ Poll, this }; }

    bool ArgusMonitorDataAccessor::RegisterSensorCallbackOnDataChanged(std::function<void(void)> callback)
    {
        new_sensor_data_callback_ = callback;
        return true;
    }

    void ArgusMonitorDataAccessor::Poll(ArgusMonitorDataAccessor* class_instance)
    {
        if (nullptr == class_instance->pointer_to_mapped_data) {
            return;
        }

        HANDLE mutex_handle = ArgusMonitorDataAccessor::OpenArgusApiMutex();
        if (nullptr == mutex_handle) {
            return;
        }

        argus_monitor::data_api::ArgusMontorData sensor_data_copy{};
        auto        sensor_data = reinterpret_cast<argus_monitor::data_api::ArgusMontorData const*>(class_instance->pointer_to_mapped_data);

        auto last_cycle_counter{ sensor_data->CycleCounter };
        DWORD const polling_interval{ 100U };
        while (class_instance->keep_polling_) {
            bool new_data_available{ false };
            {
                Lock scoped_lock(mutex_handle);
                if (last_cycle_counter != sensor_data->CycleCounter) {
                    last_cycle_counter = sensor_data->CycleCounter;
                    memcpy(&sensor_data_copy, &sensor_data, sizeof(argus_monitor::data_api::ArgusMontorData));
                    new_data_available = true;
                }
            }

            // todo: process data (e.g. call Process of class)
            if (new_data_available) {
                class_instance->ProcessSensorData(sensor_data_copy);
            }
            Sleep(polling_interval);
        }
    }

    HANDLE ArgusMonitorDataAccessor::OpenArgusApiMutex()
    {
        return OpenMutex(READ_CONTROL | MUTANT_QUERY_STATE | SYNCHRONIZE, FALSE, argus_monitor::data_api::kMutexName());
    }

    void ArgusMonitorDataAccessor::ProcessSensorData(argus_monitor::data_api::ArgusMontorData const& sensor_data) { new_sensor_data_callback_(); }
}
}
