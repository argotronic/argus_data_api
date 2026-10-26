#ifndef ARGUS_MONITOR_DATA_MEMORY_API_H
#define ARGUS_MONITOR_DATA_MEMORY_API_H

#include <cstddef>
#include <cstdint>

namespace argus_monitor::data_api
{
        constexpr wchar_t const* kMappingName()
        {
            return L"Global\\ARGUSMONITOR_DATA_INTERFACE";
        }
        constexpr wchar_t const* kMutexName()
        {
            return L"Global\\ARGUSMONITOR_DATA_INTERFACE_MUTEX";
        }
        constexpr std::size_t kMappingSize()
        {
            return 1024U * 1024U;
        }

        // ArgusMonitorData::Signature while Argus Monitor provides data ("ArgM"), 0 otherwise
        constexpr std::uint32_t kSignature{ 0x4D677241U };

        // ArgusMonitorData::Version of the layout in this header. From version 3 on, Argus Monitor raises Version only for
        // backward compatible extensions (new sensor types, use of Reserved); an incompatible layout gets new object names.
        // Readers therefore accept every Version >= kApiVersion (see IsValidSensorData()).
        constexpr std::uint32_t kApiVersion{ 3U };

        enum ARGUS_MONITOR_SENSOR_TYPE : std::uint8_t {
            SENSOR_TYPE_INVALID = 0,
            SENSOR_TYPE_TEMPERATURE,              // temperatures of mainboard sensors, external fan controllers and AIOs
            SENSOR_TYPE_SYNTHETIC_TEMPERATURE,    // user defined synthetic temperatures (mean, max, average, difference, ...)
            SENSOR_TYPE_FAN_SPEED_RPM,    // fan speed of fans attached to mainboard channels, AIOs, external fan controllers and also pump speeds of
                                          // AIOs
            SENSOR_TYPE_FAN_CONTROL_VALUE,             // if any fan/pump is controlled by Argus Monitor, then the control value can be read from this
            SENSOR_TYPE_NETWORK_SPEED,                 // up/down speeds of network adapters if selected to be monitored inside Argus Monitor
            SENSOR_TYPE_CPU_TEMPERATURE,               // the normal CPU temperature readings, per core for Intel and the only one available for AMD
            SENSOR_TYPE_CPU_TEMPERATURE_ADDITIONAL,    // additional temperatures provided by the CPU, like CCDx temperatures of AMD CPUs
            SENSOR_TYPE_CPU_MULTIPLIER,    // multiplier value for every core (inside the CPU, those values are changed MUCH faster than the sampling
                                           // frequency)
            SENSOR_TYPE_CPU_FREQUENCY_FSB,    // core frequencies can be calculated by multiplying FSB frequency by the multipliers
            SENSOR_TYPE_GPU_TEMPERATURE,
            SENSOR_TYPE_GPU_NAME,             // the name of the GPU (e.g. "Nvidia RTX3080")
            SENSOR_TYPE_GPU_LOAD,
            SENSOR_TYPE_GPU_CORECLK,
            SENSOR_TYPE_GPU_MEMORYCLK,
            SENSOR_TYPE_GPU_SHARERCLK,
            SENSOR_TYPE_GPU_FAN_SPEED_PERCENT,
            SENSOR_TYPE_GPU_FAN_SPEED_RPM,
            SENSOR_TYPE_GPU_MEMORY_USED_PERCENT,
            SENSOR_TYPE_GPU_MEMORY_USED_MB,
            SENSOR_TYPE_GPU_POWER,
            SENSOR_TYPE_DISK_TEMPERATURE,
            SENSOR_TYPE_DISK_TRANSFER_RATE,
            SENSOR_TYPE_CPU_LOAD,
            SENSOR_TYPE_RAM_USAGE,
            SENSOR_TYPE_BATTERY,
            SENSOR_TYPE_CPU_POWER,
            SENSOR_TYPE_MAX_SENSORS
        };

        const std::uint8_t kMaxSensorTypes = 32U;
        static_assert(SENSOR_TYPE_MAX_SENSORS <= kMaxSensorTypes, "Add reserved capacity before introducing a new SENSOR_TYPE");

#pragma pack(push, 1)
        const std::uint32_t kMaxSensorCount = 512U;
        const std::uint32_t kMaxLenUnit     = 32U;
        const std::uint32_t kMaxLenLabel    = 64U;

        struct ArgusMonitorSensorData {
            ARGUS_MONITOR_SENSOR_TYPE SensorType;                 // sensor type, see: enum ARGUS_MONITOR_SENSOR_TYPE
            wchar_t                   Label[kMaxLenLabel];        // user defined name, if available, source name otherwise (*)
            wchar_t                   UnitString[kMaxLenUnit];    // "°C, rpm, %, ..." (*)
                                                                  // (*) read at most the array size, the terminating 0 may be missing
            double                    Value;                      // fan speed / fan control value / temperature / load / usage / etc.
            std::uint32_t             DataIndex;      // for sensor which can provide multiple different readings, Core ID on multi core systems
            std::uint32_t             SensorIndex;    // for Sensors with multiple instances (e.g. CPU, GPU) CPU/GPU index
        };

        // Reading: open the mapping kMappingName() (read access is enough) and the mutex kMutexName(), copy ArgusMonitorData
        // while holding the mutex (keep that short, Argus Monitor waits only briefly for it) and use the copy only if
        // IsValidSensorData() accepts it. A changed CycleCounter means new data.
        struct ArgusMonitorData {
            std::uint32_t Signature;       // kSignature while Argus Monitor provides data, 0 otherwise
            std::uint8_t  ArgusMajor;      // e.g. 5
            std::uint8_t  ArgusMinorA;     // e.g. 3
            std::uint8_t  ArgusMinorB;     // e.g. 2
            std::uint8_t  ArgusExtra;      // 0 for now
            std::uint32_t ArgusBuild;      // e.g. 2300
            std::uint32_t Version;         // layout version, see kApiVersion
            std::uint32_t CycleCounter;    // starting at 1 (0 is invalid) at every start of Argus Monitor, incremented by 1 for every update

            // for every sensor type the offset of the first sensor in the SensorData array and the number of sensors of that type
            // to help with fast access to one particular type
            // e.g. to get to all FAN-Speeds:
            // the first element will be SensorData[OffsetForSensorType[SENSOR_TYPE_FAN_SPEED_RPM]]
            // and there will be SensorCount[SENSOR_TYPE_FAN_SPEED_RPM] of those elements in total
            std::uint32_t OffsetForSensorType[kMaxSensorTypes];
            std::uint32_t SensorCount[kMaxSensorTypes];
            std::uint32_t TotalSensorCount;

            std::uint32_t Reserved[16];

            ArgusMonitorSensorData SensorData[kMaxSensorCount];
        };
#pragma pack(pop)

        // the layout is shared with other processes: fail the build instead of reading a different one
        static_assert(sizeof(wchar_t) == 2U, "the data API needs a 16 bit wchar_t (Windows)");
        static_assert(sizeof(ArgusMonitorSensorData) == 209U, "ArgusMonitorSensorData layout changed");
        static_assert(offsetof(ArgusMonitorData, SensorData) == 344U, "ArgusMonitorData header layout changed");
        static_assert(sizeof(ArgusMonitorData) == 344U + kMaxSensorCount * 209U, "ArgusMonitorData layout changed");
        static_assert(sizeof(ArgusMonitorData) <= kMappingSize(), "ArgusMonitorData does not fit into the mapping");

        // true if `data` is a complete snapshot of this layout: written by a running Argus Monitor, with a compatible
        // Version and with every sensor range inside the TotalSensorCount entries of SensorData
        [[nodiscard]] constexpr bool IsValidSensorData(ArgusMonitorData const& data) noexcept
        {
            if (data.Signature != kSignature || data.Version < kApiVersion || data.TotalSensorCount > kMaxSensorCount) {
                return false;
            }
            for (std::uint32_t type{ 0U }; type < kMaxSensorTypes; ++type) {
                // compared by subtraction, so a huge offset or count cannot overflow
                if (data.OffsetForSensorType[type] > data.TotalSensorCount
                    || data.SensorCount[type] > data.TotalSensorCount - data.OffsetForSensorType[type]) {
                    return false;
                }
            }
            return true;
        }

}    // argus_monitor::data_api

#endif    // ARGUS_MONITOR_DATA_MEMORY_API_H
