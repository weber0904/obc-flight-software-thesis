#ifndef OBC_SIMULATORS_EPS_EPSSIMMODEL_HPP
#define OBC_SIMULATORS_EPS_EPSSIMMODEL_HPP

#include <chrono>

#include "simulators/eps/EpsCspProtocol.hpp"

namespace OBC {
namespace EPS {

enum class LoadMode : std::uint8_t {
    NORMAL = 0U,
    HIGH_DRAW = 1U,
};

class EpsSimModel {
  public:
    EpsSimModel();

    ResultCode processCspRequest(const CSP::Request& request, CSP::Reply& response);

    void applyScenarioState(std::uint8_t sunlight, float soc);

    void setSocForRuntime(float soc, float transitionSec);

    void setLoadModeForRuntime(LoadMode mode);

    LoadMode getLoadModeForRuntime() const;

    void setDroppedStatusReplyCount(std::uint32_t count);

    bool consumeDroppedStatusReply(CSP::ServicePort service);

    std::uint32_t getDroppedStatusReplyCount() const;

    void reset();

    StatusData snapshotStateForRuntime();

    void advanceForTest(std::chrono::milliseconds delta);

    const StatusData& getState() const;

  private:
    struct SocRampState {
        bool active = false;
        float startSoc = 0.0F;
        float targetSoc = 0.0F;
        std::chrono::steady_clock::time_point startTime = {};
        std::chrono::steady_clock::time_point endTime = {};
    };

    struct DerivedState {
        float loadCurrent = 0.0F;
        float solarCurrent = 0.0F;
        float vsolar = 0.0F;
        float vbat = 0.0F;
        float tempBat = 0.0F;
    };

    static float clampSoc_(float soc);

    static float clampUnit_(float value);

    static float loadWeightForChannel_(std::uint8_t channel);

    void loadDefaults_();

    void refreshSocRamp_(const std::chrono::steady_clock::time_point& now);

    void refreshDynamics_();

    void refreshDynamicsAt_(const std::chrono::steady_clock::time_point& now);

    std::chrono::steady_clock::time_point now_() const;

    float computeNoise_(std::uint64_t streamId, float amplitude, const std::chrono::steady_clock::time_point& now) const;

    DerivedState computeDerivedState_(const std::chrono::steady_clock::time_point& now) const;

    void updateDerivedState_();

    StatusData m_state;
    SocRampState m_socRamp;
    LoadMode m_loadMode;
    std::chrono::steady_clock::time_point m_referenceTime;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    bool m_manualTimeActive;
    std::chrono::steady_clock::time_point m_manualNow;
    std::uint32_t m_droppedStatusReplyCount = 0U;
};

}  // namespace EPS
}  // namespace OBC

#endif
