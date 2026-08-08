#include "OBC/Components/BootManager/BootManager.hpp"
#include "OBC/Components/CommController/CommController.hpp"
#include "OBC/Components/CspBridge/CspBridge.hpp"
#include "OBC/Components/ModeManager/ModeManager.hpp"
#include "OBC/Components/UartDriver/UartDriver.hpp"
#include "OBC/TopCcsds/OnboardStateSnapshotSource.hpp"

#include <iostream>
#include <string>

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

class FakeCspRuntime final : public OBC::CSP::ICspRuntime {
  public:
    OBC::CSP::RuntimeStatus init(const OBC::CSP::RuntimeConfig& config) override {
        this->m_metrics.initialized = true;
        this->m_metrics.localNodeId = config.nodeId;
        this->m_metrics.freeBuffers = 12U;
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus ping(std::uint16_t targetNode, std::uint32_t timeoutMs, bool& success) override {
        static_cast<void>(targetNode);
        static_cast<void>(timeoutMs);
        success = true;
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus sendRaw(std::uint16_t targetNode,
                                    std::uint8_t targetPort,
                                    const std::string& data) override {
        static_cast<void>(targetNode);
        static_cast<void>(targetPort);
        static_cast<void>(data);
        this->m_metrics.txPackets++;
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus requestReply(std::uint16_t targetNode,
                                         std::uint8_t targetPort,
                                         const void* requestData,
                                         std::size_t requestSize,
                                         void* replyData,
                                         std::size_t replyCapacity,
                                         std::size_t& replySize,
                                         std::uint32_t timeoutMs) override {
        static_cast<void>(targetNode);
        static_cast<void>(targetPort);
        static_cast<void>(requestData);
        static_cast<void>(requestSize);
        static_cast<void>(replyData);
        static_cast<void>(replyCapacity);
        static_cast<void>(timeoutMs);
        replySize = 0U;
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    }

    OBC::CSP::RuntimeMetrics metrics() const override {
        return this->m_metrics;
    }

    void shutdown() override {
        this->m_metrics.initialized = false;
    }

  private:
    OBC::CSP::RuntimeMetrics m_metrics = {};
};

bool testSourceRequiresCoreProviders() {
    OBCApp::OnboardStateSnapshotSource source;
    OBC::StateData::StateSnapshot snapshot = {};
    return check(!source.readStateSnapshot(snapshot), "snapshot source should fail before core providers are configured");
}

bool testSourceAggregatesCoreCachedState() {
    OBC::ModeManager modeManager("ModeManager");
    OBC::CommController commController("CommController");
    FakeCspRuntime cspRuntime;
    OBC::CspBridge cspBridge("CspBridge", cspRuntime);
    OBC::UartDriver uartDriver("UartDriver");
    OBC::BootManager bootManager("BootManager");
    OBCApp::OnboardStateSnapshotSource source;

    modeManager.applyModeForInternalSource(OBC::SatMode::IDLE, OBC::ModeApplySource::TestSetup);
    modeManager.setUptimeForTest(42U);
    static_cast<void>(commController.startPassForRuntime(90U));
    static_cast<void>(cspBridge.initForRuntime(5U));
    static_cast<void>(cspBridge.sendRawForRuntime(2U, 10U, "ping"));

    source.configure(&modeManager,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     &commController,
                     &cspBridge,
                     nullptr,
                     &uartDriver,
                     &bootManager);

    OBC::StateData::StateSnapshot snapshot = {};
    bool ok = check(source.readStateSnapshot(snapshot), "configured snapshot source should read cached state");
    ok = check(snapshot.mode == OBC::SatMode::IDLE, "snapshot should include mode manager state") && ok;
    ok = check(snapshot.uptimeSec == 42U, "snapshot should include uptime") && ok;
    ok = check(snapshot.commActiveBand == OBC::CommBand::SBAND, "snapshot should include COMM active band") && ok;
    ok = check(snapshot.commPassActive, "snapshot should include active pass state") && ok;
    ok = check(snapshot.commPassRemainingSec == 90U, "snapshot should include pass remaining seconds") && ok;
    ok = check(snapshot.cspInitialized, "snapshot should include CSP initialized state") && ok;
    ok = check(snapshot.cspLocalNodeId == 5U, "snapshot should include CSP local node id") && ok;
    ok = check(snapshot.cspFreeBuffers == 12U, "snapshot should include CSP free buffers") && ok;
    ok = check(snapshot.cspTxPackets == 1U, "snapshot should include CSP packet counters") && ok;
    ok = check(!snapshot.haveEpsStatus, "optional EPS cache should remain marked missing") && ok;
    return ok;
}

bool testSourcePreservesProvidedTimestamp() {
    OBC::ModeManager modeManager("ModeManager");
    OBC::CommController commController("CommController");
    FakeCspRuntime cspRuntime;
    OBC::CspBridge cspBridge("CspBridge", cspRuntime);
    OBC::UartDriver uartDriver("UartDriver");
    OBC::BootManager bootManager("BootManager");
    OBCApp::OnboardStateSnapshotSource source;

    source.configure(&modeManager,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     &commController,
                     &cspBridge,
                     nullptr,
                     &uartDriver,
                     &bootManager);

    OBC::StateData::StateSnapshot snapshot = {};
    const Fw::Time highResolutionTime(TimeBase::TB_SC_TIME, 7U, 123U, 456U);
    snapshot.timestamp = highResolutionTime;
    bool ok = check(source.readStateSnapshot(snapshot), "configured snapshot source should read cached state");
    ok = check(snapshot.timestamp == highResolutionTime, "snapshot source should preserve caller-provided timestamp") && ok;
    return ok;
}

bool testSourceLeavesMissingTimestampUnset() {
    OBC::ModeManager modeManager("ModeManager");
    OBC::CommController commController("CommController");
    FakeCspRuntime cspRuntime;
    OBC::CspBridge cspBridge("CspBridge", cspRuntime);
    OBC::UartDriver uartDriver("UartDriver");
    OBC::BootManager bootManager("BootManager");
    OBCApp::OnboardStateSnapshotSource source;

    source.configure(&modeManager,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     &commController,
                     &cspBridge,
                     nullptr,
                     &uartDriver,
                     &bootManager);

    OBC::StateData::StateSnapshot snapshot = {};
    bool ok = check(source.readStateSnapshot(snapshot), "configured snapshot source should read cached state");
    ok = check(snapshot.timestamp == Fw::ZERO_TIME,
               "snapshot source should leave missing timestamp for the caller to own") &&
         ok;
    return ok;
}

}  // namespace

int main() {
    bool ok = true;
    ok = testSourceRequiresCoreProviders() && ok;
    ok = testSourceAggregatesCoreCachedState() && ok;
    ok = testSourcePreservesProvidedTimestamp() && ok;
    ok = testSourceLeavesMissingTimestampUnset() && ok;
    if (!ok) {
        return 1;
    }
    std::cout << "onboard_state_snapshot_source_unit_test: PASS\n";
    return 0;
}
