#include "simulators/adcs/AdcsSimModel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace OBC {
namespace ADCS {

namespace {

constexpr double ADCS_NOISE_SEED = 0x41444353U;
constexpr double ADCS_IDLE_OMEGA_JITTER = 0.002;
constexpr double ADCS_IDLE_QUAT_JITTER = 0.0008;
constexpr double ADCS_DETUMBLE_JITTER = 0.0015;
constexpr double ADCS_POINTING_CYCLE_SEC = 60.0;
constexpr double ADCS_POINTING_TRACK_TAU_SEC = 2.5;
constexpr double ADCS_DETUMBLE_TAU_SEC = 10.0;
constexpr double ADCS_POINTING_MAX_RATE_NORM = 0.049;

constexpr std::uint64_t ADCS_NOISE_STREAM_IDLE_X = 0U;
constexpr std::uint64_t ADCS_NOISE_STREAM_IDLE_Y = 1U;
constexpr std::uint64_t ADCS_NOISE_STREAM_IDLE_Z = 2U;
constexpr std::uint64_t ADCS_NOISE_STREAM_IDLE_Q1 = 3U;
constexpr std::uint64_t ADCS_NOISE_STREAM_IDLE_Q2 = 4U;
constexpr std::uint64_t ADCS_NOISE_STREAM_IDLE_Q3 = 5U;
constexpr std::uint64_t ADCS_NOISE_STREAM_DETUMBLE_X = 6U;
constexpr std::uint64_t ADCS_NOISE_STREAM_DETUMBLE_Y = 7U;
constexpr std::uint64_t ADCS_NOISE_STREAM_DETUMBLE_Z = 8U;
constexpr std::uint64_t ADCS_NOISE_STREAM_DETUMBLE_Q1 = 9U;
constexpr std::uint64_t ADCS_NOISE_STREAM_DETUMBLE_Q2 = 10U;
constexpr std::uint64_t ADCS_NOISE_STREAM_DETUMBLE_Q3 = 11U;

float omegaNorm(const StateData& state) {
    return std::sqrt(state.omega_x * state.omega_x + state.omega_y * state.omega_y + state.omega_z * state.omega_z);
}

double clampSigned(double value, double bound) {
    return std::max(-bound, std::min(bound, value));
}

}  // namespace

AdcsSimModel::AdcsSimModel() {
    this->loadDefaults_();
}

void AdcsSimModel::setDroppedStateReplyCount(std::uint32_t count) {
    this->m_droppedStateReplyCount = count;
}

std::uint32_t AdcsSimModel::getDroppedStateReplyCount() const {
    return this->m_droppedStateReplyCount;
}

bool AdcsSimModel::consumeDroppedStateReply(const CSP::ServicePort service) {
    if (service != CSP::ServicePort::STATE || this->m_droppedStateReplyCount == 0U) {
        return false;
    }

    this->m_droppedStateReplyCount -= 1U;
    return true;
}

ResultCode AdcsSimModel::processCspRequest(const CSP::Request& request, CSP::Reply& response) {
    this->stepDynamics_();
    const CSP::ServicePort service = static_cast<CSP::ServicePort>(request.header.service);
    response = CSP::makeBlankReply(service, request.header.seq);

    if (request.header.version != CSP::VERSION) {
        response.header.result = static_cast<std::uint8_t>(ResultCode::INVALID_REQUEST);
        return ResultCode::INVALID_REQUEST;
    }

    ResultCode result = ResultCode::INVALID_REQUEST;
    switch (service) {
        case CSP::ServicePort::STATE:
            this->stepDynamics_();
            response.state = this->m_state;
            result = ResultCode::OK;
            break;
        case CSP::ServicePort::MODE:
            if (request.payload.mode.mode > 2U) {
                result = ResultCode::INVALID_REQUEST;
                break;
            }
            if (request.payload.mode.mode == 2U && this->m_state.mode != 2U) {
                this->m_pointingPassElapsedSec = 0.0;
            }
            this->m_state.mode = request.payload.mode.mode;
            this->stepDynamics_();
            response.state = this->m_state;
            result = ResultCode::OK;
            break;
        case CSP::ServicePort::TARGET:
            this->m_targetQ0 = request.payload.target.q0;
            this->m_targetQ1 = request.payload.target.q1;
            this->m_targetQ2 = request.payload.target.q2;
            this->m_targetQ3 = request.payload.target.q3;
            this->m_externalTargetActive = true;
            this->normalizeQuaternion_(this->m_targetQ0, this->m_targetQ1, this->m_targetQ2, this->m_targetQ3);
            this->stepDynamics_();
            response.state = this->m_state;
            result = ResultCode::OK;
            break;
        case CSP::ServicePort::CALIBRATE:
            if (request.payload.calibrate.sensor_id > 2U) {
                result = ResultCode::INVALID_REQUEST;
                break;
            }
            this->m_state.sensor_valid = 1U;
            response.state = this->m_state;
            result = ResultCode::OK;
            break;
        case CSP::ServicePort::RESET:
            this->reset();
            response.state = this->m_state;
            result = ResultCode::OK;
            break;
        default:
            result = ResultCode::INVALID_REQUEST;
            break;
    }

    response.header.result = static_cast<std::uint8_t>(result);
    return result;
}

void AdcsSimModel::seedScenarioAngularRate(float omegaX, float omegaY, float omegaZ) {
    this->stepDynamics_();
    this->m_state.omega_x = omegaX;
    this->m_state.omega_y = omegaY;
    this->m_state.omega_z = omegaZ;
    this->refreshDerivedState_();
}

void AdcsSimModel::restartPointingPassForRuntime() {
    this->stepDynamics_();
    this->m_pointingPassElapsedSec = 0.0;
    this->refreshDerivedState_();
}

void AdcsSimModel::advanceForTest(std::chrono::milliseconds delta) {
    if (!this->m_manualTimeActive) {
        this->m_manualNow = this->m_lastUpdateTime;
        this->m_manualTimeActive = true;
    }
    this->m_manualNow += delta;
    this->stepDynamicsAt_(this->m_manualNow);
}

void AdcsSimModel::reset() {
    this->loadDefaults_();
}

const StateData& AdcsSimModel::getState() const {
    return this->m_state;
}

void AdcsSimModel::loadDefaults_() {
    this->m_state = {};
    this->m_state.q0 = 1.0;
    this->m_state.q1 = 0.0;
    this->m_state.q2 = 0.0;
    this->m_state.q3 = 0.0;
    this->m_state.omega_x = 0.12F;
    this->m_state.omega_y = -0.08F;
    this->m_state.omega_z = 0.06F;
    this->m_state.mag_x = 0.21F;
    this->m_state.mag_y = -0.04F;
    this->m_state.mag_z = 0.44F;
    this->m_state.mode = 0U;
    this->m_state.sensor_valid = 1U;
    this->m_targetQ0 = 1.0;
    this->m_targetQ1 = 0.0;
    this->m_targetQ2 = 0.0;
    this->m_targetQ3 = 0.0;
    this->m_externalTargetActive = false;
    this->m_pointingPassElapsedSec = 0.0;
    this->m_referenceTime = std::chrono::steady_clock::now();
    this->m_lastUpdateTime = this->m_referenceTime;
    this->m_manualTimeActive = false;
    this->m_manualNow = this->m_referenceTime;
    this->m_droppedStateReplyCount = 0U;
    this->refreshDerivedState_();
}

void AdcsSimModel::refreshDerivedState_() {
    this->m_state.mag_x = 0.21F + this->m_state.omega_x * 0.1F;
    this->m_state.mag_y = -0.04F + this->m_state.omega_y * 0.1F;
    this->m_state.mag_z = 0.44F + this->m_state.omega_z * 0.1F;
    this->m_state.pointing_error_deg = this->pointingErrorDeg_();
}

void AdcsSimModel::stepDynamics_() {
    this->stepDynamicsAt_(this->now_());
}

void AdcsSimModel::stepDynamicsAt_(const std::chrono::steady_clock::time_point& now) {
    if (now < this->m_lastUpdateTime) {
        return;
    }

    const double dt = std::chrono::duration<double>(now - this->m_lastUpdateTime).count();
    if (dt <= 0.0) {
        this->refreshDerivedState_();
        return;
    }

    const std::uint8_t mode = this->m_state.mode;
    if (mode == 1U) {
        this->updateDetumbleDynamics_(dt, now);
    } else if (mode == 2U) {
        this->updatePointingDynamics_(dt, now);
    } else {
        this->updateIdleDynamics_(dt, now);
    }

    this->m_lastUpdateTime = now;
    this->refreshDerivedState_();
}

std::chrono::steady_clock::time_point AdcsSimModel::now_() const {
    return this->m_manualTimeActive ? this->m_manualNow : std::chrono::steady_clock::now();
}

double AdcsSimModel::computeNoise_(std::uint64_t streamId,
                                   double amplitude,
                                   const std::chrono::steady_clock::time_point& now) const {
    const std::uint64_t bucket = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(now - this->m_referenceTime).count());
    std::uint32_t value = static_cast<std::uint32_t>(ADCS_NOISE_SEED) ^
                          static_cast<std::uint32_t>(streamId * 0x9E3779B9U) ^
                          static_cast<std::uint32_t>(bucket + 0x7F4A7C15U);
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    value *= 0x846CA68BU;
    value ^= value >> 16U;
    const double normalized =
        static_cast<double>(value) / static_cast<double>(std::numeric_limits<std::uint32_t>::max());
    return amplitude * ((2.0 * normalized) - 1.0);
}

void AdcsSimModel::updateIdleDynamics_(double dt, const std::chrono::steady_clock::time_point& now) {
    static_cast<void>(dt);
    this->m_state.omega_x = 0.12F + static_cast<float>(this->computeNoise_(ADCS_NOISE_STREAM_IDLE_X, ADCS_IDLE_OMEGA_JITTER, now));
    this->m_state.omega_y = -0.08F + static_cast<float>(this->computeNoise_(ADCS_NOISE_STREAM_IDLE_Y, ADCS_IDLE_OMEGA_JITTER, now));
    this->m_state.omega_z = 0.06F + static_cast<float>(this->computeNoise_(ADCS_NOISE_STREAM_IDLE_Z, ADCS_IDLE_OMEGA_JITTER, now));

    this->m_state.q1 = this->computeNoise_(ADCS_NOISE_STREAM_IDLE_Q1, ADCS_IDLE_QUAT_JITTER, now);
    this->m_state.q2 = this->computeNoise_(ADCS_NOISE_STREAM_IDLE_Q2, ADCS_IDLE_QUAT_JITTER, now);
    this->m_state.q3 = this->computeNoise_(ADCS_NOISE_STREAM_IDLE_Q3, ADCS_IDLE_QUAT_JITTER, now);
    this->m_state.q0 = 1.0;
    this->normalizeQuaternion_(this->m_state.q0, this->m_state.q1, this->m_state.q2, this->m_state.q3);
}

void AdcsSimModel::updateDetumbleDynamics_(double dt, const std::chrono::steady_clock::time_point& now) {
    const double decay = std::exp(-dt / ADCS_DETUMBLE_TAU_SEC);
    const double forcingBlend = 1.0 - decay;
    const double phase = 2.0 * M_PI * (this->m_pointingPassElapsedSec / ADCS_POINTING_CYCLE_SEC);
    this->m_state.omega_x = static_cast<float>(this->m_state.omega_x * decay +
                                               (0.0012 * std::sin(phase) +
                                                this->computeNoise_(ADCS_NOISE_STREAM_DETUMBLE_X, ADCS_DETUMBLE_JITTER, now)) *
                                                   forcingBlend);
    this->m_state.omega_y = static_cast<float>(this->m_state.omega_y * decay +
                                               (0.0010 * std::sin(phase + 1.0) +
                                                this->computeNoise_(ADCS_NOISE_STREAM_DETUMBLE_Y, ADCS_DETUMBLE_JITTER, now)) *
                                                   forcingBlend);
    this->m_state.omega_z = static_cast<float>(this->m_state.omega_z * decay +
                                               (0.0008 * std::sin(phase + 2.0) +
                                                this->computeNoise_(ADCS_NOISE_STREAM_DETUMBLE_Z, ADCS_DETUMBLE_JITTER, now)) *
                                                   forcingBlend);
    this->m_state.q1 += this->computeNoise_(ADCS_NOISE_STREAM_DETUMBLE_Q1, ADCS_IDLE_QUAT_JITTER * 0.5, now) *
                        forcingBlend;
    this->m_state.q2 += this->computeNoise_(ADCS_NOISE_STREAM_DETUMBLE_Q2, ADCS_IDLE_QUAT_JITTER * 0.5, now) *
                        forcingBlend;
    this->m_state.q3 += this->computeNoise_(ADCS_NOISE_STREAM_DETUMBLE_Q3, ADCS_IDLE_QUAT_JITTER * 0.5, now) *
                        forcingBlend;
    this->normalizeQuaternion_(this->m_state.q0, this->m_state.q1, this->m_state.q2, this->m_state.q3);
    this->m_pointingPassElapsedSec += dt;
}

void AdcsSimModel::updatePointingDynamics_(double dt, const std::chrono::steady_clock::time_point& now) {
    static_cast<void>(now);
    this->m_pointingPassElapsedSec += dt;

    double desiredQ0 = 1.0;
    double desiredQ1 = 0.0;
    double desiredQ2 = 0.0;
    double desiredQ3 = 0.0;
    this->syntheticDesiredQuaternion_(desiredQ0, desiredQ1, desiredQ2, desiredQ3);

    const double prevQ0 = this->m_state.q0;
    const double prevQ1 = this->m_state.q1;
    const double prevQ2 = this->m_state.q2;
    const double prevQ3 = this->m_state.q3;

    const double alpha = 1.0 - std::exp(-dt / ADCS_POINTING_TRACK_TAU_SEC);
    this->m_state.q0 = this->m_state.q0 + alpha * (desiredQ0 - this->m_state.q0);
    this->m_state.q1 = this->m_state.q1 + alpha * (desiredQ1 - this->m_state.q1);
    this->m_state.q2 = this->m_state.q2 + alpha * (desiredQ2 - this->m_state.q2);
    this->m_state.q3 = this->m_state.q3 + alpha * (desiredQ3 - this->m_state.q3);
    this->normalizeQuaternion_(this->m_state.q0, this->m_state.q1, this->m_state.q2, this->m_state.q3);

    this->angularVelocityFromQuaternionDelta_(
        prevQ0, prevQ1, prevQ2, prevQ3, this->m_state.q0, this->m_state.q1, this->m_state.q2, this->m_state.q3, dt);

    if (!this->m_externalTargetActive) {
        this->m_targetQ0 = desiredQ0;
        this->m_targetQ1 = desiredQ1;
        this->m_targetQ2 = desiredQ2;
        this->m_targetQ3 = desiredQ3;
    }
}

void AdcsSimModel::effectiveTargetQuaternion_(double& q0, double& q1, double& q2, double& q3) const {
    q0 = this->m_targetQ0;
    q1 = this->m_targetQ1;
    q2 = this->m_targetQ2;
    q3 = this->m_targetQ3;
}

void AdcsSimModel::syntheticDesiredQuaternion_(double& q0, double& q1, double& q2, double& q3) const {
    const double phase = std::fmod(this->m_pointingPassElapsedSec, ADCS_POINTING_CYCLE_SEC) / ADCS_POINTING_CYCLE_SEC;
    const double yawDeg = -35.0 + (70.0 * phase);
    const double pitchDeg = 18.0 + (32.0 * std::sin(M_PI * phase));
    const double rollDeg = 2.0 * std::sin(2.0 * M_PI * phase);
    this->eulerToQuaternion_(
        rollDeg * M_PI / 180.0, pitchDeg * M_PI / 180.0, yawDeg * M_PI / 180.0, q0, q1, q2, q3);
}

void AdcsSimModel::eulerToQuaternion_(double rollRad,
                                      double pitchRad,
                                      double yawRad,
                                      double& q0,
                                      double& q1,
                                      double& q2,
                                      double& q3) const {
    const double cr = std::cos(rollRad * 0.5);
    const double sr = std::sin(rollRad * 0.5);
    const double cp = std::cos(pitchRad * 0.5);
    const double sp = std::sin(pitchRad * 0.5);
    const double cy = std::cos(yawRad * 0.5);
    const double sy = std::sin(yawRad * 0.5);

    q0 = cr * cp * cy + sr * sp * sy;
    q1 = sr * cp * cy - cr * sp * sy;
    q2 = cr * sp * cy + sr * cp * sy;
    q3 = cr * cp * sy - sr * sp * cy;
    this->normalizeQuaternion_(q0, q1, q2, q3);
}

void AdcsSimModel::angularVelocityFromQuaternionDelta_(double prevQ0,
                                                       double prevQ1,
                                                       double prevQ2,
                                                       double prevQ3,
                                                       double nextQ0,
                                                       double nextQ1,
                                                       double nextQ2,
                                                       double nextQ3,
                                                       double dt) {
    if (dt <= 0.0) {
        this->m_state.omega_x = 0.0F;
        this->m_state.omega_y = 0.0F;
        this->m_state.omega_z = 0.0F;
        return;
    }

    const double dq0 = prevQ0 * nextQ0 + prevQ1 * nextQ1 + prevQ2 * nextQ2 + prevQ3 * nextQ3;
    const double dq1 = prevQ0 * nextQ1 - prevQ1 * nextQ0 - prevQ2 * nextQ3 + prevQ3 * nextQ2;
    const double dq2 = prevQ0 * nextQ2 + prevQ1 * nextQ3 - prevQ2 * nextQ0 - prevQ3 * nextQ1;
    const double dq3 = prevQ0 * nextQ3 - prevQ1 * nextQ2 + prevQ2 * nextQ1 - prevQ3 * nextQ0;
    const double angle = 2.0 * std::acos(std::max(-1.0, std::min(1.0, std::fabs(dq0))));
    const double sinHalf = std::sqrt(std::max(0.0, 1.0 - (dq0 * dq0)));

    double axisX = 0.0;
    double axisY = 0.0;
    double axisZ = 0.0;
    if (sinHalf > 1e-9) {
        axisX = dq1 / sinHalf;
        axisY = dq2 / sinHalf;
        axisZ = dq3 / sinHalf;
    }

    double omegaX = clampSigned(axisX * angle / dt, 0.03);
    double omegaY = clampSigned(axisY * angle / dt, 0.03);
    double omegaZ = clampSigned(axisZ * angle / dt, 0.03);
    const double omegaNorm = std::sqrt(omegaX * omegaX + omegaY * omegaY + omegaZ * omegaZ);
    if (omegaNorm > ADCS_POINTING_MAX_RATE_NORM && omegaNorm > 1e-9) {
        const double scale = ADCS_POINTING_MAX_RATE_NORM / omegaNorm;
        omegaX *= scale;
        omegaY *= scale;
        omegaZ *= scale;
    }

    this->m_state.omega_x = static_cast<float>(omegaX);
    this->m_state.omega_y = static_cast<float>(omegaY);
    this->m_state.omega_z = static_cast<float>(omegaZ);
}

void AdcsSimModel::normalizeQuaternion_(double& q0, double& q1, double& q2, double& q3) const {
    const double norm = std::sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    if (norm == 0.0) {
        q0 = 1.0;
        q1 = 0.0;
        q2 = 0.0;
        q3 = 0.0;
        return;
    }

    q0 /= norm;
    q1 /= norm;
    q2 /= norm;
    q3 /= norm;
}

float AdcsSimModel::pointingErrorDeg_() const {
    double targetQ0 = 1.0;
    double targetQ1 = 0.0;
    double targetQ2 = 0.0;
    double targetQ3 = 0.0;
    this->effectiveTargetQuaternion_(targetQ0, targetQ1, targetQ2, targetQ3);
    double dot = this->m_state.q0 * targetQ0 + this->m_state.q1 * targetQ1 + this->m_state.q2 * targetQ2 +
                 this->m_state.q3 * targetQ3;
    dot = std::max(-1.0, std::min(1.0, dot));
    const double angle = 2.0 * std::acos(std::fabs(dot));
    return static_cast<float>(angle * 180.0 / M_PI);
}

}  // namespace ADCS
}  // namespace OBC
