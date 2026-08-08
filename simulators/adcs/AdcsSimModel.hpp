#ifndef OBC_SIMULATORS_ADCS_ADCSSIMMODEL_HPP
#define OBC_SIMULATORS_ADCS_ADCSSIMMODEL_HPP

#include <chrono>

#include "simulators/adcs/AdcsCspProtocol.hpp"

namespace OBC {
namespace ADCS {

class AdcsSimModel {
  public:
    AdcsSimModel();

    ResultCode processCspRequest(const CSP::Request& request, CSP::Reply& response);

    void setDroppedStateReplyCount(std::uint32_t count);

    std::uint32_t getDroppedStateReplyCount() const;

    bool consumeDroppedStateReply(const CSP::ServicePort service);

    void seedScenarioAngularRate(float omegaX, float omegaY, float omegaZ);

    void restartPointingPassForRuntime();

    void advanceForTest(std::chrono::milliseconds delta);

    void reset();

    const StateData& getState() const;

  private:
    void loadDefaults_();

    void refreshDerivedState_();

    void stepDynamics_();

    void stepDynamicsAt_(const std::chrono::steady_clock::time_point& now);

    std::chrono::steady_clock::time_point now_() const;

    double computeNoise_(std::uint64_t streamId, double amplitude, const std::chrono::steady_clock::time_point& now) const;

    void updateIdleDynamics_(double dt, const std::chrono::steady_clock::time_point& now);

    void updateDetumbleDynamics_(double dt, const std::chrono::steady_clock::time_point& now);

    void updatePointingDynamics_(double dt, const std::chrono::steady_clock::time_point& now);

    void effectiveTargetQuaternion_(double& q0, double& q1, double& q2, double& q3) const;

    void syntheticDesiredQuaternion_(double& q0, double& q1, double& q2, double& q3) const;

    void eulerToQuaternion_(double rollRad, double pitchRad, double yawRad, double& q0, double& q1, double& q2, double& q3) const;

    void angularVelocityFromQuaternionDelta_(double prevQ0,
                                             double prevQ1,
                                             double prevQ2,
                                             double prevQ3,
                                             double nextQ0,
                                             double nextQ1,
                                             double nextQ2,
                                             double nextQ3,
                                             double dt);

    void normalizeQuaternion_(double& q0, double& q1, double& q2, double& q3) const;

    float pointingErrorDeg_() const;

    StateData m_state;
    double m_targetQ0;
    double m_targetQ1;
    double m_targetQ2;
    double m_targetQ3;
    bool m_externalTargetActive;
    double m_pointingPassElapsedSec;
    std::chrono::steady_clock::time_point m_referenceTime;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    bool m_manualTimeActive;
    std::chrono::steady_clock::time_point m_manualNow;
    std::uint32_t m_droppedStateReplyCount;
};

}  // namespace ADCS
}  // namespace OBC

#endif
