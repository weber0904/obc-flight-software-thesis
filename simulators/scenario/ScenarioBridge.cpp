#include "simulators/scenario/ScenarioBridge.hpp"

namespace OBC {
namespace Scenario {

ScenarioBridge::ScenarioBridge(EPS::EpsSimModel& epsModel,
                               ADCS::AdcsSimModel& adcsModel,
                               const ScenarioTimeline& timeline)
    : m_epsModel(epsModel), m_adcsModel(adcsModel), m_timeline(timeline), m_currentSample(), m_initialized(false),
      m_adcsSeeded(false) {}

bool ScenarioBridge::initialize(std::string& errorMessage) {
    if (this->m_timeline.empty()) {
        errorMessage = "Scenario bridge cannot initialize from an empty timeline";
        return false;
    }

    this->m_currentSample = this->m_timeline.firstSample();
    this->applyEpsReplay_(this->m_currentSample);
    if (!this->m_adcsSeeded) {
        this->seedAdcsReplay_(this->m_currentSample);
        this->m_adcsSeeded = true;
    }

    this->m_initialized = true;
    return true;
}

bool ScenarioBridge::stepTo(double timeSec, std::string& errorMessage) {
    if (!this->m_initialized && !this->initialize(errorMessage)) {
        return false;
    }

    this->m_currentSample = this->m_timeline.sampleAt(timeSec);
    this->applyEpsReplay_(this->m_currentSample);
    return true;
}

bool ScenarioBridge::isInitialized() const {
    return this->m_initialized;
}

const ReplaySample& ScenarioBridge::currentSample() const {
    return this->m_currentSample;
}

void ScenarioBridge::applyEpsReplay_(const ReplaySample& sample) {
    this->m_epsModel.applyScenarioState(sample.sunlight, sample.battery_soc_pct);
}

void ScenarioBridge::seedAdcsReplay_(const ReplaySample& sample) {
    this->m_adcsModel.seedScenarioAngularRate(sample.omega_x_rad_s, sample.omega_y_rad_s, sample.omega_z_rad_s);
}

}  // namespace Scenario
}  // namespace OBC
