#include "OBC/Components/OnboardStateData/OnboardStateData.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <limits>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

OBC::StateData::StateSnapshot nominalSnapshot() {
    OBC::StateData::StateSnapshot snapshot = {};
    snapshot.timestamp = Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 100U, 0U);
    snapshot.mode = OBC::SatMode::IDLE;
    snapshot.uptimeSec = 42U;
    snapshot.rebootCount = 2U;
    snapshot.haveEpsStatus = true;
    snapshot.eps.soc = 76.0F;
    snapshot.eps.vbat = 8.1F;
    snapshot.eps.ibat = 1.25F;
    snapshot.eps.temp_bat = 24.5F;
    snapshot.haveAdcsState = true;
    snapshot.adcs.omega_x = 0.01F;
    snapshot.adcs.omega_y = 0.02F;
    snapshot.adcs.omega_z = 0.01F;
    snapshot.adcs.mode = 2U;
    snapshot.haveGpsState = true;
    snapshot.gps.fixValid = true;
    snapshot.gps.acceptedSentenceCount = 10U;
    snapshot.haveStorageHealth = true;
    snapshot.storage.warningActive = false;
    snapshot.storage.warningMask = 0U;
    snapshot.storage.degradedMask = 0U;
    snapshot.uartConnected = true;
    snapshot.haveRadioStatus = true;
    snapshot.radioLinkConnected = true;
    snapshot.radioTxBytes = 64U;
    snapshot.radioRxBytes = 128U;
    snapshot.activeBootSlot = OBC::BootSlot::SLOT_A;
    return snapshot;
}

template <std::size_t N>
void writeU32(std::array<U8, N>& data, U32 offset, U32 value) {
    data[offset] = static_cast<U8>(value & 0xFFU);
    data[offset + 1U] = static_cast<U8>((value >> 8U) & 0xFFU);
    data[offset + 2U] = static_cast<U8>((value >> 16U) & 0xFFU);
    data[offset + 3U] = static_cast<U8>((value >> 24U) & 0xFFU);
}

U32 readU32(const std::array<U8, OBC::StateData::BEACON_V1_WIRE_SIZE>& data, U32 offset) {
    return static_cast<U32>(data[offset]) | (static_cast<U32>(data[offset + 1U]) << 8U) |
           (static_cast<U32>(data[offset + 2U]) << 16U) | (static_cast<U32>(data[offset + 3U]) << 24U);
}

void refreshCrc(std::array<U8, OBC::StateData::BEACON_V1_WIRE_SIZE>& data) {
    const U32 fixedCrc = OBC::StateData::crc32(data.data(), OBC::StateData::BEACON_V1_WIRE_SIZE - 4U);
    writeU32(data, OBC::StateData::BEACON_V1_WIRE_SIZE - 4U, fixedCrc);
}

bool testReducedStateAndBeacon() {
    using namespace OBC::StateData;
    const ReducedStateV1 reduced = reduceStateSnapshot(nominalSnapshot());
    bool ok = true;
    ok = check(reduced.qualityMask == 0U, "nominal reduced state should have no missing-source flags") && ok;
    ok = check(reduced.healthMask == 0U, "nominal reduced state should have no health flags") && ok;
    ok = check(reduced.batteryCurrent == 1.25F, "reduced state should preserve EPS battery current") && ok;
    const BeaconPacket beacon = encodeBeaconV1(reduced, 7U);
    ok = check(beacon.sequence == 7U, "beacon sequence should be preserved") && ok;
    ok = check(beacon.bytes.size() == BEACON_V1_WIRE_SIZE, "beacon should contain fixed V1 fields") && ok;
    ok = check(beacon.bytes[4] == static_cast<U8>(BEACON_VERSION) && beacon.bytes[5] == 0U,
               "beacon should emit the current mode-model-v2 schema version") &&
         ok;
    const U32 expectedCrc = crc32(beacon.bytes.data(), static_cast<U32>(beacon.bytes.size() - 4U));
    ok = check(expectedCrc == beacon.crc, "beacon CRC should cover payload before CRC field") && ok;
    DecodedBeaconV1 decodedBeacon = {};
    ok = check(decodeBeaconV1(beacon.bytes.data(), static_cast<U32>(beacon.bytes.size()), decodedBeacon) ==
                   DecodeStatus::OK,
               "beacon decode should accept generated packet") &&
         ok;
    ok = check(decodedBeacon.version == BEACON_VERSION, "beacon decoder should report current schema version") && ok;
    ok = check(decodedBeacon.sequence == 7U, "beacon decoder should preserve sequence") && ok;
    ok = check(decodedBeacon.mode == OBC::SatMode::IDLE, "beacon decoder should preserve mode") && ok;
    ok = check(decodedBeacon.batteryCurrent == 1.25F, "beacon decoder should preserve EPS battery current") && ok;
    ok = check(decodedBeacon.batteryTempC == 24.5F, "beacon decoder should preserve EPS battery temperature") && ok;
    ok = check(decodedBeacon.radioTxBytes == 64U, "beacon decoder should preserve radio TX bytes") && ok;
    return ok;
}

bool testBeaconDecodeRejectsBadInputs() {
    using namespace OBC::StateData;
    const ReducedStateV1 reduced = reduceStateSnapshot(nominalSnapshot());
    BeaconPacket beacon = encodeBeaconV1(reduced, 9U);
    DecodedBeaconV1 decoded = {};
    bool ok = true;
    ok = check(decodeBeaconV1(beacon.bytes.data(), BEACON_V1_WIRE_SIZE - 1U, decoded) == DecodeStatus::TOO_SHORT,
               "beacon decoder should reject short packets") &&
         ok;

    std::array<U8, BEACON_V1_WIRE_SIZE + 1U> extended = {};
    std::memcpy(extended.data(), beacon.bytes.data(), BEACON_V1_WIRE_SIZE - 4U);
    extended[BEACON_V1_WIRE_SIZE - 4U] = 0x5AU;
    const U32 extendedCrc = crc32(extended.data(), static_cast<U32>(extended.size() - 4U));
    writeU32(extended, static_cast<U32>(extended.size() - 4U), extendedCrc);
    ok = check(decodeBeaconV1(extended.data(), static_cast<U32>(extended.size()), decoded) ==
                   DecodeStatus::BAD_LENGTH,
               "beacon decoder should reject non-canonical extended packets") &&
         ok;

    auto corrupted = beacon.bytes;
    corrupted[12] ^= 0x5AU;
    ok = check(decodeBeaconV1(corrupted.data(), static_cast<U32>(corrupted.size()), decoded) ==
                   DecodeStatus::CRC_MISMATCH,
               "beacon decoder should reject corrupted payload") &&
         ok;

    corrupted = beacon.bytes;
    corrupted[0] = 0U;
    refreshCrc(corrupted);
    ok = check(decodeBeaconV1(corrupted.data(), static_cast<U32>(corrupted.size()), decoded) ==
                   DecodeStatus::BAD_MAGIC,
               "beacon decoder should reject bad magic after CRC passes") &&
         ok;

    auto oldSchemaVersion = beacon.bytes;
    oldSchemaVersion[4] = 1U;
    oldSchemaVersion[5] = 0U;
    refreshCrc(oldSchemaVersion);
    ok = check(decodeBeaconV1(oldSchemaVersion.data(), static_cast<U32>(oldSchemaVersion.size()), decoded) ==
                   DecodeStatus::UNSUPPORTED_VERSION,
               "beacon decoder should reject pre-v2 mode schema version after CRC passes") &&
         ok;
    return ok;
}

bool testBeaconDecodeRejectsInvalidWireFields() {
    using namespace OBC::StateData;
    const ReducedStateV1 reduced = reduceStateSnapshot(nominalSnapshot());
    const BeaconPacket beacon = encodeBeaconV1(reduced, 10U);
    DecodedBeaconV1 decoded = {};
    bool ok = true;

    auto invalidTimeBase = beacon.bytes;
    writeU32(invalidTimeBase, 12U, 99U);
    refreshCrc(invalidTimeBase);
    ok = check(decodeBeaconV1(invalidTimeBase.data(), static_cast<U32>(invalidTimeBase.size()), decoded) ==
                   DecodeStatus::INVALID_FIELD,
               "beacon decoder should reject invalid time base values") &&
         ok;

    auto invalidMode = beacon.bytes;
    invalidMode[28] = 99U;
    refreshCrc(invalidMode);
    decoded.sequence = 0xAABBCCDDU;
    decoded.mode = OBC::SatMode::IDLE;
    ok = check(decodeBeaconV1(invalidMode.data(), static_cast<U32>(invalidMode.size()), decoded) ==
                   DecodeStatus::INVALID_FIELD,
               "beacon decoder should reject invalid mode values") &&
         ok;
    ok = check(decoded.sequence == 0xAABBCCDDU && decoded.mode == OBC::SatMode::IDLE,
               "beacon decoder should leave output unchanged on invalid fields") &&
         ok;

    auto invalidBootSlot = beacon.bytes;
    invalidBootSlot[29] = 42U;
    refreshCrc(invalidBootSlot);
    ok = check(decodeBeaconV1(invalidBootSlot.data(), static_cast<U32>(invalidBootSlot.size()), decoded) ==
                   DecodeStatus::INVALID_FIELD,
               "beacon decoder should reject invalid boot slot values") &&
         ok;
    return ok;
}

bool testBeaconAcceptsAllModeV2Values() {
    using namespace OBC::StateData;
    bool ok = true;
    U32 sequence = 20U;
    for (U8 modeValue = static_cast<U8>(OBC::SatMode::SAFE); modeValue <= static_cast<U8>(OBC::SatMode::TTC);
         modeValue++) {
        const OBC::SatMode mode(static_cast<OBC::SatMode::T>(modeValue));
        StateSnapshot snapshot = nominalSnapshot();
        snapshot.mode = mode;
        const ReducedStateV1 reduced = reduceStateSnapshot(snapshot);
        const BeaconPacket beacon = encodeBeaconV1(reduced, sequence++);
        DecodedBeaconV1 decoded = {};
        ok = check(decodeBeaconV1(beacon.bytes.data(), static_cast<U32>(beacon.bytes.size()), decoded) ==
                       DecodeStatus::OK,
                   "beacon decoder should accept all SatMode v2 values") &&
             ok;
        ok = check(decoded.mode == mode, "beacon decoder should preserve each SatMode v2 value") && ok;
    }
    return ok;
}

bool testBeaconEncodeSanitizesNonFiniteFloats() {
    using namespace OBC::StateData;
    ReducedStateV1 reduced = reduceStateSnapshot(nominalSnapshot());
    reduced.batteryTempC = std::numeric_limits<F32>::quiet_NaN();
    reduced.adcsRateNorm = std::numeric_limits<F32>::infinity();

    const BeaconPacket beacon = encodeBeaconV1(reduced, 11U);
    DecodedBeaconV1 decoded = {};
    bool ok = check(decodeBeaconV1(beacon.bytes.data(), static_cast<U32>(beacon.bytes.size()), decoded) ==
                        DecodeStatus::OK,
                    "beacon decoder should accept packets with sanitized non-finite source fields");
    ok = check(decoded.batteryTempC == 0.0F, "beacon encoder should sanitize NaN floats to zero") && ok;
    ok = check(decoded.adcsRateNorm == 0.0F, "beacon encoder should sanitize Inf floats to zero") && ok;
    return ok;
}

bool testBeaconEncodeClampsScaledFloatOverflow() {
    using namespace OBC::StateData;
    ReducedStateV1 reduced = reduceStateSnapshot(nominalSnapshot());
    reduced.batterySoc = std::numeric_limits<F32>::max();
    reduced.batteryVoltage = -std::numeric_limits<F32>::max();

    const BeaconPacket beacon = encodeBeaconV1(reduced, 12U);
    const U32 batterySocRaw = readU32(beacon.bytes, 48U);
    const U32 batteryVoltageRaw = readU32(beacon.bytes, 52U);
    bool ok = true;
    ok = check(batterySocRaw == static_cast<U32>(std::numeric_limits<I32>::max()),
               "beacon encoder should clamp positive scaled float overflow") &&
         ok;
    ok = check(batteryVoltageRaw == static_cast<U32>(std::numeric_limits<I32>::min()),
               "beacon encoder should clamp negative scaled float overflow") &&
         ok;
    return ok;
}

bool testBeaconDecodeHandlesNegativeScaledFields() {
    using namespace OBC::StateData;
    ReducedStateV1 reduced = reduceStateSnapshot(nominalSnapshot());
    reduced.batteryCurrent = -1.25F;
    reduced.batteryTempC = -24.5F;

    const BeaconPacket beacon = encodeBeaconV1(reduced, 13U);
    DecodedBeaconV1 decoded = {};
    bool ok = check(decodeBeaconV1(beacon.bytes.data(), static_cast<U32>(beacon.bytes.size()), decoded) ==
                        DecodeStatus::OK,
                    "beacon decoder should accept negative scaled fields");
    ok = check(decoded.batteryCurrent == -1.25F, "beacon decoder should preserve negative EPS current") && ok;
    ok = check(decoded.batteryTempC == -24.5F, "beacon decoder should preserve negative EPS temperature") && ok;
    return ok;
}

bool testCrc32KnownVector() {
    using namespace OBC::StateData;
    const char* const vector = "123456789";
    return check(crc32(reinterpret_cast<const U8*>(vector), static_cast<U32>(std::strlen(vector))) == 0xCBF43926U,
                 "CRC32 should match the standard test vector");
}

bool testMissingStateFlags() {
    using namespace OBC::StateData;
    StateSnapshot snapshot = nominalSnapshot();
    snapshot.haveEpsStatus = false;
    snapshot.haveAdcsState = false;
    const ReducedStateV1 reduced = reduceStateSnapshot(snapshot);
    bool ok = true;
    ok = check((reduced.qualityMask & QUALITY_EPS_MISSING) != 0U, "missing EPS should set quality flag") && ok;
    ok = check((reduced.qualityMask & QUALITY_ADCS_MISSING) != 0U, "missing ADCS should set quality flag") && ok;
    ok = check((reduced.faultMask & FAULT_SUBSYSTEM_STATE_MISSING) != 0U, "missing critical state should set fault flag") && ok;
    return ok;
}

}  // namespace

int main() {
    bool ok = true;
    ok = testReducedStateAndBeacon() && ok;
    ok = testBeaconDecodeRejectsBadInputs() && ok;
    ok = testBeaconDecodeRejectsInvalidWireFields() && ok;
    ok = testBeaconAcceptsAllModeV2Values() && ok;
    ok = testBeaconEncodeSanitizesNonFiniteFloats() && ok;
    ok = testBeaconEncodeClampsScaledFloatOverflow() && ok;
    ok = testBeaconDecodeHandlesNegativeScaledFields() && ok;
    ok = testCrc32KnownVector() && ok;
    ok = testMissingStateFlags() && ok;
    if (!ok) {
        return 1;
    }
    std::cout << "onboard_state_data_unit_test: PASS\n";
    return 0;
}
