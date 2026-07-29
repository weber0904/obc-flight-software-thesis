#ifndef OBC_SIMULATORS_SCENARIO_SCENARIOBRIDGE_HPP
#define OBC_SIMULATORS_SCENARIO_SCENARIOBRIDGE_HPP

#include <string>

#include "simulators/adcs/AdcsSimModel.hpp"
#include "simulators/eps/EpsSimModel.hpp"
#include "simulators/scenario/ScenarioTimeline.hpp"

namespace OBC {
namespace Scenario {

class ScenarioBridge {
  public:
    ScenarioBridge(EPS::EpsSimModel& epsModel, ADCS::AdcsSimModel& adcsModel, const ScenarioTimeline& timeline);

    bool initialize(std::string& errorMessage);

    bool stepTo(double timeSec, std::string& errorMessage);

    bool isInitialized() const;

    const ReplaySample& currentSample() const;

  private:
    void applyEpsReplay_(const ReplaySample& sample);

    void seedAdcsReplay_(const ReplaySample& sample);

  private:
    EPS::EpsSimModel& m_epsModel;
    ADCS::AdcsSimModel& m_adcsModel;
    const ScenarioTimeline& m_timeline;
    ReplaySample m_currentSample;
    bool m_initialized;
    bool m_adcsSeeded;
};

}  // namespace Scenario
}  // namespace OBC

#endif
