#ifndef OBC_COMPONENTS_ONBOARDSTATEDATA_ONBOARDSTATEDATA_HPP
#define OBC_COMPONENTS_ONBOARDSTATEDATA_ONBOARDSTATEDATA_HPP

#include <array>

#include "Fw/Time/Time.hpp"
#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Components/StorageHealthBridge/StorageHealthTypes.hpp"
#include "OBC/Types/BootSlotEnumAc.hpp"
#include "OBC/Types/CommBandEnumAc.hpp"
#include "OBC/Types/SatModeEnumAc.hpp"
#include "simulators/adcs/AdcsTransport.hpp"
#include "simulators/eps/EpsTransport.hpp"
#include "simulators/gps/GpsTypes.hpp"

namespace OBC {
namespace StateData {

constexpr U16 BEACON_VERSION = 2U;
constexpr U16 REDUCED_STATE_VERSION = 1U;
constexpr U32 BEACON_MAGIC = 0x3143424FU;  // OBC1, little endian on wire
constexpr U32 BEACON_V1_WIRE_SIZE = 108U;

constexpr U32 HEALTH_LOW_BATTERY = 1U << 0;
constexpr U32 HEALTH_STORAGE_WARNING = 1U << 1;
constexpr U32 HEALTH_GPS_NO_FIX = 1U << 2;
constexpr U32 HEALTH_COMM_LINK_DOWN = 1U << 3;
constexpr U32 HEALTH_ADCS_HIGH_RATE = 1U << 4;

constexpr U32 FAULT_CRITICAL_BATTERY = 1U << 0;
constexpr U32 FAULT_STORAGE_DEGRADED = 1U << 1;
constexpr U32 FAULT_SUBSYSTEM_STATE_MISSING = 1U << 2;

constexpr U32 QUALITY_EPS_MISSING = 1U << 0;
constexpr U32 QUALITY_ADCS_MISSING = 1U << 1;
constexpr U32 QUALITY_GPS_MISSING = 1U << 2;
constexpr U32 QUALITY_STORAGE_MISSING = 1U << 3;
constexpr U32 QUALITY_RADIO_MISSING = 1U << 4;

enum class DecodeStatus : U32 {
    OK = 0U,
    TOO_SHORT = 1U,
    BAD_MAGIC = 2U,
    UNSUPPORTED_VERSION = 3U,
    CRC_MISMATCH = 4U,
    INVALID_FIELD = 5U,
    BAD_LENGTH = 6U,
};

struct StateSnapshot {
    Fw::Time timestamp = Fw::ZERO_TIME;
    OBC::SatMode mode = OBC::SatMode::SAFE;
    U32 uptimeSec = 0U;
    U16 rebootCount = 0U;

    bool haveEpsStatus = false;
    OBC::EPS::StatusData eps = {};

    bool haveAdcsState = false;
    OBC::ADCS::StateData adcs = {};

    bool haveGpsState = false;
    OBC::GPS::StateData gps = {};

    bool haveStorageHealth = false;
    OBC::STORAGE::HealthState storage = {};

    bool commPassActive = false;
    OBC::CommBand commActiveBand = OBC::CommBand::SBAND;
    U32 commPassRemainingSec = 0U;
    U32 commTotalPasses = 0U;
    bool cspInitialized = false;
    U8 cspLocalNodeId = 0U;
    U32 cspTxPackets = 0U;
    U32 cspRxPackets = 0U;
    U32 cspErrorCount = 0U;
    U32 cspFreeBuffers = 0U;
    bool uartConnected = false;
    U32 uartTxBytes = 0U;
    U32 uartRxBytes = 0U;
    U32 uartTxErrors = 0U;
    U32 uartRxErrors = 0U;
    bool radioLinkConnected = false;
    U32 radioTxBytes = 0U;
    U32 radioRxBytes = 0U;
    U32 radioTxErrors = 0U;
    U32 radioRxErrors = 0U;
    bool haveRadioStatus = false;
    bool radioEnabled = false;
    U8 radioPowerDbm = 0U;
    U32 radioFreqHz = 0U;
    F32 radioTemperatureC = 0.0F;
    I16 radioRssiDbm = 0;

    OBC::BootSlot activeBootSlot = OBC::BootSlot::SLOT_A;
    OBC::BootSlot pendingBootSlot = OBC::BootSlot::NONE;
    OBC::BootSlot lastKnownGoodBootSlot = OBC::BootSlot::SLOT_A;
    bool bootConfirmed = true;
    bool bootStageVerified = false;
    U32 bootExpectedSize = 0U;
    U32 bootLastBootAttemptTime = 0U;
    U32 bootLastErrorCode = 0U;
    U32 bootRemainingConfirmSeconds = 0U;
    U8 bootUpdateProgress = 0U;
    U8 lastResetReason = 0U;
};

struct ReducedStateV1 {
    U16 version = REDUCED_STATE_VERSION;
    Fw::Time timestamp = Fw::ZERO_TIME;
    OBC::SatMode mode = OBC::SatMode::SAFE;
    U32 uptimeSec = 0U;
    U16 rebootCount = 0U;
    OBC::BootSlot activeBootSlot = OBC::BootSlot::SLOT_A;

    F32 batterySoc = 0.0F;
    F32 batteryVoltage = 0.0F;
    F32 batteryCurrent = 0.0F;
    F32 batteryTempC = 0.0F;
    F32 adcsRateNorm = 0.0F;
    U8 adcsMode = 0U;
    bool gpsFixValid = false;
    U32 gpsAcceptedSentences = 0U;
    U32 gpsRejectedSentences = 0U;
    U32 commPassRemainingSec = 0U;
    U32 commTotalPasses = 0U;
    U32 cspTxPackets = 0U;
    U32 cspRxPackets = 0U;
    U32 radioTxBytes = 0U;
    U32 radioRxBytes = 0U;
    U8 storageWarningMask = 0U;
    U8 storageDegradedMask = 0U;

    U32 healthMask = 0U;
    U32 faultMask = 0U;
    U32 qualityMask = 0U;
};

struct BeaconPacket {
    U32 sequence = 0U;
    Fw::Time timestamp = Fw::ZERO_TIME;
    U32 crc = 0U;
    std::array<U8, BEACON_V1_WIRE_SIZE> bytes = {};
};

struct DecodedBeaconV1 {
    U16 version = 0U;
    U32 sequence = 0U;
    Fw::Time timestamp = Fw::ZERO_TIME;
    OBC::SatMode mode = OBC::SatMode::SAFE;
    OBC::BootSlot activeBootSlot = OBC::BootSlot::SLOT_A;
    U16 rebootCount = 0U;
    U32 uptimeSec = 0U;
    U32 healthMask = 0U;
    U32 faultMask = 0U;
    U32 qualityMask = 0U;
    F32 batterySoc = 0.0F;
    F32 batteryVoltage = 0.0F;
    F32 batteryCurrent = 0.0F;
    F32 batteryTempC = 0.0F;
    F32 adcsRateNorm = 0.0F;
    U8 adcsMode = 0U;
    bool gpsFixValid = false;
    U32 gpsAcceptedSentences = 0U;
    U32 gpsRejectedSentences = 0U;
    U8 storageWarningMask = 0U;
    U8 storageDegradedMask = 0U;
    U32 commPassRemainingSec = 0U;
    U32 commTotalPasses = 0U;
    U32 cspTxPackets = 0U;
    U32 cspRxPackets = 0U;
    U32 radioTxBytes = 0U;
    U32 radioRxBytes = 0U;
    U32 crc = 0U;
};

class IStateSnapshotSource {
  public:
    virtual ~IStateSnapshotSource() = default;
    virtual bool readStateSnapshot(StateSnapshot& snapshot) const = 0;
};

class IReducedStateSource {
  public:
    virtual ~IReducedStateSource() = default;
    virtual bool getReducedStateForRuntime(ReducedStateV1& state) const = 0;
};

class IBeaconSink {
  public:
    virtual ~IBeaconSink() = default;
    virtual bool sendBeacon(const U8* data, U32 size) = 0;
};

ReducedStateV1 reduceStateSnapshot(const StateSnapshot& snapshot);

F32 adcsRateNorm(const OBC::ADCS::StateData& state);

BeaconPacket encodeBeaconV1(const ReducedStateV1& state, U32 sequence);

DecodeStatus decodeBeaconV1(const U8* data, U32 size, DecodedBeaconV1& decoded);

U32 crc32(const U8* data, U32 size);

}  // namespace StateData
}  // namespace OBC

#endif
