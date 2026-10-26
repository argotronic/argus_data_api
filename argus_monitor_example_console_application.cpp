#include "argus_monitor_data_accessor.h"
#include "argus_monitor_data_api.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <windows.h>

using namespace std::chrono_literals;

namespace {
    // The unit strings contain non-ASCII characters (degree sign): print UTF-8 and restore the console code page at the end.
    class ConsoleUtf8Output {
    public:
        ConsoleUtf8Output() noexcept
            : previous_code_page_{ GetConsoleOutputCP() }
        {
            SetConsoleOutputCP(CP_UTF8);
        }
        ~ConsoleUtf8Output()
        {
            if (previous_code_page_ != 0U) {
                SetConsoleOutputCP(previous_code_page_);
            }
        }
        ConsoleUtf8Output(ConsoleUtf8Output const&)            = delete;
        ConsoleUtf8Output& operator=(ConsoleUtf8Output const&) = delete;

    private:
        UINT const previous_code_page_;
    };

    // Label and UnitString are fixed-size arrays: stop at the first 0 or at the array size, whichever comes first.
    std::string FixedTextToUtf8(wchar_t const* const text, std::size_t const max_length)
    {
        int const length = static_cast<int>(std::find(text, text + max_length, L'\0') - text);
        int const size   = WideCharToMultiByte(CP_UTF8, 0, text, length, nullptr, 0, nullptr, nullptr);
        if (size <= 0) {
            return {};
        }
        std::string utf8(static_cast<std::size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text, length, utf8.data(), size, nullptr, nullptr);
        return utf8;
    }

    void PrintSensorData(argus_monitor::data_api::ArgusMonitorData const&  sensor_data,
                         argus_monitor::data_api::ARGUS_MONITOR_SENSOR_TYPE const sensor_type)
    {
        // the accessor only delivers snapshots that pass IsValidSensorData(), so this range is inside SensorData
        std::uint32_t const first = sensor_data.OffsetForSensorType[sensor_type];
        std::uint32_t const count = sensor_data.SensorCount[sensor_type];

        for (std::uint32_t index{ first }; index < first + count; ++index) {
            auto const& sensor = sensor_data.SensorData[index];
            std::cout << FixedTextToUtf8(sensor.Label, argus_monitor::data_api::kMaxLenLabel) << ": " << sensor.Value << ' '
                      << FixedTextToUtf8(sensor.UnitString, argus_monitor::data_api::kMaxLenUnit) << '\n';
        }
    }
}


int main()
{
    ConsoleUtf8Output const utf8_output{};
    std::future<void>       wait_for_key = std::async(std::launch::async, []() { std::cin.get(); });

    // the callback runs on the polling thread: keep its output and the one of main() apart
    std::mutex output_mutex{};

    auto const new_sensor_data_available = [&output_mutex](argus_monitor::data_api::ArgusMonitorData const& new_sensor_data) {
        std::scoped_lock const lock{ output_mutex };
        std::cout << "New Argus Monitor sensor data available:\n\n";
        std::cout << "Argus Monitor Version: " << std::to_string(new_sensor_data.ArgusMajor) << "." << std::to_string(new_sensor_data.ArgusMinorA)
                  << "." << std::to_string(new_sensor_data.ArgusMinorB) << "." << std::to_string(new_sensor_data.ArgusBuild) << '\n';
        std::cout << "Total number of sensor values:" << std::to_string(new_sensor_data.TotalSensorCount) << '\n';

        // Example: print all CPU temperature values
        PrintSensorData(new_sensor_data, argus_monitor::data_api::SENSOR_TYPE_CPU_TEMPERATURE);

        // and some extra values (e.g. CCDx temps for AMD processors)
        PrintSensorData(new_sensor_data, argus_monitor::data_api::SENSOR_TYPE_CPU_TEMPERATURE_ADDITIONAL);

        // Example2: print all SSD/HDD temperatures
        PrintSensorData(new_sensor_data, argus_monitor::data_api::SENSOR_TYPE_DISK_TEMPERATURE);

        // Example3: print all GPU temperatures
        PrintSensorData(new_sensor_data, argus_monitor::data_api::SENSOR_TYPE_GPU_TEMPERATURE);


        std::cout << "\n\nWaiting for new sensor data -- press RETURN or Ctrl-C to exit.\n";
    };


    // destroyed (and closed) before output_mutex
    argus_monitor::data_api::ArgusMonitorDataAccessor data_accessor{};
    data_accessor.RegisterSensorCallbackOnDataChanged(new_sensor_data_available);
    while (!data_accessor.Open()) {
        std::cout << "Waiting for Argus Monitor API connection to become available\n";
        if (wait_for_key.wait_for(1s) == std::future_status::ready) {
            return -1;
        }
    }

    {
        std::scoped_lock const lock{ output_mutex };
        std::cout << "Connection to Argus Monitor established\n";
        std::cout << "Waiting for sensor data (Argus Monitor has to be running) -- press RETURN or Ctrl-C to exit.\n";
    }

    wait_for_key.wait();
    data_accessor.Close();

    return 0;
}
