#ifndef OBC_SIMULATORS_SCENARIO_SCENARIOTIMELINE_HPP
#define OBC_SIMULATORS_SCENARIO_SCENARIOTIMELINE_HPP

#include <istream>
#include <string>
#include <vector>

#include "simulators/scenario/ScenarioTypes.hpp"

namespace OBC {
namespace Scenario {

class ScenarioTimeline {
  public:
    bool loadFromCsvFile(const std::string& path, std::string& errorMessage);

    bool loadFromCsvStream(std::istream& input, std::string& errorMessage);

    bool empty() const;

    const ReplaySample& firstSample() const;

    ReplaySample sampleAt(double timeSec) const;

    const std::vector<ReplaySample>& samples() const;

  private:
    std::vector<ReplaySample> m_samples;
};

}  // namespace Scenario
}  // namespace OBC

#endif
