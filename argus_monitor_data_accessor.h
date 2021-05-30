#ifndef ARGUS_MONITOR_DATA_API_ACCESSOR_H
#define ARGUS_MONITOR_DATA_API_ACCESSOR_H

#include "argus_monitor_data_api.h"
#include <mutex>
#include <thread>
#include <wchar.h>
#include <windows.h>

namespace argus_monitor {
namespace data_api {

    class ArgusMonitorDataAccessor {

    private:
        HANDLE                                                               handle_file_mapping{ nullptr };
        void*                                                                pointer_to_mapped_data{ nullptr };
        argus_monitor::data_api::ArgusMontorData const*                      sensor_data_{ nullptr };
        bool                                                                 is_open_{ false };
        bool                                                                 keep_polling_{ true };
        std::function<void(argus_monitor::data_api::ArgusMontorData const&)> new_sensor_data_callback_{};
        std::thread                                                          polling_thread{};


        void          StartThread();
        static void   Poll(ArgusMonitorDataAccessor* class_instance);
        static HANDLE OpenArgusApiMutex();
        void          ProcessSensorData(argus_monitor::data_api::ArgusMontorData const& sensor_data);

    public:
        ArgusMonitorDataAccessor() = default;

        ArgusMonitorDataAccessor(ArgusMonitorDataAccessor const&) = delete;
        ArgusMonitorDataAccessor(ArgusMonitorDataAccessor&&)      = delete;
        ArgusMonitorDataAccessor& operator=(ArgusMonitorDataAccessor const&) = delete;
        ArgusMonitorDataAccessor& operator=(ArgusMonitorDataAccessor&&) = delete;

        ~ArgusMonitorDataAccessor()
        {
            keep_polling_ = false;
            if (polling_thread.joinable()) {
                polling_thread.join();
            }
        }

        // todo: just for running test suite -- remove before release
        void TestFunction_FakeCycleCounterIncrement()
        {
            ++reinterpret_cast<argus_monitor::data_api::ArgusMontorData*>(pointer_to_mapped_data)->CycleCounter;
        }

        bool Open();
        bool IsOpen() const noexcept { return is_open_; }
        void Close();

        bool RegisterSensorCallbackOnDataChanged(std::function<void(argus_monitor::data_api::ArgusMontorData const&)> callback);
    };


}
}

#endif    // ARGUS_MONITOR_DATA_API_ACCESSOR_H
