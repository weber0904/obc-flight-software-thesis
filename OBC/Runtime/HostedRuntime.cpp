#include "OBC/Runtime/HostedRuntime.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/resource.h>
#include <thread>

#if defined(__APPLE__)
#include <mach/mach.h>
#endif

#include "OBC/Components/CommandIngressAuthority/CommandAuthKeystore.hpp"
#include "simulators/comm/CommCspProtocol.hpp"
#include "simulators/comm/TransparentLinkFraming.hpp"

namespace OBC {
namespace Runtime {
namespace {

bool parseVmRssKb(const std::string& line, float& rssMb) {
    static constexpr char PREFIX[] = "VmRSS:";
    if (line.rfind(PREFIX, 0) != 0) {
        return false;
    }

    std::istringstream stream(line.substr(sizeof(PREFIX) - 1U));
    unsigned long rssKb = 0UL;
    std::string units;
    if (!(stream >> rssKb >> units) || units != "kB") {
        return false;
    }
    rssMb = static_cast<float>(rssKb) / 1024.0F;
    return true;
}

}  // namespace

float currentRssMb() {
#if defined(__APPLE__)
    mach_task_basic_info info = {};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    const kern_return_t status =
        ::task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count);
    if (status != KERN_SUCCESS) {
        return 0.0F;
    }
    return static_cast<float>(info.resident_size) / (1024.0F * 1024.0F);
#else
    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) {
        return 0.0F;
    }

    std::string line;
    while (std::getline(statusFile, line)) {
        float rssMb = 0.0F;
        if (parseVmRssKb(line, rssMb)) {
            return rssMb;
        }
    }
    return 0.0F;
#endif
}

namespace {

std::string joinPath(const std::string& base, const char* child) {
    if (base.empty()) {
        return child == nullptr ? std::string() : std::string(child);
    }
    if (child == nullptr || child[0] == '\0') {
        return base;
    }
    if (base.back() == '/') {
        return base + child;
    }
    return base + "/" + child;
}

bool resolveIpv4Host(const std::string& input, std::string& output) {
    if (input.empty()) {
        return false;
    }

    in_addr directAddress = {};
    if (::inet_pton(AF_INET, input.c_str(), &directAddress) == 1) {
        output = input;
        return true;
    }

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (::getaddrinfo(input.c_str(), nullptr, &hints, &result) != 0 || result == nullptr) {
        if (result != nullptr) {
            ::freeaddrinfo(result);
        }
        return false;
    }

    const auto* ipv4Address = reinterpret_cast<const sockaddr_in*>(result->ai_addr);
    char buffer[INET_ADDRSTRLEN] = {};
    const char* converted = ::inet_ntop(AF_INET, &ipv4Address->sin_addr, buffer, sizeof(buffer));
    if (converted != nullptr) {
        output = converted;
    }

    ::freeaddrinfo(result);
    return converted != nullptr;
}

bool parseUnsignedArg(const char* option,
                      const char* text,
                      unsigned long maxValue,
                      unsigned long& value,
                      std::ostream& err) {
    if (text == nullptr || text[0] == '\0') {
        err << option << " requires a non-empty numeric value\n";
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(text, &end, 10);
    if (errno != 0 || end == text || end == nullptr || *end != '\0' || parsed > maxValue) {
        err << option << " must be an integer in range [0, " << maxValue << "]: " << text << "\n";
        return false;
    }

    value = parsed;
    return true;
}

bool parseFloatWord(const char* field, const std::string& text, float& value, std::ostream& err) {
    if (text.empty()) {
        err << field << " requires a non-empty numeric value\n";
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const float parsed = std::strtof(text.c_str(), &end);
    if (errno != 0 || end == text.c_str() || end == nullptr || *end != '\0') {
        err << field << " must be a numeric value: " << text << "\n";
        return false;
    }

    value = parsed;
    return true;
}

bool parseU32Word(const char* field, const std::string& text, U32& value, std::ostream& err) {
    if (!text.empty() && (text[0] == '-' || text[0] == '+')) {
        err << field << " must be an integer in range [0, " << std::numeric_limits<U32>::max() << "]: " << text
            << "\n";
        return false;
    }

    unsigned long parsed = 0UL;
    if (!parseUnsignedArg(field, text.c_str(), std::numeric_limits<U32>::max(), parsed, err)) {
        return false;
    }
    value = static_cast<U32>(parsed);
    return true;
}

bool parseU64Word(const char* field, const std::string& text, U64& value, std::ostream& err) {
    if (!text.empty() && (text[0] == '-' || text[0] == '+')) {
        err << field << " must be an integer in range [0, " << std::numeric_limits<U64>::max() << "]: " << text
            << "\n";
        return false;
    }

    if (text.empty()) {
        err << field << " requires a non-empty numeric value\n";
        return false;
    }

    char* end = nullptr;
    errno = 0;
    const unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || end == nullptr || *end != '\0') {
        err << field << " must be an integer in range [0, " << std::numeric_limits<U64>::max() << "]: " << text
            << "\n";
        return false;
    }
    if (parsed > std::numeric_limits<U64>::max()) {
        err << field << " must be an integer in range [0, " << std::numeric_limits<U64>::max() << "]: " << text
            << "\n";
        return false;
    }
    value = static_cast<U64>(parsed);
    return true;
}

const char* satModeName(const OBC::SatMode mode) {
    switch (mode.e) {
        case OBC::SatMode::SAFE:
            return "SAFE";
        case OBC::SatMode::IDLE:
            return "IDLE";
        case OBC::SatMode::HELL:
            return "HELL";
        case OBC::SatMode::PAYLOAD:
            return "PAYLOAD";
        case OBC::SatMode::TTC:
            return "TTC";
        default:
            return "UNKNOWN";
    }
}

const char* adcsModeName(const OBC::AdcsMode mode) {
    switch (mode.e) {
        case OBC::AdcsMode::IDLE:
            return "IDLE";
        case OBC::AdcsMode::DETUMBLE:
            return "DETUMBLE";
        case OBC::AdcsMode::POINTING:
            return "POINTING";
        case OBC::AdcsMode::SLEW:
            return "SLEW";
        default:
            return "UNKNOWN";
    }
}

const char* commBandName(const OBC::CommBand band) {
    return band.e == OBC::CommBand::UHF ? "UHF" : "SBAND";
}

const char* gpsSourceModeName(const OBC::GPS::SourceMode mode) {
    switch (mode) {
        case OBC::GPS::SourceMode::FAKE:
            return "FAKE";
        case OBC::GPS::SourceMode::REPLAY:
            return "REPLAY";
        case OBC::GPS::SourceMode::LIVE_UART:
            return "LIVE_UART";
        default:
            return "UNKNOWN";
    }
}

const char* bootSlotName(const OBC::BootSlot slot) {
    switch (slot.e) {
        case OBC::BootSlot::SLOT_A:
            return "SLOT_A";
        case OBC::BootSlot::SLOT_B:
            return "SLOT_B";
        default:
            return "NONE";
    }
}

const char* watchdogStateName(const OBC::WatchdogState state) {
    switch (state.e) {
        case OBC::WatchdogState::DISABLED:
            return "DISABLED";
        case OBC::WatchdogState::HEALTHY:
            return "HEALTHY";
        case OBC::WatchdogState::WARNING:
            return "WARNING";
        case OBC::WatchdogState::LATCHED_FAULT:
            return "LATCHED_FAULT";
        case OBC::WatchdogState::FEED_SUPPRESSED:
            return "FEED_SUPPRESSED";
        default:
            return "UNKNOWN";
    }
}

const char* watchdogRecoveryLevelName(const OBC::WatchdogRecoveryLevel level) {
    switch (level.e) {
        case OBC::WatchdogRecoveryLevel::NONE:
            return "NONE";
        case OBC::WatchdogRecoveryLevel::WARNING:
            return "WARNING";
        case OBC::WatchdogRecoveryLevel::LATCHED_FAULT:
            return "LATCHED_FAULT";
        case OBC::WatchdogRecoveryLevel::SAFE_REQUESTED:
            return "SAFE_REQUESTED";
        case OBC::WatchdogRecoveryLevel::FEED_SUPPRESSED:
            return "FEED_SUPPRESSED";
        default:
            return "UNKNOWN";
    }
}

const char* ttcReasonName(const OBC::TtcPassPolicyReason reason) {
    return OBC::ttcPassPolicyReasonName(reason);
}

std::string bytesToHex(const std::string& bytes) {
    std::ostringstream output;
    output << std::hex << std::uppercase << std::setfill('0');
    for (unsigned char value : bytes) {
        output << std::setw(2) << static_cast<unsigned int>(value);
    }
    return output.str();
}

std::string bytesToAsciiPreview(const std::string& bytes) {
    std::string preview;
    preview.reserve(bytes.size());
    for (unsigned char value : bytes) {
        preview.push_back(value >= 0x20U && value <= 0x7EU ? static_cast<char>(value) : '.');
    }
    return preview;
}

bool parseHealthItemWord(const std::string& value, OBC::HealthItem& item) {
    if (value == "cpu" || value == "cpu-usage") {
        item = OBC::HealthItem::CPU_USAGE;
        return true;
    }
    if (value == "rss" || value == "mem-rss" || value == "mem-rss-mb") {
        item = OBC::HealthItem::MEM_RSS_MB;
        return true;
    }
    return false;
}

bool handleHelp(std::istringstream&,
                RuntimeServices&,
                RuntimeState&,
                const RuntimeConfig&,
                std::ostream& out) {
    printHelp(out);
    return true;
}

bool handleQuit(std::istringstream&,
                RuntimeServices&,
                RuntimeState&,
                const RuntimeConfig&,
                std::ostream&) {
    return false;
}

bool handleStatus(std::istringstream&,
                  RuntimeServices& services,
                  RuntimeState& state,
                  const RuntimeConfig& config,
                  std::ostream& out) {
    refreshState(services, state);
    printStatus(services, state, config, out);
    return true;
}

bool handleMode(std::istringstream& input,
                RuntimeServices& services,
                RuntimeState& state,
                const RuntimeConfig& config,
                std::ostream& out) {
    std::string value;
    input >> value;
    OBC::SatMode mode;
    if (!parseSatMode(value, mode)) {
        out << "unknown mode\n";
        return true;
    }
    const Fw::CmdResponse response = services.setMode(mode);
    out << "mode response=" << static_cast<int>(response.e) << "\n";
    refreshState(services, state);
    printStatus(services, state, config, out);
    return true;
}

bool handleCsp(std::istringstream& input,
               RuntimeServices& services,
               RuntimeState&,
               const RuntimeConfig&,
               std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "ping") {
        unsigned int node = 0U;
        input >> node;
        bool success = false;
        const Fw::CmdResponse response = services.pingCsp(static_cast<U8>(node), 250U, success);
        out << "csp ping response=" << static_cast<int>(response.e)
            << " success=" << (success ? "yes" : "no") << "\n";
    }
    return true;
}

bool handleEps(std::istringstream& input,
               RuntimeServices& services,
               RuntimeState& state,
               const RuntimeConfig& config,
               std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "get") {
        state.haveEps = services.refreshEpsStatus(state.eps);
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "pdu") {
        unsigned int channel = 0U;
        std::string enabledWord;
        input >> channel >> enabledWord;
        bool enabled = false;
        if (!parseBoolWord(enabledWord, enabled)) {
            out << "expected on/off\n";
            return true;
        }
        const Fw::CmdResponse response = services.setEpsPdu(static_cast<U8>(channel), enabled, state.eps);
        state.haveEps = response == Fw::CmdResponse::OK;
        out << "eps pdu response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    if (action == "heater") {
        std::string enabledWord;
        input >> enabledWord;
        bool enabled = false;
        if (!parseBoolWord(enabledWord, enabled)) {
            out << "expected on/off\n";
            return true;
        }
        const Fw::CmdResponse response = services.setEpsHeater(enabled, state.eps);
        state.haveEps = response == Fw::CmdResponse::OK;
        out << "eps heater response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    return true;
}

bool handleAdcs(std::istringstream& input,
                RuntimeServices& services,
                RuntimeState& state,
                const RuntimeConfig& config,
                std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "get") {
        state.haveAdcs = services.refreshAdcsState(state.adcs);
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "mode") {
        std::string value;
        input >> value;
        OBC::AdcsMode mode;
        if (!parseAdcsMode(value, mode)) {
            out << "unknown adcs mode\n";
            return true;
        }
        const Fw::CmdResponse response = services.setAdcsMode(mode, state.adcs);
        state.haveAdcs = response == Fw::CmdResponse::OK;
        out << "adcs mode response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    return true;
}

bool handleGps(std::istringstream& input,
               RuntimeServices& services,
               RuntimeState& state,
               const RuntimeConfig& config,
               std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "get") {
        const bool success = services.pollGpsState();
        refreshState(services, state);
        out << "gps get success=" << (success ? "yes" : "no") << "\n";
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "source") {
        std::string value;
        input >> value;
        OBC::GpsSourceMode mode;
        if (!parseGpsSourceMode(value, mode)) {
            out << "unknown gps source\n";
            return true;
        }
        const Fw::CmdResponse response = services.setGpsSourceMode(mode);
        out << "gps source response=" << static_cast<int>(response.e) << "\n";
        refreshState(services, state);
        return true;
    }
    return true;
}

bool handleStorage(std::istringstream& input,
                   RuntimeServices& services,
                   RuntimeState& state,
                   const RuntimeConfig& config,
                   std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "get") {
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "scan") {
        const bool success = services.scanStorageHealth();
        refreshState(services, state);
        out << "storage scan success=" << (success ? "yes" : "no") << "\n";
        printStatus(services, state, config, out);
        return true;
    }
    return true;
}

bool handleComm(std::istringstream& input,
                RuntimeServices& services,
                RuntimeState&,
                const RuntimeConfig&,
                std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "band") {
        std::string value;
        input >> value;
        OBC::CommBand band;
        if (!parseCommBand(value, band)) {
            out << "unknown band\n";
            return true;
        }
        const Fw::CmdResponse response = services.setCommBand(band);
        out << "comm band response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    if (action == "pass") {
        std::string subaction;
        input >> subaction;
        if (subaction == "start") {
            unsigned int duration = 0U;
            input >> duration;
            const Fw::CmdResponse response = services.startCommPass(duration);
            out << "comm pass start response=" << static_cast<int>(response.e) << "\n";
        } else if (subaction == "stop") {
            const Fw::CmdResponse response = services.stopCommPass();
            out << "comm pass stop response=" << static_cast<int>(response.e) << "\n";
        }
        return true;
    }
    return true;
}

bool handleTtc(std::istringstream& input,
               RuntimeServices& services,
               RuntimeState& state,
               const RuntimeConfig& config,
               std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "status") {
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "config") {
        std::string enabledWord;
        std::string timeoutWord;
        input >> enabledWord >> timeoutWord;
        bool enabled = false;
        U32 timeoutSec = 0U;
        if (!parseBoolWord(enabledWord, enabled)) {
            out << "expected on/off\n";
            return true;
        }
        if (!parseU32Word("ttc lossOfLockTimeoutSec", timeoutWord, timeoutSec, out)) {
            return true;
        }
        const Fw::CmdResponse response = services.setTtcPolicy(enabled, timeoutSec);
        out << "ttc config response=" << static_cast<int>(response.e) << "\n";
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "window") {
        std::string subaction;
        input >> subaction;
        if (subaction == "set") {
            std::string startWord;
            std::string endWord;
            U64 startUnixSec = 0U;
            U64 endUnixSec = 0U;
            input >> startWord >> endWord;
            if (!parseU64Word("ttc window startUnixSec", startWord, startUnixSec, out) ||
                !parseU64Word("ttc window endUnixSec", endWord, endUnixSec, out)) {
                return true;
            }
            const Fw::CmdResponse response = services.setTtcPassWindow(startUnixSec, endUnixSec);
            out << "ttc window response=" << static_cast<int>(response.e) << "\n";
            refreshState(services, state);
            printStatus(services, state, config, out);
            return true;
        }
        if (subaction == "clear") {
            const Fw::CmdResponse response = services.clearTtcPassWindow();
            out << "ttc window clear response=" << static_cast<int>(response.e) << "\n";
            refreshState(services, state);
            printStatus(services, state, config, out);
            return true;
        }
    }
    return true;
}

bool handleHealth(std::istringstream& input,
                  RuntimeServices& services,
                  RuntimeState& state,
                  const RuntimeConfig& config,
                  std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "enable") {
        std::string enabledWord;
        bool enabled = false;
        input >> enabledWord;
        if (!parseBoolWord(enabledWord, enabled)) {
            out << "expected on/off\n";
            return true;
        }
        services.setHealthEnabled(enabled);
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "threshold") {
        std::string itemWord;
        std::string valueWord;
        input >> itemWord;
        input >> valueWord;
        OBC::HealthItem item;
        if (!parseHealthItemWord(itemWord, item)) {
            out << "unknown health item\n";
            return true;
        }
        float value = 0.0F;
        if (!parseFloatWord("health threshold value", valueWord, value, out)) {
            return true;
        }
        const Fw::CmdResponse response = services.setHealthThreshold(item, value);
        out << "health threshold response=" << static_cast<int>(response.e) << "\n";
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "status") {
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    return true;
}

bool handleWatchdog(std::istringstream& input,
                    RuntimeServices& services,
                    RuntimeState& state,
                    const RuntimeConfig& config,
                    std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "status") {
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "config") {
        std::string sourceWord;
        std::string enabledWord;
        std::string warningWord;
        std::string safeWord;
        std::string suppressWord;
        U32 warningTicks = 0U;
        U32 safeTicks = 0U;
        U32 suppressTicks = 0U;
        input >> sourceWord >> enabledWord >> warningWord >> safeWord >> suppressWord;
        OBC::WatchdogSource source;
        bool enabled = false;
        if (!parseWatchdogSource(sourceWord, source)) {
            out << "unknown watchdog source\n";
            return true;
        }
        if (!parseBoolWord(enabledWord, enabled)) {
            out << "expected on/off\n";
            return true;
        }
        if (!parseU32Word("watchdog warningTicks", warningWord, warningTicks, out) ||
            !parseU32Word("watchdog safeTicks", safeWord, safeTicks, out) ||
            !parseU32Word("watchdog suppressTicks", suppressWord, suppressTicks, out)) {
            return true;
        }
        const Fw::CmdResponse response =
            services.setWatchdogConfig(source, enabled, warningTicks, safeTicks, suppressTicks);
        out << "watchdog config response=" << static_cast<int>(response.e) << "\n";
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "suppress") {
        std::string sourceWord;
        std::string enabledWord;
        input >> sourceWord >> enabledWord;
        OBC::WatchdogSource source;
        bool suppressed = false;
        if (!parseWatchdogSource(sourceWord, source)) {
            out << "unknown watchdog source\n";
            return true;
        }
        if (!parseBoolWord(enabledWord, suppressed)) {
            out << "expected on/off\n";
            return true;
        }
        out << "watchdog suppress success="
            << (services.setWatchdogProbeSuppression(source, suppressed) ? "yes" : "no") << "\n";
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    return true;
}

bool handleRecovery(std::istringstream& input,
                    RuntimeServices& services,
                    RuntimeState& state,
                    const RuntimeConfig& config,
                    std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "status") {
        refreshState(services, state);
        printStatus(services, state, config, out);
    }
    return true;
}

bool handleFault(std::istringstream& input,
                 RuntimeServices& services,
                 RuntimeState&,
                 const RuntimeConfig&,
                 std::ostream& out) {
    std::string action;
    input >> action;
    if (action != "history") {
        return true;
    }

    std::string limitWord;
    input >> limitWord;
    U32 limit = OBC::PersistentFaultHistoryMaxReadback;
    if (!limitWord.empty() && !parseU32Word("fault history count", limitWord, limit, out)) {
        return true;
    }

    OBC::PersistentFaultHistoryStatus status = {};
    std::vector<OBC::PersistentFaultRecord> records = {};
    if (!services.getPersistentFaultHistory(limit, status, records)) {
        out << "fault history unavailable\n";
        return true;
    }

    out << "fault total=" << status.totalRecords << " returned=" << status.returnedRecords
        << " activeCopy=" << OBC::persistentFaultStoreCopyName(status.activeCopy)
        << " generation=" << status.generation << "\n";
    for (std::size_t index = 0; index < records.size(); index++) {
        const OBC::PersistentFaultRecord& record = records.at(index);
        out << "fault[" << index << "] kind=" << OBC::persistentFaultRecordKindName(record.kind)
            << " source=" << OBC::recoveryIncidentSourceName(record.source)
            << " level=" << OBC::recoveryLevelName(record.level)
            << " action=" << OBC::recoveryActionName(record.action)
            << " resetCause=" << OBC::resetCauseName(record.resetCause)
            << " bootCount=" << record.bootCount
            << " consecutiveResetCount=" << record.consecutiveResetCount
            << " uptimeSec=" << record.uptimeSec
            << " timestampSec=" << record.timestampSec
            << " detail=" << record.detail
            << " flags=" << record.flags << "\n";
    }
    return true;
}

bool handleRadio(std::istringstream& input,
                 RuntimeServices& services,
                 RuntimeState& state,
                 const RuntimeConfig& config,
                 std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "status") {
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "enable") {
        std::string enabledWord;
        input >> enabledWord;
        bool enabled = false;
        if (!parseBoolWord(enabledWord, enabled)) {
            out << "expected on/off\n";
            return true;
        }
        const Fw::CmdResponse response = services.enableRadio(enabled, state.radio);
        refreshState(services, state);
        out << "radio enable response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    if (action == "power") {
        unsigned int power = 0U;
        input >> power;
        const Fw::CmdResponse response = services.setRadioPower(static_cast<U8>(power), state.radio);
        refreshState(services, state);
        out << "radio power response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    if (action == "freq") {
        std::uint32_t freq = 0U;
        input >> freq;
        const Fw::CmdResponse response = services.setRadioFrequency(freq, state.radio);
        refreshState(services, state);
        out << "radio freq response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    return true;
}

bool handleUart(std::istringstream& input,
                RuntimeServices& services,
                RuntimeState&,
                const RuntimeConfig&,
                std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "raw") {
        std::string payload;
        std::getline(input, payload);
        if (!payload.empty() && payload.front() == ' ') {
            payload.erase(payload.begin());
        }
        std::string response;
        if (services.exchangeUart(payload + "\n", response)) {
            out << "uart response: " << response << "\n";
        } else {
            out << "uart exchange failed\n";
        }
        return true;
    }
    if (action == "frame-hex") {
        std::string hexPayload;
        input >> hexPayload;
        std::string payload;
        if (!parseHexBytes(hexPayload, payload)) {
            out << "expected even-length hex payload\n";
            return true;
        }

        const std::string request = OBC::COMM::encodeTransparentLinkFrame(payload);
        std::string encodedResponse;
        if (!services.exchangeDelimitedUart(request, encodedResponse, OBC::COMM::TRANSPARENT_FRAME_DELIMITER)) {
            out << "uart framed exchange failed\n";
            return true;
        }

        std::string responsePayload;
        OBC::COMM::TransparentFrameInfo frameInfo = {};
        const OBC::COMM::TransparentFrameStatus status =
            OBC::COMM::decodeTransparentLinkFrame(encodedResponse, responsePayload, &frameInfo);
        if (status != OBC::COMM::TransparentFrameStatus::OK) {
            out << "uart framed response decode failed: "
                << OBC::COMM::transparentFrameStatusName(status) << "\n";
            return true;
        }

        out << "uart frame response meta: version=" << static_cast<unsigned int>(frameInfo.version)
            << " flags=" << static_cast<unsigned int>(frameInfo.flags) << "\n";
        out << "uart frame response hex: " << bytesToHex(responsePayload) << "\n";
        out << "uart frame response ascii: " << bytesToAsciiPreview(responsePayload) << "\n";
        return true;
    }
    return true;
}

bool handleBoot(std::istringstream& input,
                RuntimeServices& services,
                RuntimeState& state,
                const RuntimeConfig& config,
                std::ostream& out) {
    std::string action;
    input >> action;
    if (action == "status") {
        refreshState(services, state);
        printStatus(services, state, config, out);
        return true;
    }
    if (action == "prepare") {
        U32 imageSize = 0U;
        std::string digest;
        input >> imageSize >> digest;
        const Fw::CmdResponse response = services.bootPrepareUpdate(imageSize, digest);
        out << "boot prepare response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    if (action == "verify") {
        std::string path;
        input >> path;
        const Fw::CmdResponse response = services.bootVerifyStagedImage(path);
        out << "boot verify response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    if (action == "activate") {
        const Fw::CmdResponse response = services.bootActivateStagedImage();
        out << "boot activate response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    if (action == "confirm") {
        const Fw::CmdResponse response = services.bootConfirm();
        out << "boot confirm response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    if (action == "rollback") {
        const Fw::CmdResponse response = services.bootRollback();
        out << "boot rollback response=" << static_cast<int>(response.e) << "\n";
        return true;
    }
    return true;
}

using CommandHandler = bool (*)(std::istringstream&,
                                RuntimeServices&,
                                RuntimeState&,
                                const RuntimeConfig&,
                                std::ostream&);

struct CommandEntry {
    const char* name;
    CommandHandler handler;
};

const CommandEntry COMMAND_TABLE[] = {
    {"help", handleHelp},
    {"quit", handleQuit},
    {"exit", handleQuit},
    {"status", handleStatus},
    {"mode", handleMode},
    {"csp", handleCsp},
    {"eps", handleEps},
    {"adcs", handleAdcs},
    {"gps", handleGps},
    {"storage", handleStorage},
    {"comm", handleComm},
    {"ttc", handleTtc},
    {"health", handleHealth},
    {"watchdog", handleWatchdog},
    {"recovery", handleRecovery},
    {"fault", handleFault},
    {"radio", handleRadio},
    {"uart", handleUart},
    {"boot", handleBoot},
};

}  // namespace

OBC::COMM::GroundLinkBackendMode effectiveGroundLinkMode(const RuntimeConfig& config) {
    if (config.groundLinkMode == "disabled") {
        return OBC::COMM::GroundLinkBackendMode::DISABLED;
    }
    if (config.groundLinkMode == "comm-csp") {
        return OBC::COMM::GroundLinkBackendMode::COMM_CSP;
    }
    if (config.gdsPort == 0U) {
        return OBC::COMM::GroundLinkBackendMode::DISABLED;
    }
    return OBC::COMM::GroundLinkBackendMode::DIRECT_TCP;
}

OBC::COMM::GroundLinkHealthSemantics groundLinkHealthSemanticsForCommCspNode(const std::uint16_t commCspNode) {
    switch (commCspNode) {
        case OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID:
        case OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID:
            return OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP;
        case OBC::COMM::CSP::DEFAULT_GENERIC_COMM_NODE_ID:
            return OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK;
        default:
            return OBC::COMM::GroundLinkHealthSemantics::DISABLED;
    }
}

Fw::TimeInterval tickIntervalFromMs(int tickMs) {
    const U32 seconds = static_cast<U32>(tickMs / 1000);
    const U32 microseconds = static_cast<U32>((tickMs % 1000) * 1000);
    return Fw::TimeInterval(seconds, microseconds);
}

void printUsage(const char* app, std::ostream& out) {
    out << "Usage: " << app
        << " [--comm tcp|serial] [--comm-host 127.0.0.1] [--comm-port 7000]"
        << " [--comm-device /path/to/tty] [--comm-baudrate 115200] [--radio-protocol mock-text]"
        << " [--ground-link direct-tcp|comm-csp|disabled] [--comm-csp-node 5]"
        << " [--initial-comm-band sband|uhf]"
        << " [--command-authority-profile sband-primary|uhf-primary|uhf-backup|dev-direct|internal]"
        << " [--boot-trust hmac-sha256] [--boot-trust-signer-id repo-dev-boot-signer]"
        << " [--boot-trust-key-slot 1] [--boot-trust-key-hex <hex>]"
        << " [--uhf-beacon-csp-node 6]"
        << " [--gds-host 127.0.0.1] [--gds-port 50000]"
        << " [--runtime-root /path/to/runtime] [--persistent-root /path/to/persistent-data]"
        << " [--staging-root /path/to/staging]"
        << " [--hardware-watchdog disabled|linux-device] [--hardware-watchdog-device /dev/watchdog0]"
        << " [--hardware-watchdog-timeout-sec 15]"
        << " [--diagnostic-quiet-packet-egress]"
        << " [--enable-comm-subsystem-health-detector]"
        << " [--disable-primary-ground-link-driver]"
        << " [--headless]"
        << " [--tick-ms 1000]\n";
}

void finalizeRuntimeRoots(RuntimeConfig& config) {
    if (config.runtimeRoot.empty()) {
        config.runtimeRoot = "runtime";
    }
    if (config.persistentRoot.empty()) {
        config.persistentRoot = joinPath(config.runtimeRoot, "persistent-data");
    }
    if (config.stagingRoot.empty()) {
        config.stagingRoot = joinPath(config.runtimeRoot, "staging");
    }
}

bool finalizeNetworkTargets(RuntimeConfig& config, std::ostream& err) {
    if (config.commMode == "tcp") {
        std::string resolvedCommHost;
        if (!resolveIpv4Host(config.commHost, resolvedCommHost)) {
            err << "Failed to resolve --comm-host to an IPv4 address: " << config.commHost << "\n";
            return false;
        }
        config.commHost = resolvedCommHost;
    }

    if (config.gdsPort != 0U) {
        std::string resolvedGdsHost;
        if (!resolveIpv4Host(config.gdsHost, resolvedGdsHost)) {
            err << "Failed to resolve --gds-host to an IPv4 address: " << config.gdsHost << "\n";
            return false;
        }
        config.gdsHost = resolvedGdsHost;
    }

    return true;
}

bool parseArgs(int argc, char* argv[], RuntimeConfig& config, std::ostream& out, std::ostream& err) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0], out);
            return false;
        }
        if (arg == "--comm" && i + 1 < argc) {
            config.commMode = argv[++i];
            continue;
        }
        if (arg == "--comm-host" && i + 1 < argc) {
            config.commHost = argv[++i];
            continue;
        }
        if (arg == "--radio-protocol" && i + 1 < argc) {
            config.radioProtocol = argv[++i];
            continue;
        }
        if (arg == "--comm-port" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--comm-port", argv[++i], std::numeric_limits<std::uint16_t>::max(), parsed, err)) {
                return false;
            }
            config.commPort = static_cast<std::uint16_t>(parsed);
            continue;
        }
        if (arg == "--comm-device" && i + 1 < argc) {
            config.commDevice = argv[++i];
            continue;
        }
        if (arg == "--comm-baudrate" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--comm-baudrate", argv[++i], std::numeric_limits<std::uint32_t>::max(), parsed, err)) {
                return false;
            }
            config.commBaudrate = static_cast<std::uint32_t>(parsed);
            continue;
        }
        if (arg == "--ground-link" && i + 1 < argc) {
            config.groundLinkMode = argv[++i];
            continue;
        }
        if (arg == "--command-authority-profile" && i + 1 < argc) {
            config.commandAuthorityProfile = argv[++i];
            continue;
        }
        if (arg == "--initial-comm-band" && i + 1 < argc) {
            OBC::CommBand band;
            if (!parseCommBand(argv[++i], band)) {
                err << "Invalid --initial-comm-band value\n";
                return false;
            }
            config.initialCommBand = band;
            continue;
        }
        if (arg == "--command-auth-keystore" || arg == "--command-auth" || arg == "--command-auth-source-id" ||
            arg == "--command-auth-key-slot" || arg == "--command-auth-key-hex") {
            err << arg << " has been removed; hosted comm-managed auth now loads from "
                << OBC::defaultCommandAuthKeystorePath() << "\n";
            return false;
        }
        if (arg == "--boot-trust" && i + 1 < argc) {
            config.bootTrustMode = argv[++i];
            continue;
        }
        if (arg == "--boot-trust-signer-id" && i + 1 < argc) {
            config.bootTrustSignerId = argv[++i];
            continue;
        }
        if (arg == "--boot-trust-key-slot" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--boot-trust-key-slot",
                                  argv[++i],
                                  std::numeric_limits<std::uint32_t>::max(),
                                  parsed,
                                  err)) {
                return false;
            }
            config.bootTrustKeySlot = static_cast<std::uint32_t>(parsed);
            continue;
        }
        if (arg == "--boot-trust-key-hex" && i + 1 < argc) {
            config.bootTrustKeyHex = argv[++i];
            continue;
        }
        if (arg == "--comm-csp-node" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--comm-csp-node", argv[++i], std::numeric_limits<std::uint16_t>::max(), parsed, err)) {
                return false;
            }
            config.commCspNode = static_cast<std::uint16_t>(parsed);
            continue;
        }
        if (arg == "--uhf-beacon-csp-node" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--uhf-beacon-csp-node", argv[++i], std::numeric_limits<std::uint16_t>::max(), parsed, err)) {
                return false;
            }
            config.uhfBeaconCspNode = static_cast<std::uint16_t>(parsed);
            continue;
        }
        if (arg == "--gds-host" && i + 1 < argc) {
            config.gdsHost = argv[++i];
            continue;
        }
        if (arg == "--gds-port" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--gds-port", argv[++i], std::numeric_limits<std::uint16_t>::max(), parsed, err)) {
                return false;
            }
            config.gdsPort = static_cast<std::uint16_t>(parsed);
            continue;
        }
        if (arg == "--runtime-root" && i + 1 < argc) {
            config.runtimeRoot = argv[++i];
            continue;
        }
        if (arg == "--persistent-root" && i + 1 < argc) {
            config.persistentRoot = argv[++i];
            continue;
        }
        if (arg == "--staging-root" && i + 1 < argc) {
            config.stagingRoot = argv[++i];
            continue;
        }
        if (arg == "--hardware-watchdog" && i + 1 < argc) {
            config.hardwareWatchdogMode = argv[++i];
            continue;
        }
        if (arg == "--hardware-watchdog-device" && i + 1 < argc) {
            config.hardwareWatchdogDevice = argv[++i];
            continue;
        }
        if (arg == "--hardware-watchdog-timeout-sec" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--hardware-watchdog-timeout-sec",
                                  argv[++i],
                                  std::numeric_limits<std::uint32_t>::max(),
                                  parsed,
                                  err)) {
                return false;
            }
            config.hardwareWatchdogTimeoutSec = static_cast<std::uint32_t>(parsed);
            continue;
        }
        if (arg == "--diagnostic-quiet-packet-egress") {
            config.diagnosticQuietPacketEgress = true;
            continue;
        }
        if (arg == "--enable-comm-subsystem-health-detector") {
            config.enableCommSubsystemHealthDetector = true;
            continue;
        }
        if (arg == "--disable-primary-ground-link-driver") {
            config.enablePrimaryGroundLinkDriver = false;
            continue;
        }
        if (arg == "--comm-subsystem-ping-timeout-ms" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--comm-subsystem-ping-timeout-ms",
                                  argv[++i],
                                  std::numeric_limits<std::uint32_t>::max(),
                                  parsed,
                                  err)) {
                return false;
            }
            config.commSubsystemPingTimeoutMs = static_cast<std::uint32_t>(parsed);
            continue;
        }
        if (arg == "--comm-primary-unavailable-failure-threshold" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--comm-primary-unavailable-failure-threshold",
                                  argv[++i],
                                  std::numeric_limits<std::uint32_t>::max(),
                                  parsed,
                                  err)) {
                return false;
            }
            config.commPrimaryUnavailableFailureThreshold = static_cast<std::uint32_t>(parsed);
            continue;
        }
        if (arg == "--tick-ms" && i + 1 < argc) {
            unsigned long parsed = 0;
            if (!parseUnsignedArg("--tick-ms", argv[++i], static_cast<unsigned long>(std::numeric_limits<int>::max()), parsed, err)) {
                return false;
            }
            config.tickMs = static_cast<int>(parsed);
            continue;
        }
        if (arg == "--headless") {
            config.headless = true;
            continue;
        }

        err << "Unknown argument: " << arg << "\n";
        printUsage(argv[0], out);
        return false;
    }

    if (config.commMode == "pty") {
        config.commMode = "serial";
    }
    if (config.commMode != "tcp" && config.commMode != "serial") {
        err << "Invalid --comm mode: " << config.commMode << "\n";
        return false;
    }
    if (config.commMode == "serial" && config.commDevice.empty()) {
        err << "--comm-device is required when --comm serial is selected\n";
        return false;
    }
    if (config.commBaudrate == 0U) {
        err << "--comm-baudrate must be non-zero\n";
        return false;
    }
    if (!OBC::COMM::isSupportedRadioProtocol(config.radioProtocol)) {
        err << "Unsupported --radio-protocol: " << config.radioProtocol << "\n";
        return false;
    }
    if (config.groundLinkMode != "direct-tcp" && config.groundLinkMode != "comm-csp" &&
        config.groundLinkMode != "disabled") {
        err << "Invalid --ground-link mode: " << config.groundLinkMode << "\n";
        return false;
    }
    if (config.hardwareWatchdogMode != "disabled" && config.hardwareWatchdogMode != "linux-device") {
        err << "Invalid --hardware-watchdog mode: " << config.hardwareWatchdogMode << "\n";
        return false;
    }
    if (config.hardwareWatchdogMode == "linux-device") {
        if (config.hardwareWatchdogDevice.empty()) {
            err << "--hardware-watchdog-device is required when --hardware-watchdog linux-device is selected\n";
            return false;
        }
        if (config.hardwareWatchdogTimeoutSec == 0U) {
            err << "--hardware-watchdog-timeout-sec must be non-zero when --hardware-watchdog linux-device is selected\n";
            return false;
        }
    }
    if (config.bootTrustMode != "hmac-sha256") {
        err << "Invalid --boot-trust mode: " << config.bootTrustMode << "\n";
        return false;
    }
    if (config.bootTrustSignerId.empty() || config.bootTrustKeySlot == 0U || config.bootTrustKeyHex.empty()) {
        err << "--boot-trust hmac-sha256 requires signer id, non-zero key slot, and key hex\n";
        return false;
    }
    std::string decodedBootTrustKey;
    if (!parseHexBytes(config.bootTrustKeyHex, decodedBootTrustKey)) {
        err << "--boot-trust-key-hex must be an even-length hexadecimal string\n";
        return false;
    }
    if (config.commCspNode == 0U) {
        err << "--comm-csp-node must be in range 1..65535\n";
        return false;
    }
    if (config.uhfBeaconCspNode > 255U) {
        err << "--uhf-beacon-csp-node must fit in an 8-bit CSP node id\n";
        return false;
    }
    if (config.tickMs <= 0) {
        err << "--tick-ms must be positive\n";
        return false;
    }
    if (config.commSubsystemPingTimeoutMs == 0U) {
        err << "--comm-subsystem-ping-timeout-ms must be non-zero\n";
        return false;
    }
    if (config.commPrimaryUnavailableFailureThreshold == 0U) {
        err << "--comm-primary-unavailable-failure-threshold must be non-zero\n";
        return false;
    }

    finalizeRuntimeRoots(config);
    if (!finalizeNetworkTargets(config, err)) {
        return false;
    }
    return true;
}

bool parseBoolWord(const std::string& value, bool& enabled) {
    if (value == "on" || value == "1" || value == "true") {
        enabled = true;
        return true;
    }
    if (value == "off" || value == "0" || value == "false") {
        enabled = false;
        return true;
    }
    return false;
}

bool parseSatMode(const std::string& value, OBC::SatMode& mode) {
    if (value == "safe") {
        mode = OBC::SatMode::SAFE;
        return true;
    }
    if (value == "idle") {
        mode = OBC::SatMode::IDLE;
        return true;
    }
    if (value == "hell") {
        mode = OBC::SatMode::HELL;
        return true;
    }
    if (value == "payload") {
        mode = OBC::SatMode::PAYLOAD;
        return true;
    }
    if (value == "ttc") {
        mode = OBC::SatMode::TTC;
        return true;
    }
    return false;
}

bool parseAdcsMode(const std::string& value, OBC::AdcsMode& mode) {
    if (value == "idle") {
        mode = OBC::AdcsMode::IDLE;
        return true;
    }
    if (value == "detumble") {
        mode = OBC::AdcsMode::DETUMBLE;
        return true;
    }
    if (value == "pointing") {
        mode = OBC::AdcsMode::POINTING;
        return true;
    }
    if (value == "slew") {
        mode = OBC::AdcsMode::SLEW;
        return true;
    }
    return false;
}

bool parseCommBand(const std::string& value, OBC::CommBand& band) {
    if (value == "sband") {
        band = OBC::CommBand::SBAND;
        return true;
    }
    if (value == "uhf") {
        band = OBC::CommBand::UHF;
        return true;
    }
    return false;
}

bool parseGpsSourceMode(const std::string& value, OBC::GpsSourceMode& mode) {
    if (value == "fake") {
        mode = OBC::GpsSourceMode::FAKE;
        return true;
    }
    if (value == "replay") {
        mode = OBC::GpsSourceMode::REPLAY;
        return true;
    }
    if (value == "live-uart") {
        mode = OBC::GpsSourceMode::LIVE_UART;
        return true;
    }
    return false;
}

bool parseWatchdogSource(const std::string& value, OBC::WatchdogSource& source) {
    if (value == "eps-bridge") {
        source = OBC::WatchdogSource::EPS_BRIDGE;
        return true;
    }
    if (value == "eps-fdir") {
        source = OBC::WatchdogSource::EPS_FDIR;
        return true;
    }
    if (value == "adcs-fdir") {
        source = OBC::WatchdogSource::ADCS_FDIR;
        return true;
    }
    if (value == "mode-safety") {
        source = OBC::WatchdogSource::MODE_SAFETY;
        return true;
    }
    if (value == "comm-controller") {
        source = OBC::WatchdogSource::COMM_CONTROLLER;
        return true;
    }
    return false;
}

bool parseHexBytes(const std::string& hex, std::string& bytes) {
    bytes.clear();
    if (hex.empty() || (hex.size() % 2U) != 0U) {
        return false;
    }

    auto decodeNibble = [](char ch, std::uint8_t& value) -> bool {
        if (ch >= '0' && ch <= '9') {
            value = static_cast<std::uint8_t>(ch - '0');
            return true;
        }
        if (ch >= 'a' && ch <= 'f') {
            value = static_cast<std::uint8_t>(10 + ch - 'a');
            return true;
        }
        if (ch >= 'A' && ch <= 'F') {
            value = static_cast<std::uint8_t>(10 + ch - 'A');
            return true;
        }
        return false;
    };

    bytes.reserve(hex.size() / 2U);
    for (std::size_t index = 0; index < hex.size(); index += 2U) {
        std::uint8_t hi = 0U;
        std::uint8_t lo = 0U;
        if (!decodeNibble(hex[index], hi) || !decodeNibble(hex[index + 1U], lo)) {
            bytes.clear();
            return false;
        }
        bytes.push_back(static_cast<char>((hi << 4U) | lo));
    }
    return true;
}

void printHelp(std::ostream& out) {
    out << "Commands:\n"
        << "  help\n"
        << "  status\n"
        << "  mode <safe|idle|hell|payload|ttc>\n"
        << "  csp ping <node-id>\n"
        << "  eps get\n"
        << "  eps pdu <channel> <on|off>\n"
        << "  eps heater <on|off>\n"
        << "  adcs get\n"
        << "  adcs mode <idle|detumble|pointing|slew>\n"
        << "  gps get\n"
        << "  gps source <fake|replay|live-uart>\n"
        << "  storage get\n"
        << "  storage scan\n"
        << "  comm band <sband|uhf>\n"
        << "  comm pass start <sec>\n"
        << "  comm pass stop\n"
        << "  ttc status\n"
        << "  ttc config <on|off> <loss-timeout-sec>\n"
        << "  ttc window set <start-unix-sec> <end-unix-sec>\n"
        << "  ttc window clear\n"
        << "  health status\n"
        << "  health enable <on|off>\n"
        << "  health threshold <cpu|rss> <value>\n"
        << "  watchdog status\n"
        << "  watchdog config <eps-bridge|eps-fdir|adcs-fdir|mode-safety|comm-controller> <on|off> <warn> <safe> <suppress>\n"
        << "  watchdog suppress <eps-bridge|eps-fdir|adcs-fdir|mode-safety|comm-controller> <on|off>\n"
        << "  recovery status\n"
        << "  fault history [count]\n"
        << "  radio status\n"
        << "  radio enable <on|off>\n"
        << "  radio power <dbm>\n"
        << "  radio freq <hz>\n"
        << "  uart raw <request-without-newline>\n"
        << "  uart frame-hex <hex-payload>\n"
        << "  boot status\n"
        << "  boot prepare <image-size> <sha256-hex>\n"
        << "  boot verify <staging-path>\n"
        << "  boot activate\n"
        << "  boot confirm\n"
        << "  boot rollback\n"
        << "  quit\n";
}

void refreshState(RuntimeServices& services, RuntimeState& state) {
    state.haveEps = services.getCachedEpsStatus(state.eps);
    state.haveAdcs = services.getCachedAdcsState(state.adcs);
    state.haveGps = services.getGpsCachedState(state.gps);
    state.radioObservation = services.getRadioObservation();
    state.haveRadio = state.radioObservation.haveSample;
    if (state.haveRadio) {
        state.radio = state.radioObservation.lastStatus;
    } else {
        state.radio = {};
    }
    state.haveStorageHealth = services.getStorageHealth(state.storage);
    state.ttc = services.getTtcPassStatus();
}

void sampleHealth(RuntimeServices& services) {
    services.updateResourceSample(0.0F, currentRssMb());
}

void printStatus(RuntimeServices& services,
                 const RuntimeState& state,
                 const RuntimeConfig& config,
                 std::ostream& out) {
    const OBC::CommRuntimeState commState = services.getCommState();
    const OBC::CspRuntimeCounters cspState = services.getCspCounters();
    const OBC::WatchdogRuntimeSnapshot watchdogState = services.getWatchdogStatus();
    const OBC::LinuxWatchdogRuntimeStatus hwWatchdogState = services.getHardwareWatchdogStatus();
    const OBC::RecoveryRuntimeStatus recoveryState = services.getRecoveryStatus();
    const OBC::TtcPassRuntimeStatus ttcState = services.getTtcPassStatus();
    OBC::ADCS::PollHealthState adcsPollHealth = {};
    const bool haveAdcsPollHealth = services.getAdcsPollHealth(adcsPollHealth);
    const OBC::BootMetadata& bootState = services.getBootMetadata();
    const OBC::COMM::ByteStreamStats uartStats = services.getUartStats();
    const OBC::COMM::ByteStreamStats radioStats = services.getRadioLinkStats();
    const OBC::COMM::GroundLinkStats groundLinkStats = services.getGroundLinkStats();
    const OBC::COMM::GroundLinkObservationState sbandObservation =
        services.getGroundLinkObservation(OBC::CommBand::SBAND);
    const OBC::COMM::GroundLinkObservationState uhfObservation = services.getGroundLinkObservation(OBC::CommBand::UHF);
    const OBC::COMM::GroundLinkBackendMode groundLinkMode = effectiveGroundLinkMode(config);

    out << std::fixed << std::setprecision(2);
    out << "mode=" << satModeName(services.getMode())
        << " uptime=" << services.getUptime()
        << " rebootCount=" << services.getRebootCount() << "\n";
    out << "csp initialized=" << (cspState.initialized ? "yes" : "no")
        << " node=" << static_cast<unsigned int>(cspState.localNodeId)
        << " tx=" << cspState.txPackets
        << " rx=" << cspState.rxPackets
        << " errors=" << cspState.errorCount << "\n";
    out << "comm band=" << commBandName(commState.activeBand)
        << " passActive=" << (commState.passActive ? "yes" : "no")
        << " remaining=" << commState.passRemainingSec
        << " totalPasses=" << commState.totalPasses << "\n";
    out << "comm primary command=" << commBandName(commState.primaryCommandLink)
        << " telemetry=" << commBandName(commState.primaryTelemetryLink)
        << " file=" << commBandName(commState.primaryFileLink)
        << " sbandAvailable=" << (commState.sbandAvailable ? "yes" : "no")
        << " uhfAvailable=" << (commState.uhfAvailable ? "yes" : "no")
        << " sbandActivityAge=" << commState.sbandActivityAgeTicks
        << " uhfActivityAge=" << commState.uhfActivityAgeTicks
        << " sbandReason=" << commLinkAvailabilityReasonName(commState.sbandAvailabilityReason)
        << " uhfReason=" << commLinkAvailabilityReasonName(commState.uhfAvailabilityReason)
        << " activeOwner=" << commState.downlinkActiveOwner
        << " pendingOwner=" << commState.downlinkPendingOwner << "\n";
    out << "comm fdir latched=" << (commState.fdirFaultLatched ? "yes" : "no")
        << " kind=" << static_cast<U32>(commState.fdirFaultKind)
        << " unavailableCount=" << commState.consecutivePrimaryUnavailable
        << " transportCount=" << commState.consecutivePrimaryTransportGrowth
        << " recoveryFailovers=" << commState.recoveryFailoverTotal
        << " recoveryOwnerClears=" << commState.recoveryOwnerClearTotal << "\n";
    out << "comm uhfPrimaryPacketQuiet active=" << (commState.uhfPrimaryPacketQuietActive ? "yes" : "no") << "\n";
    out << "comm uhfBeaconSuppress active=" << (commState.uhfBeaconSuppressActive ? "yes" : "no")
        << " ingressPort=" << commState.uhfBeaconSuppressIngressPort
        << " role=" << static_cast<U32>(commState.uhfBeaconSuppressRole)
        << " sessionId=" << commState.uhfBeaconSuppressSessionId
        << " lastSequence=" << commState.uhfBeaconSuppressLastAcceptedSequence
        << " remainingTicks=" << commState.uhfBeaconSuppressRemainingTicks
        << " timeoutTicks=" << commState.uhfBeaconSuppressTimeoutTicks << "\n";
    out << "ttc enabled=" << (ttcState.enabled ? "yes" : "no")
        << " windowConfigured=" << (ttcState.windowConfigured ? "yes" : "no")
        << " windowActive=" << (ttcState.windowActive ? "yes" : "no")
        << " gpsValid=" << (ttcState.gpsTimeValid ? "yes" : "no")
        << " ttcActive=" << (ttcState.ttcActive ? "yes" : "no")
        << " lossTimeout=" << ttcState.lossOfLockTimeoutSec
        << " lossTimer=" << ttcState.lossOfLockTimerSec
        << " window=[" << ttcState.windowStartUnixSec << "," << ttcState.windowEndUnixSec << ")"
        << " gpsUnix=" << ttcState.currentGpsUnixSec
        << " entryReason=" << ttcReasonName(ttcState.lastEntryReason)
        << " exitReason=" << ttcReasonName(ttcState.lastExitReason)
        << " entryCount=" << ttcState.entryCount
        << " exitCount=" << ttcState.exitCount << "\n";
    out << "watchdog aggregate=" << watchdogStateName(watchdogState.aggregateState)
        << " recovery=" << watchdogRecoveryLevelName(watchdogState.recoveryLevel)
        << " feedEligible=" << (watchdogState.feedEligible ? "yes" : "no")
        << " warningMask=0x" << std::hex << watchdogState.warningMask
        << " faultMask=0x" << watchdogState.faultMask
        << " suppressMask=0x" << watchdogState.suppressMask << std::dec
        << " safeRequests=" << watchdogState.safeRequestCount
        << " feedSuppressions=" << watchdogState.feedSuppressCount << "\n";
    for (const auto& source : watchdogState.sources) {
        out << "watchdog source=" << OBC::watchdogSourceName(source.source)
            << " enabled=" << (source.config.enabled ? "yes" : "no")
            << " state=" << watchdogStateName(source.state)
            << " age=" << source.ageTicks
            << " beatPending=" << (source.beatPending ? "yes" : "no")
            << " probeSuppressed=" << (source.probeSuppressed ? "yes" : "no")
            << " warn/safe/suppress=" << source.config.warningTicks << "/" << source.config.safeTicks << "/"
            << source.config.suppressTicks << "\n";
    }
    out << "hwWatchdog mode=" << config.hardwareWatchdogMode
        << " enabled=" << (hwWatchdogState.enabled ? "yes" : "no")
        << " open=" << (hwWatchdogState.open ? "yes" : "no")
        << " device=" << (hwWatchdogState.devicePath.empty() ? "<none>" : hwWatchdogState.devicePath)
        << " timeoutSec=" << hwWatchdogState.timeoutSec
        << " feedCount=" << hwWatchdogState.feedCount
        << " lastFeedCode=" << hwWatchdogState.lastFeedCode
        << " lastError=" << hwWatchdogState.lastError << "\n";
    out << "recovery activeCount=" << recoveryState.activeIncidentCount
        << " activeSource=" << OBC::recoveryIncidentSourceName(recoveryState.activeSource)
        << " currentLevel=" << OBC::recoveryLevelName(recoveryState.currentLevel)
        << " highestLevel=" << OBC::recoveryLevelName(recoveryState.highestLevel)
        << " lastAction=" << OBC::recoveryActionName(recoveryState.lastAction)
        << " pendingProcessRestart=" << (recoveryState.pendingProcessRestart ? "yes" : "no")
        << " pendingReboot=" << (recoveryState.pendingReboot ? "yes" : "no")
        << " relatchCount=" << recoveryState.relatchCount << "\n";
    out << "radio protocol=" << config.radioProtocol << "\n";
    out << "comm baudrate=" << config.commBaudrate << "\n";
    out << "boot active=" << bootSlotName(bootState.activeSlot)
        << " pending=" << bootSlotName(bootState.pendingSlot)
        << " confirmed=" << (bootState.confirmed ? "yes" : "no")
        << " progress=" << static_cast<unsigned int>(services.getBootUpdateProgress())
        << " confirmTimeoutRemaining=" << services.getBootRemainingConfirmSeconds()
        << " trustStatus=" << bootState.trustStatus
        << " trustRejectReason=" << bootState.trustRejectReason
        << " stagedVersion=" << bootState.stagedSoftwareVersion
        << " activeVersion=" << bootState.activeSoftwareVersion
        << " lastAcceptedVersion=" << bootState.lastAcceptedVersion
        << " signer=" << bootState.signerId
        << " keySlot=" << bootState.keySlot << "\n";
    out << "boot resetCause=" << OBC::resetCauseName(bootState.resetCause)
        << " bootCount=" << bootState.bootCount
        << " consecutiveResetCount=" << bootState.consecutiveResetCount
        << " safeFallbackRequired=" << (bootState.bootSafeFallbackRequired ? "yes" : "no")
        << " lastRecoverySource=" << OBC::recoveryIncidentSourceName(bootState.lastRecoverySource)
        << " lastRecoveryLevel=" << OBC::recoveryLevelName(bootState.lastRecoveryLevel) << "\n";
    out << "groundLink mode=" << OBC::COMM::groundLinkBackendModeName(groundLinkMode);
    if (groundLinkMode == OBC::COMM::GroundLinkBackendMode::DIRECT_TCP) {
        out << " target=" << config.gdsHost << ":" << config.gdsPort;
    } else if (groundLinkMode == OBC::COMM::GroundLinkBackendMode::COMM_CSP) {
        out << " commNode=" << config.commCspNode;
    }
    out << "\n";
    out << "storage persistent=" << config.persistentRoot
        << " staging=" << config.stagingRoot
        << " logs=" << joinPath(config.runtimeRoot, "logs") << "\n";

    if (state.haveEps) {
        out << "eps soc=" << state.eps.soc
            << " vbat=" << state.eps.vbat
            << " tempBat=" << state.eps.temp_bat
            << " pdu=" << static_cast<unsigned int>(state.eps.pdu_status) << "\n";
    } else {
        out << "eps unavailable\n";
    }
    if (state.haveAdcs) {
        out << "adcs mode=" << adcsModeName(OBC::AdcsMode(static_cast<OBC::AdcsMode::T>(state.adcs.mode)))
            << " omega=(" << state.adcs.omega_x << "," << state.adcs.omega_y << "," << state.adcs.omega_z << ")"
            << " pointingErr=" << state.adcs.pointing_error_deg << "\n";
    } else {
        out << "adcs unavailable\n";
    }
    if (haveAdcsPollHealth) {
        out << "adcs fdir transportOk=" << (adcsPollHealth.lastScheduledTransportOk ? "yes" : "no")
            << " validRefresh=" << (adcsPollHealth.lastScheduledValidRefresh ? "yes" : "no")
            << " hasValidState=" << (adcsPollHealth.hasValidState ? "yes" : "no")
            << " transportFailures=" << adcsPollHealth.consecutiveTransportFailures
            << " noValidRefresh=" << adcsPollHealth.consecutiveNoValidRefresh
            << " transportErrors=" << adcsPollHealth.cumulativeTransportErrors
            << " cumulativeNoValidRefresh=" << adcsPollHealth.cumulativeNoValidRefresh << "\n";
    } else {
        out << "adcs fdir unavailable\n";
    }
    if (state.haveGps) {
        out << "gps source=" << gpsSourceModeName(state.gps.sourceMode)
            << " sample=" << (state.gps.hasSample ? "yes" : "no")
            << " fixValid=" << (state.gps.fixValid ? "yes" : "no")
            << " lat=" << state.gps.latitudeDeg
            << " lon=" << state.gps.longitudeDeg
            << " alt=" << state.gps.altitudeMeters
            << " sats=" << static_cast<unsigned int>(state.gps.satelliteCount)
            << " utcSec=" << state.gps.utcSecondsOfDay
            << " date=" << state.gps.utcDateYmd
            << " accepted=" << state.gps.acceptedSentenceCount
            << " rejected=" << state.gps.rejectedSentenceCount << "\n";
    } else {
        out << "gps unavailable\n";
    }
    if (state.haveStorageHealth) {
        out << "storageHealth warning=" << (state.storage.warningActive ? "yes" : "no")
            << " warningMask=0x" << std::hex << static_cast<unsigned int>(state.storage.warningMask)
            << " degradedMask=0x" << static_cast<unsigned int>(state.storage.degradedMask) << std::dec
            << " scans=" << state.storage.scanCount
            << " scanErrors=" << state.storage.scanErrorCount << "\n";
        out << "storage persistent files=" << state.storage.persistent.fileCount
            << " bytes=" << state.storage.persistent.totalBytes << "\n";
        out << "storage staging files=" << state.storage.staging.fileCount
            << " bytes=" << state.storage.staging.totalBytes
            << " logs files=" << state.storage.logs.fileCount
            << " bytes=" << state.storage.logs.totalBytes << "\n";
        out << "storage data-products exists=" << (state.storage.dataProducts.exists ? "yes" : "no")
            << " scanOk=" << (state.storage.dataProducts.scanOk ? "yes" : "no")
            << " files=" << state.storage.dataProducts.fileCount
            << " bytes=" << state.storage.dataProducts.totalBytes
            << " quota=" << state.storage.dataProducts.quotaBytes
            << " quotaStatus=" << static_cast<unsigned int>(state.storage.dataProducts.quotaStatus)
            << " retentionStatus=" << static_cast<unsigned int>(state.storage.dataProducts.retentionStatus)
            << "\n";
    } else {
        out << "storageHealth unavailable\n";
    }
    if (state.haveRadio) {
        out << "radio enabled=" << (state.radio.enabled ? "yes" : "no")
            << " power=" << static_cast<unsigned int>(state.radio.powerDbm)
            << " freq=" << state.radio.freqHz
            << " temp=" << state.radio.temperatureC
            << " rssi=" << state.radio.rssiDbm << "\n";
    } else {
        out << "radio unavailable\n";
    }
    out << "radioObservation sample=" << (state.radioObservation.haveSample ? "yes" : "no")
        << " ageTicks=" << state.radioObservation.statusAgeTicks
        << " result=" << radioObservationResultName(state.radioObservation.lastResult) << "\n";
    out << "groundLinkRaw band=SBAND mode=" << OBC::COMM::groundLinkBackendModeName(sbandObservation.mode)
        << " semantics=" << OBC::COMM::groundLinkHealthSemanticsName(sbandObservation.healthSemantics)
        << " connected=" << (sbandObservation.connected ? "yes" : "no")
        << " txChunks=" << sbandObservation.txChunks
        << " rxChunks=" << sbandObservation.rxChunks
        << " txBytes=" << sbandObservation.txBytes
        << " rxBytes=" << sbandObservation.rxBytes
        << " txErr=" << sbandObservation.txErrors
        << " rxErr=" << sbandObservation.rxErrors
        << " statusObs=" << sbandObservation.successfulStatusObservations << "\n";
    out << "groundLinkRaw band=UHF mode=" << OBC::COMM::groundLinkBackendModeName(uhfObservation.mode)
        << " semantics=" << OBC::COMM::groundLinkHealthSemanticsName(uhfObservation.healthSemantics)
        << " connected=" << (uhfObservation.connected ? "yes" : "no")
        << " txChunks=" << uhfObservation.txChunks
        << " rxChunks=" << uhfObservation.rxChunks
        << " txBytes=" << uhfObservation.txBytes
        << " rxBytes=" << uhfObservation.rxBytes
        << " txErr=" << uhfObservation.txErrors
        << " rxErr=" << uhfObservation.rxErrors
        << " statusObs=" << uhfObservation.successfulStatusObservations << "\n";
    out << "uart connected=" << (uartStats.connected ? "yes" : "no")
        << " tx=" << uartStats.txBytes
        << " rx=" << uartStats.rxBytes
        << " txErr=" << uartStats.txErrors
        << " rxErr=" << uartStats.rxErrors << "\n";
    out << "radioLink connected=" << (radioStats.connected ? "yes" : "no")
        << " tx=" << radioStats.txBytes
        << " rx=" << radioStats.rxBytes
        << " txErr=" << radioStats.txErrors
        << " rxErr=" << radioStats.rxErrors << "\n";
    out << "groundLink connected=" << (groundLinkStats.connected ? "yes" : "no")
        << " tx=" << groundLinkStats.txBytes
        << " rx=" << groundLinkStats.rxBytes
        << " txErr=" << groundLinkStats.txErrors
        << " rxErr=" << groundLinkStats.rxErrors << "\n";
}

void configureComm(const RuntimeConfig& config, RuntimeServices& services) {
    if (config.commMode == "tcp") {
        const std::shared_ptr<OBC::COMM::IByteStreamTransport> link =
            std::make_shared<OBC::COMM::TcpByteStreamTransport>(config.commHost, config.commPort, 300U);
        services.configureCommTransport(link, OBC::COMM::makeSharedMockRadioTransport(link, config.radioProtocol));
        return;
    }

    const std::shared_ptr<OBC::COMM::IByteStreamTransport> link =
        std::make_shared<OBC::COMM::SerialByteStreamTransport>(config.commDevice, 300U, config.commBaudrate);
    services.configureCommTransport(link, OBC::COMM::makeSharedMockRadioTransport(link, config.radioProtocol));
}

bool handleCommand(const std::string& line,
                   RuntimeServices& services,
                   RuntimeState& state,
                   const RuntimeConfig& config,
                   std::ostream& out) {
    std::istringstream input(line);
    std::string scope;
    input >> scope;
    if (scope.empty()) {
        return true;
    }

    for (const CommandEntry& command : COMMAND_TABLE) {
        if (scope == command.name) {
            return command.handler(input, services, state, config, out);
        }
    }

    out << "unknown command\n";
    return true;
}

class RuntimeSession final {
  public:
    explicit RuntimeSession(RuntimeServices& services) : services(services) {}

    RuntimeSession(const RuntimeSession&) = delete;
    RuntimeSession& operator=(const RuntimeSession&) = delete;

    ~RuntimeSession() { cleanup(); }

    void initialize(const RuntimeConfig& config) {
        this->services.announceBoot();
        this->services.setHealthEnabled(true);
        OBC::BootTrustConfig bootTrustConfig;
        bootTrustConfig.algorithm = config.bootTrustMode;
        bootTrustConfig.trustedSignerId = config.bootTrustSignerId;
        bootTrustConfig.trustedKeySlot = config.bootTrustKeySlot;
        bootTrustConfig.trustedKeyHex = config.bootTrustKeyHex;
        this->services.configureBootTrust(bootTrustConfig);
        this->services.configureCommandIngressPersistenceRoot(config.persistentRoot);
        this->services.configurePersistentFaultStorageRoot(config.persistentRoot);
        this->services.configureBootStorageRoots(config.persistentRoot, config.stagingRoot);
        this->services.configureStorageRuntime(config.runtimeRoot, config.persistentRoot, config.stagingRoot);
        this->services.initCspForRuntime(1U);
        this->cspInitialized = true;

        configureComm(config, this->services);
        if (effectiveGroundLinkMode(config) != OBC::COMM::GroundLinkBackendMode::DISABLED) {
            static_cast<void>(this->services.startGroundLink());
        }

        this->rateGroupThread =
            std::thread([&config, this]() { this->services.startRateGroups(tickIntervalFromMs(config.tickMs)); });
    }

    void cleanup() {
        if (this->cleanedUp) {
            return;
        }

        if (this->rateGroupThread.joinable()) {
            this->services.stopRateGroups();
            this->rateGroupThread.join();
        }

        if (this->cspInitialized) {
            this->services.stopGroundLink();
            this->services.joinGroundLink();
            this->services.shutdownCspForRuntime();
        }
        this->cleanedUp = true;
    }

  private:
    RuntimeServices& services;
    std::thread rateGroupThread;
    bool cspInitialized = false;
    bool cleanedUp = false;
};

void initializeRuntime(const RuntimeConfig& config, RuntimeSession& session) {
    session.initialize(config);
}

void printStartupBanner(const RuntimeConfig& config, const StartupBanner& banner, std::ostream& out) {
    out << banner.runtimeStartedLine << "\n";
    for (const std::string& line : banner.extraLines) {
        out << line << "\n";
    }
    out << "Comm mode: " << config.commMode;
    if (config.commMode == "tcp") {
        out << " (" << config.commHost << ":" << config.commPort << ")";
    } else {
        out << " (" << config.commDevice << " @" << config.commBaudrate << ")";
    }
    out << "\n";
    out << "Radio protocol: " << config.radioProtocol << "\n";
    out << "Live beacon UART sink: "
        << (config.radioProtocol == "transparent-passive" ? "transparent-passive" : "disabled") << "\n";
    if (config.uhfBeaconCspNode != 0U) {
        out << "UHF beacon CSP sink: node=" << config.uhfBeaconCspNode
            << " port=" << static_cast<unsigned int>(OBC::COMM::CSP::ServicePort::BEACON_PUSH) << "\n";
    } else {
        out << "UHF beacon CSP sink: disabled\n";
    }
    if (effectiveGroundLinkMode(config) == OBC::COMM::GroundLinkBackendMode::DISABLED) {
        out << "Ground link disabled.\n";
    } else if (effectiveGroundLinkMode(config) == OBC::COMM::GroundLinkBackendMode::DIRECT_TCP) {
        out << "Ground link target: " << config.gdsHost << ":" << config.gdsPort << "\n";
    } else {
        out << "Ground link via COMM CSP node: " << config.commCspNode << "\n";
    }
    out << "Storage roots: persistent=" << config.persistentRoot
        << " staging=" << config.stagingRoot << "\n";
    out << "Hardware watchdog: mode=" << config.hardwareWatchdogMode
        << " device=" << config.hardwareWatchdogDevice
        << " timeoutSec=" << config.hardwareWatchdogTimeoutSec << "\n";
    if (config.headless) {
        out << "Runtime mode: headless\n";
    }
}

OBC::RecoveryExitRequest runCommandLoop(const RuntimeConfig& config,
                                        RuntimeServices& services,
                                        RuntimeState& state,
                                        std::ostream& out) {
    bool running = true;
    OBC::RecoveryExitRequest exitRequest = OBC::RecoveryExitRequest::NONE;
    auto readCommand = [&running, &services, &state, &config, &out]() -> bool {
        std::string line;
        if (!std::getline(std::cin, line)) {
            return false;
        }
        running = handleCommand(line, services, state, config, out);
        return true;
    };

    while (running) {
        if (config.headless) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config.tickMs));
            exitRequest = services.consumeRecoveryExitRequest();
            if (exitRequest != OBC::RecoveryExitRequest::NONE) {
                break;
            }
            sampleHealth(services);
            refreshState(services, state);
            continue;
        }

        struct pollfd pfd;
        pfd.fd = 0;
        pfd.events = POLLIN;
        pfd.revents = 0;

        const int result = ::poll(&pfd, 1, config.tickMs);
        if (result == 0) {
            exitRequest = services.consumeRecoveryExitRequest();
            if (exitRequest != OBC::RecoveryExitRequest::NONE) {
                break;
            }
            sampleHealth(services);
            refreshState(services, state);
            continue;
        }
        if (result < 0) {
            break;
        }
        if ((pfd.revents & POLLIN) != 0) {
            if (!readCommand()) {
                break;
            }
        } else if ((pfd.revents & (POLLHUP | POLLERR | POLLNVAL)) != 0) {
            break;
        }
        exitRequest = services.consumeRecoveryExitRequest();
        if (exitRequest != OBC::RecoveryExitRequest::NONE) {
            break;
        }
        sampleHealth(services);
        refreshState(services, state);
    }
    return exitRequest;
}

int exitCodeForRecoveryExitRequest(const OBC::RecoveryExitRequest request) {
    switch (request) {
        case OBC::RecoveryExitRequest::PROCESS_RESTART:
            return 31;
        case OBC::RecoveryExitRequest::OBC_REBOOT:
            return 32;
        case OBC::RecoveryExitRequest::NONE:
        default:
            return 0;
    }
}

int runHostedRuntime(const RuntimeConfig& config,
                     RuntimeServices& services,
                     const StartupBanner& banner,
                     std::ostream& out,
                     std::ostream&) {
    RuntimeSession session(services);
    initializeRuntime(config, session);

    RuntimeState state;
    refreshState(services, state);
    sampleHealth(services);

    printStartupBanner(config, banner, out);
    const OBC::RecoveryExitRequest exitRequest = runCommandLoop(config, services, state, out);
    session.cleanup();
    return exitCodeForRecoveryExitRequest(exitRequest);
}

}  // namespace Runtime
}  // namespace OBC
