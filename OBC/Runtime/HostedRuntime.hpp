#ifndef OBC_Runtime_HostedRuntime_HPP
#define OBC_Runtime_HostedRuntime_HPP

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "Fw/Time/TimeInterval.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"
#include "OBC/Components/AdcsBridge/AdcsBridge.hpp"
#include "OBC/Components/BootManager/BootManager.hpp"
#include "OBC/Components/CommController/CommController.hpp"
#include "OBC/Components/CspBridge/CspBridge.hpp"
#include "OBC/Components/EpsBridge/EpsBridge.hpp"
#include "OBC/Components/GpsBridge/GpsBridge.hpp"
#include "OBC/Components/GroundLinkDriver/GroundLinkDriver.hpp"
#include "OBC/Components/LinuxWatchdogSink/LinuxWatchdogRuntime.hpp"
#include "OBC/Components/ModeManager/ModeManager.hpp"
#include "OBC/Components/PersistentFaultManager/PersistentFaultRuntime.hpp"
#include "OBC/Components/RadioController/RadioController.hpp"
#include "OBC/Components/RadioController/RadioControllerRuntime.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryRuntime.hpp"
#include "OBC/Components/StorageHealthBridge/StorageHealthBridge.hpp"
#include "OBC/Components/TtcPassManager/TtcPassRuntime.hpp"
#include "OBC/Components/UartDriver/UartDriver.hpp"
#include "OBC/Components/WatchdogSupervisor/WatchdogRuntime.hpp"
#include "OBC/Types/HealthItemEnumAc.hpp"
#include "simulators/comm/ByteStreamTransport.hpp"
#include "simulators/comm/RadioTransport.hpp"

namespace OBC {
namespace Runtime {

struct RuntimeConfig {
    std::string commMode = "tcp";
    std::string radioProtocol = "mock-text";
    std::string commHost = "127.0.0.1";
    std::uint16_t commPort = 7000U;
    std::uint32_t commBaudrate = 115200U;
    std::string commDevice;
    std::string groundLinkMode = "direct-tcp";
    std::string commandAuthorityProfile = "sband-primary";
    std::string bootTrustMode = "hmac-sha256";
    std::string bootTrustSignerId = "repo-dev-boot-signer";
    std::uint32_t bootTrustKeySlot = 1U;
    std::string bootTrustKeyHex = "424f4f545f54525553545f434841494e5f56315f4445565f4b4559";
    std::uint16_t commCspNode = OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID;
    std::uint16_t uhfBeaconCspNode = 0U;
    std::string gdsHost = "127.0.0.1";
    std::uint16_t gdsPort = 0U;
    std::string runtimeRoot = "runtime/dev-macos";
    std::string persistentRoot;
    std::string stagingRoot;
    std::string hardwareWatchdogMode = "disabled";
    std::string hardwareWatchdogDevice = "/dev/watchdog0";
    std::uint32_t hardwareWatchdogTimeoutSec = 15U;
    int tickMs = 1000;
    bool headless = false;
    bool diagnosticQuietPacketEgress = false;
    bool enableCommSubsystemHealthDetector = false;
    bool enablePrimaryGroundLinkDriver = true;
    OBC::CommBand initialCommBand = OBC::CommBand::SBAND;
    std::uint32_t commSubsystemPingTimeoutMs = 500U;
    std::uint32_t commPrimaryUnavailableFailureThreshold = 10U;
};

struct RuntimeState {
    bool haveEps = false;
    bool haveAdcs = false;
    bool haveGps = false;
    bool haveRadio = false;
    bool haveStorageHealth = false;
    OBC::EPS::StatusData eps = {};
    OBC::ADCS::StateData adcs = {};
    OBC::GPS::StateData gps = {};
    OBC::COMM::RadioStatus radio = {};
    OBC::RadioObservationState radioObservation = {};
    OBC::STORAGE::HealthState storage = {};
    OBC::TtcPassRuntimeStatus ttc = {};
};

struct StartupBanner {
    std::string runtimeStartedLine;
    std::vector<std::string> extraLines;
};

class RuntimeServices {
  public:
    virtual ~RuntimeServices() = default;

    virtual void announceBoot() = 0;
    virtual void setHealthEnabled(bool enabled) = 0;
    virtual Fw::CmdResponse setHealthThreshold(OBC::HealthItem item, float value) = 0;
    virtual void configureCommandIngressPersistenceRoot(const std::string& persistentRoot) = 0;
    virtual void configureBootStorageRoots(const std::string& persistentRoot, const std::string& stagingRoot) = 0;
    virtual void configurePersistentFaultStorageRoot(const std::string& persistentRoot) = 0;
    virtual void configureBootTrust(const OBC::BootTrustConfig& config) = 0;
    virtual void configureStorageRuntime(const std::string& runtimeRoot,
                                         const std::string& persistentRoot,
                                         const std::string& stagingRoot) = 0;
    virtual void initCspForRuntime(U8 nodeId) = 0;
    virtual void shutdownCspForRuntime() = 0;
    virtual void configureCommTransport(const std::shared_ptr<OBC::COMM::IByteStreamTransport>& link,
                                        std::unique_ptr<OBC::COMM::IRadioTransport> radioTransport) = 0;
    virtual bool startGroundLink() = 0;
    virtual void stopGroundLink() = 0;
    virtual void joinGroundLink() = 0;
    virtual void startRateGroups(Fw::TimeInterval interval) = 0;
    virtual void stopRateGroups() = 0;

    virtual bool refreshEpsStatus(OBC::EPS::StatusData& status) = 0;
    virtual bool getCachedEpsStatus(OBC::EPS::StatusData& status) = 0;
    virtual bool refreshAdcsState(OBC::ADCS::StateData& state) = 0;
    virtual bool getCachedAdcsState(OBC::ADCS::StateData& state) = 0;
    virtual bool getAdcsPollHealth(OBC::ADCS::PollHealthState& state) = 0;
    virtual bool getGpsCachedState(OBC::GPS::StateData& state) = 0;
    virtual bool getRadioStatus(OBC::COMM::RadioStatus& status) = 0;
    virtual OBC::RadioObservationState getRadioObservation() const = 0;
    virtual bool getStorageHealth(OBC::STORAGE::HealthState& state) = 0;
    virtual void updateResourceSample(float cpuPct, float rssMb) = 0;

    virtual OBC::SatMode getMode() const = 0;
    virtual U32 getUptime() const = 0;
    virtual U32 getRebootCount() const = 0;
    virtual OBC::CommRuntimeState getCommState() const = 0;
    virtual OBC::CspRuntimeCounters getCspCounters() const = 0;
    virtual OBC::WatchdogRuntimeSnapshot getWatchdogStatus() const = 0;
    virtual OBC::LinuxWatchdogRuntimeStatus getHardwareWatchdogStatus() const = 0;
    virtual OBC::RecoveryRuntimeStatus getRecoveryStatus() const = 0;
    virtual bool getPersistentFaultHistory(
        U32 limit, OBC::PersistentFaultHistoryStatus& status, std::vector<OBC::PersistentFaultRecord>& records) const = 0;
    virtual OBC::TtcPassRuntimeStatus getTtcPassStatus() const = 0;
    virtual const OBC::BootMetadata& getBootMetadata() const = 0;
    virtual U8 getBootUpdateProgress() const = 0;
    virtual U32 getBootRemainingConfirmSeconds() const = 0;
    virtual OBC::COMM::ByteStreamStats getUartStats() const = 0;
    virtual OBC::COMM::ByteStreamStats getRadioLinkStats() const = 0;
    virtual OBC::COMM::GroundLinkStats getGroundLinkStats() const = 0;
    virtual OBC::COMM::GroundLinkObservationState getGroundLinkObservation(OBC::CommBand band) const = 0;

    virtual Fw::CmdResponse setMode(OBC::SatMode mode) = 0;
    virtual Fw::CmdResponse pingCsp(U8 node, U32 timeoutMs, bool& success) = 0;
    virtual Fw::CmdResponse setEpsPdu(U8 channel, bool enabled, OBC::EPS::StatusData& status) = 0;
    virtual Fw::CmdResponse setEpsHeater(bool enabled, OBC::EPS::StatusData& status) = 0;
    virtual Fw::CmdResponse setAdcsMode(OBC::AdcsMode mode, OBC::ADCS::StateData& state) = 0;
    virtual bool pollGpsState() = 0;
    virtual Fw::CmdResponse setGpsSourceMode(OBC::GpsSourceMode mode) = 0;
    virtual bool scanStorageHealth() = 0;
    virtual Fw::CmdResponse setCommBand(OBC::CommBand band) = 0;
    virtual Fw::CmdResponse startCommPass(U32 durationSeconds) = 0;
    virtual Fw::CmdResponse stopCommPass() = 0;
    virtual Fw::CmdResponse setTtcPolicy(bool enabled, U32 lossOfLockTimeoutSec) = 0;
    virtual Fw::CmdResponse setTtcPassWindow(U64 startUnixSec, U64 endUnixSec) = 0;
    virtual Fw::CmdResponse clearTtcPassWindow() = 0;
    virtual Fw::CmdResponse setWatchdogConfig(OBC::WatchdogSource source,
                                              bool enabled,
                                              U32 warningTicks,
                                              U32 safeTicks,
                                              U32 suppressTicks) = 0;
    virtual bool setWatchdogProbeSuppression(OBC::WatchdogSource source, bool suppressed) = 0;
    virtual Fw::CmdResponse enableRadio(bool enabled, OBC::COMM::RadioStatus& status) = 0;
    virtual Fw::CmdResponse setRadioPower(U8 powerDbm, OBC::COMM::RadioStatus& status) = 0;
    virtual Fw::CmdResponse setRadioFrequency(U32 freqHz, OBC::COMM::RadioStatus& status) = 0;
    virtual bool exchangeUart(const std::string& request, std::string& response) = 0;
    virtual bool exchangeDelimitedUart(const std::string& request, std::string& response, char delimiter) = 0;
    virtual Fw::CmdResponse bootPrepareUpdate(U32 imageSize, const std::string& digest) = 0;
    virtual Fw::CmdResponse bootVerifyStagedImage(const std::string& path) = 0;
    virtual Fw::CmdResponse bootActivateStagedImage() = 0;
    virtual Fw::CmdResponse bootConfirm() = 0;
    virtual Fw::CmdResponse bootRollback() = 0;
    virtual OBC::RecoveryExitRequest consumeRecoveryExitRequest() = 0;
};

OBC::COMM::GroundLinkBackendMode effectiveGroundLinkMode(const RuntimeConfig& config);
OBC::COMM::GroundLinkHealthSemantics groundLinkHealthSemanticsForCommCspNode(std::uint16_t commCspNode);
Fw::TimeInterval tickIntervalFromMs(int tickMs);

void printUsage(const char* app, std::ostream& out);
bool parseArgs(int argc, char* argv[], RuntimeConfig& config, std::ostream& out, std::ostream& err);
void finalizeRuntimeRoots(RuntimeConfig& config);
bool finalizeNetworkTargets(RuntimeConfig& config, std::ostream& err);

bool parseBoolWord(const std::string& value, bool& enabled);
bool parseSatMode(const std::string& value, OBC::SatMode& mode);
bool parseAdcsMode(const std::string& value, OBC::AdcsMode& mode);
bool parseCommBand(const std::string& value, OBC::CommBand& band);
bool parseGpsSourceMode(const std::string& value, OBC::GpsSourceMode& mode);
bool parseWatchdogSource(const std::string& value, OBC::WatchdogSource& source);
bool parseHexBytes(const std::string& hex, std::string& bytes);
float currentRssMb();

void printHelp(std::ostream& out);
void refreshState(RuntimeServices& services, RuntimeState& state);
void sampleHealth(RuntimeServices& services);
void printStatus(RuntimeServices& services,
                 const RuntimeState& state,
                 const RuntimeConfig& config,
                 std::ostream& out);
bool handleCommand(const std::string& line,
                   RuntimeServices& services,
                   RuntimeState& state,
                   const RuntimeConfig& config,
                   std::ostream& out);
void configureComm(const RuntimeConfig& config, RuntimeServices& services);
int runHostedRuntime(const RuntimeConfig& config,
                     RuntimeServices& services,
                     const StartupBanner& banner,
                     std::ostream& out,
                     std::ostream& err);

}  // namespace Runtime
}  // namespace OBC

#endif
