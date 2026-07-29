#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "OBC/Components/GpsBridge/GpsBridge.hpp"
#include "simulators/gps/GpsSource.hpp"

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

class ScriptedGpsSource final : public OBC::GPS::IGpsSentenceSource {
  public:
    explicit ScriptedGpsSource(std::vector<std::string> sentences)
        : m_sentences(std::move(sentences)), m_index(0U) {}

    bool nextSentence(std::string& sentence) override {
        if (this->m_index >= this->m_sentences.size()) {
            return false;
        }
        sentence = this->m_sentences[this->m_index++];
        return true;
    }

    void reset() override {
        this->m_index = 0U;
    }

  private:
    std::vector<std::string> m_sentences;
    std::size_t m_index;
};

bool testCachedStateTracksValidNoFixAndRejectedSentences() {
    bool ok = true;

    OBC::GpsBridge bridge("gpsBridgeTest");
    bridge.setSentenceSourceForTest(
        std::unique_ptr<OBC::GPS::IGpsSentenceSource>(new ScriptedGpsSource({
            "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
            "$GPGGA,123520,,,,,0,00,99.99,,,,,,*4F",
            "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*00",
        })),
        OBC::GpsSourceMode::FAKE);

    OBC::GPS::StateData state = {};

    ok = check(bridge.pollStateForTest(), "Expected first GPS poll to succeed") && ok;
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached GPS state after valid sample") && ok;
    ok = check(state.hasSample, "Expected cached GPS sample after valid sentence") && ok;
    ok = check(state.fixValid, "Expected valid GPS fix after valid sentence") && ok;
    ok = check(state.satelliteCount == 8U, "Expected satellite count from valid GGA") && ok;
    ok = check(state.altitudeMeters > 545.0F && state.altitudeMeters < 546.0F, "Expected altitude from valid GGA") &&
         ok;
    ok = check(state.acceptedSentenceCount == 1U, "Expected first accepted sentence count") && ok;
    ok = check(state.rejectedSentenceCount == 0U, "Expected zero rejected sentences after valid sample") && ok;
    ok = check(state.sourceMode == OBC::GPS::SourceMode::FAKE, "Expected fake source mode in cached state") && ok;

    ok = check(bridge.pollStateForTest(), "Expected no-fix GPS poll to succeed") && ok;
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached GPS state after no-fix sample") && ok;
    ok = check(!state.fixValid, "Expected no-fix sample to clear valid-fix state") && ok;
    ok = check(state.satelliteCount == 0U, "Expected no-fix sample to report zero satellites") && ok;
    ok = check(state.acceptedSentenceCount == 2U, "Expected second accepted sentence count") && ok;
    ok = check(state.rejectedSentenceCount == 0U, "Expected rejected count to remain zero after no-fix sample") &&
         ok;

    ok = check(!bridge.pollStateForTest(), "Expected invalid checksum sentence to fail poll") && ok;
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached GPS state to remain readable after reject") &&
         ok;
    ok = check(!state.fixValid, "Expected rejected sentence not to restore valid fix") && ok;
    ok = check(state.acceptedSentenceCount == 2U, "Expected accepted count unchanged after rejected sentence") && ok;
    ok = check(state.rejectedSentenceCount == 1U, "Expected rejected sentence count after invalid checksum") && ok;

    return ok;
}

}  // namespace

int main() {
    return testCachedStateTracksValidNoFixAndRejectedSentences() ? 0 : 1;
}
