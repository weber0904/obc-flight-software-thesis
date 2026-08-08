#include "simulators/eps/EpsSimModel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace OBC {
namespace EPS {

namespace {

constexpr float EPS_HEATER_LOAD_A = 0.20F;
constexpr float EPS_HIGH_DRAW_OVERLAY_A = 0.85F;
constexpr float EPS_SOLAR_ON_BASELINE_A = 0.40F;
constexpr float EPS_SOLAR_OFF_BASELINE_A = 0.02F;
constexpr float EPS_CHARGE_RATE_SOC_PER_S_PER_A = 0.008F;
constexpr float EPS_DISCHARGE_RATE_SOC_PER_S_PER_A = 0.020F;
constexpr std::uint32_t EPS_NOISE_SEED = 0x4F424345U;
constexpr std::uint64_t EPS_NOISE_STREAM_LOAD = 0U;
constexpr std::uint64_t EPS_NOISE_STREAM_SOLAR = 1U;
constexpr std::uint64_t EPS_NOISE_STREAM_VBAT = 2U;
constexpr std::uint64_t EPS_NOISE_STREAM_VSOLAR = 3U;
constexpr std::uint64_t EPS_NOISE_STREAM_TEMP = 4U;

std::uint8_t countSetBits(std::uint8_t value) {
    std::uint8_t count = 0U;
    while (value != 0U) {
        count = static_cast<std::uint8_t>(count + (value & 0x1U));
        value = static_cast<std::uint8_t>(value >> 1U);
    }
    return count;
}

}  // namespace

EpsSimModel::EpsSimModel() {
    this->loadDefaults_();
}

ResultCode EpsSimModel::processCspRequest(const CSP::Request& request, CSP::Reply& response) {
    this->refreshDynamics_();
    const CSP::ServicePort service = static_cast<CSP::ServicePort>(request.header.service);
    response = CSP::makeBlankReply(service, request.header.seq);

    if (request.header.version != CSP::VERSION) {
        response.header.result = static_cast<std::uint8_t>(ResultCode::INVALID_REQUEST);
        return ResultCode::INVALID_REQUEST;
    }

    ResultCode result = ResultCode::INVALID_REQUEST;
    switch (service) {
        case CSP::ServicePort::STATUS:
            this->updateDerivedState_();
            response.status = this->m_state;
            result = ResultCode::OK;
            break;
        case CSP::ServicePort::PDU: {
            const std::uint8_t channel = request.payload.pdu.channel;
            if (channel >= 8U) {
                result = ResultCode::INVALID_REQUEST;
                break;
            }

            const std::uint8_t mask = static_cast<std::uint8_t>(1U << channel);
            if (request.payload.pdu.enabled != 0U) {
                this->m_state.pdu_status = static_cast<std::uint8_t>(this->m_state.pdu_status | mask);
            } else {
                this->m_state.pdu_status =
                    static_cast<std::uint8_t>(this->m_state.pdu_status & static_cast<std::uint8_t>(~mask));
            }
            this->updateDerivedState_();
            response.status = this->m_state;
            result = ResultCode::OK;
            break;
        }
        case CSP::ServicePort::CONFIG:
            this->m_state.heater_enabled = request.payload.heater.enabled != 0U ? 1U : 0U;
            this->updateDerivedState_();
            response.status = this->m_state;
            result = ResultCode::OK;
            break;
        case CSP::ServicePort::RESET:
            this->loadDefaults_();
            response.status = this->m_state;
            result = ResultCode::OK;
            break;
        default:
            result = ResultCode::INVALID_REQUEST;
            break;
    }

    response.header.result = static_cast<std::uint8_t>(result);
    return result;
}

void EpsSimModel::applyScenarioState(std::uint8_t sunlight, float soc) {
    this->refreshDynamics_();
    this->m_state.sunlight = sunlight != 0U ? 1U : 0U;
    this->m_state.soc = clampSoc_(soc);
    this->m_socRamp.active = false;
    this->updateDerivedState_();
}

void EpsSimModel::setSocForRuntime(float soc, float transitionSec) {
    this->refreshDynamics_();
    const float clampedSoc = clampSoc_(soc);
    if (transitionSec <= 0.0F || this->m_state.soc == clampedSoc) {
        this->m_state.soc = clampedSoc;
        this->m_socRamp.active = false;
        this->updateDerivedState_();
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    this->m_socRamp.active = true;
    this->m_socRamp.startSoc = this->m_state.soc;
    this->m_socRamp.targetSoc = clampedSoc;
    this->m_socRamp.startTime = now;
    this->m_socRamp.endTime = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                        std::chrono::duration<double>(transitionSec));
}

void EpsSimModel::setLoadModeForRuntime(LoadMode mode) {
    this->refreshDynamics_();
    this->m_loadMode = mode;
    this->updateDerivedState_();
}

LoadMode EpsSimModel::getLoadModeForRuntime() const {
    return this->m_loadMode;
}

void EpsSimModel::setDroppedStatusReplyCount(std::uint32_t count) {
    this->m_droppedStatusReplyCount = count;
}

bool EpsSimModel::consumeDroppedStatusReply(CSP::ServicePort service) {
    if (service != CSP::ServicePort::STATUS || this->m_droppedStatusReplyCount == 0U) {
        return false;
    }
    this->m_droppedStatusReplyCount -= 1U;
    return true;
}

std::uint32_t EpsSimModel::getDroppedStatusReplyCount() const {
    return this->m_droppedStatusReplyCount;
}

void EpsSimModel::reset() {
    this->loadDefaults_();
}

StatusData EpsSimModel::snapshotStateForRuntime() {
    this->refreshDynamics_();
    return this->m_state;
}

void EpsSimModel::advanceForTest(std::chrono::milliseconds delta) {
    if (!this->m_manualTimeActive) {
        this->m_manualNow = this->m_lastUpdateTime;
        this->m_manualTimeActive = true;
    }
    this->m_manualNow += delta;
    this->refreshDynamicsAt_(this->m_manualNow);
}

const StatusData& EpsSimModel::getState() const {
    return this->m_state;
}

float EpsSimModel::clampSoc_(float soc) {
    return std::max(0.0F, std::min(soc, 100.0F));
}

float EpsSimModel::clampUnit_(float value) {
    return std::max(0.0F, std::min(value, 1.0F));
}

float EpsSimModel::loadWeightForChannel_(std::uint8_t channel) {
    switch (channel) {
        case 0U:
            return 0.22F;
        case 1U:
            return 0.12F;
        case 3U:
            return 0.18F;
        case 5U:
            return 0.20F;
        case 6U:
            return 0.14F;
        default:
            return 0.0F;
    }
}

void EpsSimModel::loadDefaults_() {
    this->m_state = {};
    this->m_state.soc = 76.0F;
    this->m_state.pdu_status = 0x03U;
    this->m_state.sunlight = 1U;
    this->m_state.heater_enabled = 0U;
    this->m_state.overcurrent_flags = 0U;
    this->m_socRamp = {};
    this->m_loadMode = LoadMode::NORMAL;
    this->m_referenceTime = std::chrono::steady_clock::now();
    this->m_lastUpdateTime = this->m_referenceTime;
    this->m_manualTimeActive = false;
    this->m_manualNow = this->m_referenceTime;
    this->m_droppedStatusReplyCount = 0U;
    this->updateDerivedState_();
}

void EpsSimModel::refreshSocRamp_(const std::chrono::steady_clock::time_point& now) {
    if (!this->m_socRamp.active) {
        return;
    }
    if (now >= this->m_socRamp.endTime) {
        this->m_state.soc = this->m_socRamp.targetSoc;
        this->m_socRamp.active = false;
        this->updateDerivedState_();
        return;
    }

    const auto elapsed = std::chrono::duration<double>(now - this->m_socRamp.startTime).count();
    const auto total = std::chrono::duration<double>(this->m_socRamp.endTime - this->m_socRamp.startTime).count();
    if (total <= 0.0) {
        this->m_state.soc = this->m_socRamp.targetSoc;
        this->m_socRamp.active = false;
        this->updateDerivedState_();
        return;
    }

    const double ratio = std::max(0.0, std::min(1.0, elapsed / total));
    const float nextSoc =
        this->m_socRamp.startSoc +
        static_cast<float>((this->m_socRamp.targetSoc - this->m_socRamp.startSoc) * ratio);
    if (nextSoc != this->m_state.soc) {
        this->m_state.soc = nextSoc;
        this->updateDerivedState_();
    }
}

void EpsSimModel::refreshDynamics_() {
    this->refreshDynamicsAt_(this->now_());
}

void EpsSimModel::refreshDynamicsAt_(const std::chrono::steady_clock::time_point& now) {
    if (now < this->m_lastUpdateTime) {
        return;
    }

    this->refreshSocRamp_(now);
    const double dt = std::chrono::duration<double>(now - this->m_lastUpdateTime).count();
    if (dt > 0.0) {
        const DerivedState derived = this->computeDerivedState_(now);
        const float dischargeCurrent = std::max(derived.loadCurrent - derived.solarCurrent, 0.0F);
        const float chargeCurrent = std::max(derived.solarCurrent - derived.loadCurrent, 0.0F);
        this->m_state.soc = clampSoc_(
            this->m_state.soc - dischargeCurrent * EPS_DISCHARGE_RATE_SOC_PER_S_PER_A * static_cast<float>(dt) +
            chargeCurrent * EPS_CHARGE_RATE_SOC_PER_S_PER_A * static_cast<float>(dt));
    }

    this->m_lastUpdateTime = now;
    this->updateDerivedState_();
}

std::chrono::steady_clock::time_point EpsSimModel::now_() const {
    return this->m_manualTimeActive ? this->m_manualNow : std::chrono::steady_clock::now();
}

float EpsSimModel::computeNoise_(std::uint64_t streamId,
                                 float amplitude,
                                 const std::chrono::steady_clock::time_point& now) const {
    const std::uint64_t bucket = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(now - this->m_referenceTime).count());
    std::uint32_t value = EPS_NOISE_SEED ^ static_cast<std::uint32_t>(streamId * 0x9E3779B9U);
    value ^= static_cast<std::uint32_t>(bucket + 0x7F4A7C15U);
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    value *= 0x846CA68BU;
    value ^= value >> 16U;
    const float normalized =
        static_cast<float>(static_cast<double>(value) / static_cast<double>(std::numeric_limits<std::uint32_t>::max()));
    return amplitude * ((2.0F * normalized) - 1.0F);
}

EpsSimModel::DerivedState EpsSimModel::computeDerivedState_(const std::chrono::steady_clock::time_point& now) const {
    DerivedState derived = {};

    float baseLoadCurrent = 0.0F;
    for (std::uint8_t channel = 0U; channel < 8U; channel++) {
        const std::uint8_t mask = static_cast<std::uint8_t>(1U << channel);
        if ((this->m_state.pdu_status & mask) != 0U) {
            baseLoadCurrent += loadWeightForChannel_(channel);
        }
    }

    if (this->m_state.heater_enabled != 0U) {
        baseLoadCurrent += EPS_HEATER_LOAD_A;
    }
    if (this->m_loadMode == LoadMode::HIGH_DRAW) {
        baseLoadCurrent += EPS_HIGH_DRAW_OVERLAY_A;
    }

    const float loadNoise = this->computeNoise_(EPS_NOISE_STREAM_LOAD, 0.0125F, now);
    const float solarNoise = this->computeNoise_(EPS_NOISE_STREAM_SOLAR, 0.0125F, now);
    derived.loadCurrent = std::max(0.0F, baseLoadCurrent + loadNoise);

    const float solarBaseline = (this->m_state.sunlight != 0U) ? EPS_SOLAR_ON_BASELINE_A : EPS_SOLAR_OFF_BASELINE_A;
    derived.solarCurrent = std::max(0.0F, solarBaseline + solarNoise);

    const float openCircuitVoltage = 6.95F + (0.014F * std::min(this->m_state.soc, 100.0F));
    const float dischargeCurrent = std::max(derived.loadCurrent - derived.solarCurrent, 0.0F);
    const float vbatNoise = this->computeNoise_(EPS_NOISE_STREAM_VBAT, 0.015F, now);
    derived.vbat = std::max(0.0F, openCircuitVoltage - (0.10F * dischargeCurrent) + vbatNoise);

    if (this->m_state.sunlight != 0U) {
        derived.vsolar = std::max(0.0F, 5.40F + this->computeNoise_(EPS_NOISE_STREAM_VSOLAR, 0.030F, now));
    } else {
        derived.vsolar = 0.0F;
    }

    const float heaterTempTerm = (this->m_state.heater_enabled != 0U) ? 1.5F : 0.0F;
    derived.tempBat =
        24.0F + (1.2F * derived.loadCurrent) + heaterTempTerm + this->computeNoise_(EPS_NOISE_STREAM_TEMP, 0.20F, now);
    return derived;
}

void EpsSimModel::updateDerivedState_() {
    const DerivedState derived = this->computeDerivedState_(this->m_lastUpdateTime);
    this->m_state.isolar = derived.solarCurrent;
    this->m_state.ibat = derived.solarCurrent - derived.loadCurrent;
    this->m_state.vsolar = derived.vsolar;
    this->m_state.vbat = derived.vbat;
    this->m_state.power_out = derived.vbat * derived.loadCurrent;
    this->m_state.temp_bat = derived.tempBat;
    this->m_state.overcurrent_flags = derived.loadCurrent >= 1.40F ? 0x01U : 0x00U;
}

}  // namespace EPS
}  // namespace OBC
