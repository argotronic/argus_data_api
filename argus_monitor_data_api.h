#ifndef ARGUS_MONITOR_DATA_MEMORY_API_H
#define ARGUS_MONITOR_DATA_MEMORY_API_H

#include <cstdint>

namespace argus_monitor {
namespace data_api {

    //    static constexpr wchar_t const* kMappingName{L"Global\\ARGUSMONITOR_DATA_INTERFACE"};
    //    static constexpr wchar_t const* kMutexName{L"Global\\ARGUSMONITOR_DATA_INTERFACE_MUTEX"};
    //    static constexpr std::size_t    kMappingSize{1024U * 1024U};
    inline wchar_t const* kMappingName() { return L"Global\\ARGUSMONITOR_DATA_INTERFACE"; }
    inline wchar_t const* kMutexName() { return L"Global\\ARGUSMONITOR_DATA_INTERFACE_MUTEX"; }
    inline std::size_t    kMappingSize() { return 1024U * 1024U; }

    enum ARGUS_MONITOR_SENSOR_TYPE {
        SENSOR_TYPE_INVALID = 0,
        SENSOR_TYPE_TEMPERATURE,              // temperatures of mainboard sensors, external fan controllers and AIOs
        SENSOR_TYPE_SYNTHETIC_TEMPERATURE,    // user defined synthetic temperatures (mean, max, average, difference, ...)
        SENSOR_TYPE_FAN_SPEED_RPM,    // fan speed of fans attached to mainboard channels, AIOs, external fan controllers and also pump speeds of AIOs
        SENSOR_TYPE_FAN_CONTROL_VALUE,             // if any fan/pump is controlled by Argus Monitor, then the control value can be read from this
        SENSOR_TYPE_NETWORK_SPEED,                 // up/down speeds of network adapters if selected to be monitored inside Argus Monitor
        SENSOR_TYPE_CPU_TEMPERATURE,               // the normal CPU temperature readings, per core for Intel and the only one available for AMD
        SENSOR_TYPE_CPU_TEMPERATURE_ADDITIONAL,    // additional temperatures provided by the CPU, like CCDx temperatures of AMD CPUs
        SENSOR_TYPE_CPU_MULTIPLIER_MIN,
        SENSOR_TYPE_CPU_MULTIPLIER_MAX,
        SENSOR_TYPE_CPU_FREQUENCY_MIN,
        SENSOR_TYPE_CPU_FREQUENCY_MAX,
        SENSOR_TYPE_GPU_TEMPERATURE,
        SENSOR_TYPE_GPU_LOAD,
        SENSOR_TYPE_GPU_CORECLK,
        SENSOR_TYPE_GPU_MEMORYCLK,
        SENSOR_TYPE_GPU_FAN_SPEED_PERCENT,
        SENSOR_TYPE_GPU_FAN_SPEED_RPM,
        SENSOR_TYPE_GPU_MEMORY_USED_PERC,
        SENSOR_TYPE_GPU_MEMORY_USED_MB,
        SENSOR_TYPE_GPU_POWER,
        SENSOR_TYPE_DISK_TEMPERATURE,
        SENSOR_TYPE_DISK_TRANSFER_RATE,
        SENSOR_TYPE_CPU_LOAD_ALL,
        SENSOR_TYPE_CPU_LOAD_PROGRAMS,
        SENSOR_TYPE_CPU_LOAD_SYSTEM,
        SENSOR_TYPE_RAM_USAGE_PERCENT,
        SENSOR_TYPE_RAM_USAGE_MB,
        SENSOR_TYPE_BATTERY_CHARGE_STATE,
        SENSOR_TYPE_BATTERY_CHARGE_PERCENT,
        SENSOR_TYPE_MAX_SENSORS
        // TODO: define more sensor types
    };


#pragma pack(1)
    const std::uint32_t kMaxSensorCount = 512U;
    const std::uint32_t kMaxLenUnit     = 32U;
    const std::uint32_t kMaxLenLabel    = 64U;

    struct ArgusMonitorSensorData {
        ARGUS_MONITOR_SENSOR_TYPE SensorType;                 // sensor type, see: enum ARGUS_MONITOR_SENSOR_TYPE
        wchar_t                   Label[kMaxLenLabel];        // "user defined name, if available, source name otherwise
        wchar_t                   UnitString[kMaxLenUnit];    // "°C, rpm, %, ..."
        double                    Value;                      // fan speed / fan control value / temperature / load / usage / etc.
        std::uint32_t             Index;                      // for sensor types with multiple instances, Core ID on multi core systems
        std::uint32_t             Extra;                      // usually 0, for Sensors with multiple instances (e.g. CPU, GPU) CPU/GPU index
    };

    struct ArgusMontorData {
        std::uint8_t  Signature[4];    // if Argus Monitor is active: { 'U', 'R', 'S', 'S' } --  otherwise: 4x '0x00'
        std::uint8_t  ArgusMajor;      // e.g. 5
        std::uint8_t  ArgusMinorA;     // e.g. 3
        std::uint8_t  ArgusMinorB;     // e.g. 2
        std::uint8_t  ArgusExtra;      // 0 for now
        std::uint32_t ArgusBuild;      // e.g. 2300
        std::uint32_t Version;         // version of this api data structure, starting with 1 and hopefully being stable for some time
        std::uint32_t CycleCounter;    // starting at 1 (0 is invalid) at every start of Argus Monitor, incremented by 1 for every update

        // for every sensor type the offset of the first sensor in the SensorData array and the number of sensors of that type
        // to help with fast access to one particular type
        // e.g. to get to all FAN-Speeds:
        // the first element will be SensorData[OffsetForSensorType[SENSOR_TYPE_FAN_SPEED_RPM]]
        // and there will be SensorCount[SENSOR_TYPE_FAN_SPEED_RPM] of those elements in total
        std::uint32_t OffsetForSensorType[SENSOR_TYPE_MAX_SENSORS];
        std::uint32_t SensorCount[SENSOR_TYPE_MAX_SENSORS];
        std::uint32_t TotalSensorCount;

        ArgusMonitorSensorData SensorData[kMaxSensorCount];
    };
#pragma pack()


}    // data_api
}    // argus_monitor

#endif    // ARGUS_MONITOR_DATA_MEMORY_API_H
