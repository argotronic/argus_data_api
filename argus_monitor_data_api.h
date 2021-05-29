#ifndef ARGUS_MONITOR_DATA_MEMORY_API_H
#define ARGUS_MONITOR_DATA_MEMORY_API_H

#include <cstdint>

namespace argus_monitor {
namespace data_api {

    typedef std::uint32_t DWORD;

    enum ARGUS_MONITOR_SENSOR_TYPE {
        SENSOR_TYPE_INVALID = 0,
        SENSOR_TYPE_TEMPERATURE,
        SENSOR_TYPE_SYNTHETIC_TEMPERATURE,
        SENSOR_TYPE_FAN,
        SENSOR_TYPE_NETWORK_SPEED,
        SENSOR_TYPE_GPU_LOAD,
        SENSOR_TYPE_MAX_SENSORS
        // TODO: define more sensor types
    };

#pragma pack(1)
    const size_t kMaxSensorCount = 256U;
    const size_t kMaxLenUnit     = 8U;
    const size_t kMaxLenLabel    = 32U;

    struct ArgusMonitorSensorData {
        ARGUS_MONITOR_SENSOR_TYPE SensorType;                 // sensor type, see: enum ARGUS_MONITOR_SENSOR_TYPE
        wchar_t                   Label[kMaxLenLabel];        // "user defined name, if available, source name otherwise
        wchar_t                   UnitString[kMaxLenUnit];    // "°C, rpm, %, ..."
        double                    Value;                      // fan speed / temperature / load / usage / etc.
        std::uint8_t              ControlValue;               // for fan or pumps, the % applied for control; 0xFF if not controlled by Argus
        // TODO: add the superset of potential readings any one sensor could provide
    };

    struct ArgusMontorData {
        std::uint8_t Signature[4];    // if Argus is active: { 'U', 'R', 'S', 'S' } --  otherwise: 4x '0x00'
        std::uint8_t ArgusMajor;      // e.g. 5
        std::uint8_t ArgusMinorA;     // e.g. 3
        std::uint8_t ArgusMinorB;     // e.g. 2
        std::uint8_t ArgusExtra;      // 0 for now
        DWORD        ArgusBuild;      // e.g. 2300
        DWORD        Version;         // version of this api data structure, starting with 1 and hopefully being stable for some time
        DWORD        CycleCounter;    // starting at 1 (0 is invalid) at every start of Argus Monitor, incremented by 1 for every update

        // for every sensor type the offset of the first sensor in the SensorData array and the number of sensors of that type
        // to help with fast access to one particular type
        // e.g. to get to all FAN-Speeds:
        // the first element will be SensorData[OffsetForSensorType[SENSOR_TYPE_FAN_SPEED_RPM]]
        // and there will be SensorCount[SENSOR_TYPE_FAN_SPEED_RPM] of those elements in total
        DWORD OffsetForSensorType[SENSOR_TYPE_MAX_SENSORS];
        DWORD SensorCount[SENSOR_TYPE_MAX_SENSORS];
        DWORD TotalSensorCount;

        ArgusMonitorSensorData SensorData[kMaxSensorCount];
    };
#pragma pack()


}    // data_api
}    // argus_monitor

#endif    // ARGUS_MONITOR_DATA_MEMORY_API_H
