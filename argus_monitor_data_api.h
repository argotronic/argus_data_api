#ifndef ARGUS_MONITOR_DATA_MEMORY_API_H
#define ARGUS_MONITOR_DATA_MEMORY_API_H

#include <cstdint>

namespace argus_monitor {
namespace data_api {

//    static constexpr wchar_t const* kMappingName{L"Global\\ARGUSMONITOR_DATA_INTERFACE"};
//    static constexpr wchar_t const* kMutexName{L"Global\\ARGUSMONITOR_DATA_INTERFACE_MUTEX"};
//    static constexpr std::size_t    kMappingSize{100 * 1024U};
    inline wchar_t const* kMappingName() { return L"Global\\ARGUSMONITOR_DATA_INTERFACE"; }
    inline wchar_t const* kMutexName() { return L"Global\\ARGUSMONITOR_DATA_INTERFACE_MUTEX"; }
    inline std::size_t    kMappingSize() { return 100 * 1024U; }

    enum ARGUS_MONITOR_SENSOR_TYPE {
        SENSOR_TYPE_INVALID = 0,
        SENSOR_TYPE_TEMPERATURE,
        SENSOR_TYPE_SYNTHETIC_TEMPERATURE,
        SENSOR_TYPE_FAN,
        SENSOR_TYPE_NETWORK_SPEED,
        SENSOR_TYPE_GPU,
        SENSOR_TYPE_MAX_SENSORS
        // TODO: define more sensor types
    };

#pragma pack(1)
    const std::uint32_t kMaxSensorCount = 256U;
    const std::uint32_t kMaxLenUnit     = 8U;
    const std::uint32_t kMaxLenLabel    = 32U;

    struct ArgusMonitorSensorData {
        ARGUS_MONITOR_SENSOR_TYPE SensorType;                 // sensor type, see: enum ARGUS_MONITOR_SENSOR_TYPE
        wchar_t                   Label[kMaxLenLabel];        // "user defined name, if available, source name otherwise
        wchar_t                   UnitString[kMaxLenUnit];    // "°C, rpm, %, ..."
        double                    Value;                      // fan speed / temperature / load / usage / etc.
        std::uint8_t              ControlValue;               // for fan or pumps, the % applied for control; 0xFF if not controlled by Argus
        // TODO: add the superset of potential readings any one sensor could provide
    };

    struct ArgusMontorData {
        std::uint8_t  Signature[4];    // if Argus is active: { 'U', 'R', 'S', 'S' } --  otherwise: 4x '0x00'
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
