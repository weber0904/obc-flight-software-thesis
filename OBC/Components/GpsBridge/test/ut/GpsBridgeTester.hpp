#ifndef OBC_GpsBridgeTester_HPP
#define OBC_GpsBridgeTester_HPP

#include <memory>
#include <string>
#include <vector>

#include "OBC/Components/GpsBridge/GpsBridge.hpp"
#include "OBC/Components/GpsBridge/GpsBridgeGTestBase.hpp"
#include "simulators/gps/GpsSource.hpp"

namespace OBC {

class GpsBridgeTester final : public GpsBridgeGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 32;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    GpsBridgeTester();

    ~GpsBridgeTester() override;

    void testGetStatePublishesValidFix();

    void testGetStateReplaysTelemetryWhenValuesUnchanged();

    void testNoFixClearsLatchedFixViaScheduler();

    void testMalformedSentenceReportsParseError();

    void testReplayModeRejectsWhenUnavailable();

    void testLiveUartModeRejectsWhenUnavailable();

    void testLiveUartModeFallsBackWhenBaudrateEnvOverflows();

    void testLiveUartModePublishesTelemetryWithTestSource();

    void testSourceDepletionReportsSourceError();

  private:
    void connectPorts();

    void initComponents();

    void setScriptedSource_(std::vector<std::string> sentences, OBC::GpsSourceMode mode = OBC::GpsSourceMode::FAKE);

  private:
    OBC::GpsBridge component;
};

}  // namespace OBC

#endif
