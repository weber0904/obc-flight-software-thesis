#include "OBC/Components/EpsBridge/EpsBridge.hpp"
#include "OBC/Components/BootManager/BootManager.hpp"
#include "OBC/Components/EpsFdirController/EpsFdirController.hpp"
#include "OBC/Components/ModeManager/ModeManager.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryExecutor.hpp"

#include <deque>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "Os/FileSystem.hpp"
#include "Fw/Tlm/TlmPortAc.hpp"

namespace {

class NullTlmSink final : public Fw::PassiveComponentBase {
  public:
    NullTlmSink() : Fw::PassiveComponentBase("NullTlmSink") {
        this->m_input.init();
        this->m_input.addCallComp(this, &NullTlmSink::handle);
        this->m_input.setPortNum(0);
    }

    Fw::InputTlmPort* getPort() { return &this->m_input; }

  private:
    static void handle(Fw::PassiveComponentBase* callComp,
                       FwIndexType portNum,
                       FwChanIdType id,
                       Fw::Time& timeTag,
                       Fw::TlmBuffer& val) {
        static_cast<void>(callComp);
        static_cast<void>(portNum);
        static_cast<void>(id);
        static_cast<void>(timeTag);
        static_cast<void>(val);
    }

    Fw::InputTlmPort m_input;
};

class FakeEpsTransport final : public OBC::EPS::IEpsTransport {
  public:
    struct Reply {
        OBC::EPS::TransportStatus status;
        OBC::EPS::StatusData data;
    };

    void pushStatusReply(OBC::EPS::TransportStatus status, const OBC::EPS::StatusData& data) {
        this->m_statusReplies.push_back({status, data});
    }

    OBC::EPS::TransportStatus getStatus(OBC::EPS::StatusData& outStatus) override {
        if (this->m_statusReplies.empty()) {
            return OBC::EPS::TransportStatus::TRANSPORT_ERROR;
        }
        const Reply reply = this->m_statusReplies.front();
        this->m_statusReplies.pop_front();
        outStatus = reply.data;
        return reply.status;
    }

    OBC::EPS::TransportStatus setPdu(std::uint8_t, bool, OBC::EPS::StatusData& outStatus) override {
        outStatus = {};
        return OBC::EPS::TransportStatus::TRANSPORT_ERROR;
    }

    OBC::EPS::TransportStatus setHeater(bool, OBC::EPS::StatusData& outStatus) override {
        outStatus = {};
        return OBC::EPS::TransportStatus::TRANSPORT_ERROR;
    }

    OBC::EPS::TransportStatus reset(OBC::EPS::StatusData& outStatus) override {
        outStatus = this->lastResetStatus;
        this->resetCalls++;
        return OBC::EPS::TransportStatus::OK;
    }

    OBC::EPS::StatusData lastResetStatus = {};
    U32 resetCalls = 0U;

  private:
    std::deque<Reply> m_statusReplies;
};

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

OBC::EPS::StatusData makeStatus(float soc) {
    OBC::EPS::StatusData status = {};
    status.soc = soc;
    status.vbat = 8.0F;
    status.temp_bat = 25.0F;
    status.pdu_status = 0x03U;
    return status;
}

}  // namespace

int main() {
    OBC::ModeManager modeManager("ModeManagerIntegration");
    OBC::EpsBridge epsBridge("EpsBridgeIntegration");
    OBC::EpsFdirController controller("EpsFdirIntegration");
    OBC::BootManager bootManager("RecoveryBootIntegration");
    OBC::RecoveryExecutor recoveryExecutor("RecoveryExecutorIntegration");
    FakeEpsTransport transport;
    NullTlmSink tlmSink;
    char bootRootTemplate[] = "/tmp/eps-fdir-integration-XXXXXX";
    const char* const bootRoot = ::mkdtemp(bootRootTemplate);

    modeManager.init(0);
    epsBridge.init(0);
    controller.init(0);
    bootManager.init(0);
    recoveryExecutor.init(0);
    epsBridge.set_epsStatusRefreshTlmOut_OutputPort(0, tlmSink.getPort());
    epsBridge.setTransportForTest(&transport);
    if (bootRoot != nullptr) {
        bootManager.configureStorageRootForTest(std::string(bootRoot) + "/persistent-data/boot");
    }
    recoveryExecutor.configureRuntime(&modeManager, &epsBridge, nullptr, &bootManager, nullptr);
    controller.configureRuntime(&modeManager, &epsBridge, &recoveryExecutor);

    bool ok = true;

    modeManager.applyModeForInternalSource(OBC::SatMode::IDLE, OBC::ModeApplySource::TestSetup);
    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, {});
    ok = check(!epsBridge.pollStatusForTest(), "first failure should fail poll") && ok;
    OBC::EpsFdirDecision decision = controller.runCycle();
    ok = check(decision.shouldEnterRetry, "first failure should keep retrying") && ok;
    ok = check(modeManager.getModeForRuntime() == OBC::SatMode::IDLE, "first failure should not change mode") && ok;

    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, {});
    ok = check(!epsBridge.pollStatusForTest(), "second failure should fail poll") && ok;
    decision = controller.runCycle();
    ok = check(decision.shouldEnterRetry, "second failure should still retry") && ok;
    ok = check(modeManager.getModeForRuntime() == OBC::SatMode::IDLE, "second failure should not change mode") && ok;

    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, {});
    ok = check(!epsBridge.pollStatusForTest(), "third failure should fail poll") && ok;
    decision = controller.runCycle();
    ok = check(decision.shouldLatchFault, "third failure should latch fault") && ok;
    ok = check(modeManager.getModeForRuntime() == OBC::SatMode::SAFE,
               "third failure should reach SAFE via recovery executor") && ok;
    ok = check(transport.resetCalls == 1U, "third failure should issue EPS reset via shared recovery executor") && ok;

    transport.pushStatusReply(OBC::EPS::TransportStatus::OK, makeStatus(72.0F));
    ok = check(epsBridge.pollStatusForTest(), "recovery poll should succeed") && ok;
    decision = controller.runCycle();
    ok = check(decision.shouldClearFault, "first success after fault should clear fault") && ok;
    ok = check(modeManager.getModeForRuntime() == OBC::SatMode::SAFE, "recovery should not auto-leave SAFE") && ok;

    modeManager.applyModeForInternalSource(OBC::SatMode::IDLE, OBC::ModeApplySource::TestSetup);
    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, {});
    ok = check(!epsBridge.pollStatusForTest(), "relatch failure should fail poll") && ok;
    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, {});
    ok = check(!epsBridge.pollStatusForTest(), "relatch failure should fail poll twice") && ok;
    transport.pushStatusReply(OBC::EPS::TransportStatus::TIMEOUT, {});
    ok = check(!epsBridge.pollStatusForTest(), "relatch threshold failure should fail poll") && ok;
    decision = controller.runCycle();
    ok = check(decision.shouldLatchFault, "relatch should latch fault again") && ok;
    ok = check(recoveryExecutor.consumeRebootRequestForRuntime(),
               "second EPS fault should escalate to reboot via shared recovery executor") && ok;

    if (bootRoot != nullptr) {
        (void)Os::FileSystem::removeFile((std::string(bootRoot) + "/persistent-data/boot/metadata-v1.txt").c_str());
        (void)Os::FileSystem::removeDirectory((std::string(bootRoot) + "/persistent-data/boot").c_str());
        (void)Os::FileSystem::removeDirectory((std::string(bootRoot) + "/persistent-data").c_str());
        (void)Os::FileSystem::removeDirectory(bootRoot);
    }

    return ok ? 0 : 1;
}
