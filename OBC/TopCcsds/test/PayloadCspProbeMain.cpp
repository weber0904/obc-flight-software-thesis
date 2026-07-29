#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "OBC/TopCcsds/PayloadCspProtocol.hpp"
#include "simulators/csp/CspRuntime.hpp"

namespace {

std::uint16_t parsePort(const char* value, std::uint16_t fallback) {
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    return static_cast<std::uint16_t>(std::strtoul(value, nullptr, 10));
}

}  // namespace

int main(int argc, char* argv[]) {
    std::uint16_t localNode = 8U;
    std::uint16_t targetNode = OBCApp::PayloadCSP::FIRST_LOCAL_SERVICE_NODE_ID;
    std::uint32_t timeoutMs = 1000U;
    std::uint32_t expectCaptureIdMin = 0U;
    OBC::CSP::RuntimeConfig runtimeConfig = OBC::CSP::runtimeConfigFromEnvironment(localNode, "PAYLCSP");
    runtimeConfig.nodeId = localNode;
    runtimeConfig.interfaceName = "PAYLCSP";

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--local-node" && i + 1 < argc) {
            localNode = static_cast<std::uint16_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--target-node" && i + 1 < argc) {
            targetNode = static_cast<std::uint16_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--timeout-ms" && i + 1 < argc) {
            timeoutMs = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--expect-capture-id-min" && i + 1 < argc) {
            expectCaptureIdMin = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--transport" && i + 1 < argc) {
            runtimeConfig.transportKind = argv[++i];
        } else if (arg == "--hub-host" && i + 1 < argc) {
            runtimeConfig.hubHost = argv[++i];
        } else if (arg == "--hub-sub-port" && i + 1 < argc) {
            runtimeConfig.hubSubPort = parsePort(argv[++i], runtimeConfig.hubSubPort);
        } else if (arg == "--hub-pub-port" && i + 1 < argc) {
            runtimeConfig.hubPubPort = parsePort(argv[++i], runtimeConfig.hubPubPort);
        } else if (arg == "--interface-name" && i + 1 < argc) {
            runtimeConfig.interfaceName = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    runtimeConfig.nodeId = localNode;

    OBC::CSP::LibCspRuntime runtime;
    if (runtime.init(runtimeConfig) != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Failed to initialize payload CSP probe runtime\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto shutdown = [&runtime]() { runtime.shutdown(); };

    OBCApp::PayloadCSP::BasicRequest request = OBCApp::PayloadCSP::makeBasicRequest(OBCApp::PayloadCSP::ServicePort::STATUS, 1U);
    OBCApp::PayloadCSP::StatusReply statusReply = {};
    std::size_t replySize = 0U;
    if (runtime.requestReply(targetNode,
                             static_cast<std::uint8_t>(OBCApp::PayloadCSP::ServicePort::STATUS),
                             &request,
                             sizeof(request),
                             &statusReply,
                             sizeof(statusReply),
                             replySize,
                             timeoutMs) != OBC::CSP::RuntimeStatus::OK ||
        replySize != sizeof(statusReply) ||
        statusReply.header.result != static_cast<std::uint8_t>(OBCApp::PayloadCSP::ResultCode::OK)) {
        shutdown();
        std::cerr << "STATUS request failed\n";
        return 1;
    }

    request = OBCApp::PayloadCSP::makeBasicRequest(OBCApp::PayloadCSP::ServicePort::CAPABILITIES, 2U);
    OBCApp::PayloadCSP::CapabilitiesReply capabilitiesReply = {};
    replySize = 0U;
    if (runtime.requestReply(targetNode,
                             static_cast<std::uint8_t>(OBCApp::PayloadCSP::ServicePort::CAPABILITIES),
                             &request,
                             sizeof(request),
                             &capabilitiesReply,
                             sizeof(capabilitiesReply),
                             replySize,
                             timeoutMs) != OBC::CSP::RuntimeStatus::OK ||
        replySize != sizeof(capabilitiesReply) ||
        capabilitiesReply.header.result != static_cast<std::uint8_t>(OBCApp::PayloadCSP::ResultCode::OK)) {
        shutdown();
        std::cerr << "CAPABILITIES request failed\n";
        return 1;
    }

    request = OBCApp::PayloadCSP::makeBasicRequest(OBCApp::PayloadCSP::ServicePort::LAST_CAPTURE_METADATA, 3U);
    OBCApp::PayloadCSP::MetadataReply metadataReply = {};
    replySize = 0U;
    if (runtime.requestReply(targetNode,
                             static_cast<std::uint8_t>(OBCApp::PayloadCSP::ServicePort::LAST_CAPTURE_METADATA),
                             &request,
                             sizeof(request),
                             &metadataReply,
                             sizeof(metadataReply),
                             replySize,
                             timeoutMs) != OBC::CSP::RuntimeStatus::OK ||
        replySize != sizeof(metadataReply) ||
        metadataReply.header.result != static_cast<std::uint8_t>(OBCApp::PayloadCSP::ResultCode::OK)) {
        shutdown();
        std::cerr << "LAST_CAPTURE_METADATA request failed\n";
        return 1;
    }

    if (metadataReply.captureId < expectCaptureIdMin) {
        shutdown();
        std::cerr << "captureId " << metadataReply.captureId << " is below expected minimum " << expectCaptureIdMin << "\n";
        return 1;
    }

    std::cout << "STATUS state=" << static_cast<unsigned int>(statusReply.state)
              << " prepared_ready=" << static_cast<unsigned int>(statusReply.preparedReadyKind)
              << " last_result=" << static_cast<unsigned int>(statusReply.lastResult)
              << " capture_id=" << statusReply.lastCaptureId
              << " flags=0x" << std::hex << static_cast<unsigned int>(statusReply.header.flags) << std::dec << "\n";
    std::cout << "CAPABILITIES supported_mask=0x" << std::hex << capabilitiesReply.supportedMask
              << " off_only_mask=0x" << capabilitiesReply.offOnlyMask
              << " auto_mask=0x" << capabilitiesReply.autoMutableMask
              << " deterministic_mask=0x" << capabilitiesReply.deterministicMutableMask << std::dec
              << " backend=" << capabilitiesReply.backendName
              << " camera=" << capabilitiesReply.cameraModel
              << " flags=0x" << std::hex << static_cast<unsigned int>(capabilitiesReply.header.flags) << std::dec << "\n";
    std::cout << "METADATA capture_id=" << metadataReply.captureId
              << " capture_index=" << static_cast<unsigned int>(metadataReply.captureIndex)
              << " capture_policy=" << static_cast<unsigned int>(metadataReply.capturePolicy)
              << " result=" << static_cast<unsigned int>(metadataReply.resultCode)
              << " exposure_usec=" << metadataReply.exposureUsec
              << " gain_x100=" << metadataReply.gainX100
              << " raw_path=" << metadataReply.rawRelativePath
              << " preview_path=" << metadataReply.previewRelativePath
              << " preview_dp=" << metadataReply.previewDataProductPath
              << " raw_dp=" << metadataReply.rawDataProductPath << "\n";

    shutdown();
    return 0;
}
