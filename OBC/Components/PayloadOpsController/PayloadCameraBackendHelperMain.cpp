#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include "OBC/Components/PayloadOpsController/PayloadBackendHelperProtocol.hpp"
#include "OBC/Components/PayloadOpsController/PayloadBackendHelperProtocolUtils.hpp"
#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"

namespace {

template <typename T>
bool readExact(T& value) {
    std::uint8_t* bytes = reinterpret_cast<std::uint8_t*>(&value);
    std::size_t offset = 0U;
    while (offset < sizeof(T)) {
        const ssize_t count = ::read(STDIN_FILENO, bytes + offset, sizeof(T) - offset);
        if (count <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(count);
    }
    return true;
}

template <typename T>
bool writeExact(const T& value) {
    const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(&value);
    std::size_t offset = 0U;
    while (offset < sizeof(T)) {
        const ssize_t count = ::write(STDOUT_FILENO, bytes + offset, sizeof(T) - offset);
        if (count <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(count);
    }
    return true;
}

void initializeReply(OBC::PayloadHelperProtocol::Reply& reply, OBC::PayloadHelperProtocol::OpCode opCode) {
    reply = {};
    reply.header.magic = OBC::PayloadHelperProtocol::MAGIC;
    reply.header.version = OBC::PayloadHelperProtocol::VERSION;
    reply.header.opCode = static_cast<std::uint16_t>(opCode);
    reply.response = static_cast<std::uint32_t>(Fw::CmdResponse::EXECUTION_ERROR);
}

}  // namespace

int main() {
    std::unique_ptr<OBC::IPiCameraDriver> driver = OBC::makePayloadHelperBackendDriver();
    if (driver == nullptr) {
        std::cerr << "payload helper backend unavailable\n";
        return 2;
    }

    while (true) {
        OBC::PayloadHelperProtocol::Header header = {};
        if (!readExact(header)) {
            break;
        }
        if (header.magic != OBC::PayloadHelperProtocol::MAGIC ||
            header.version != OBC::PayloadHelperProtocol::VERSION) {
            break;
        }

        const auto opCode = static_cast<OBC::PayloadHelperProtocol::OpCode>(header.opCode);
        OBC::PayloadHelperProtocol::Reply reply = {};
        initializeReply(reply, opCode);
        U32 detailCode = 0U;

        switch (opCode) {
            case OBC::PayloadHelperProtocol::OpCode::GET_CAPABILITIES: {
                reply.response = static_cast<std::uint32_t>(Fw::CmdResponse::OK);
                reply.capabilities = OBC::PayloadHelperProtocolUtils::encodeCapabilities(driver->getCapabilities());
                break;
            }
            case OBC::PayloadHelperProtocol::OpCode::PREPARE: {
                OBC::PayloadHelperProtocol::PrepareRequest request = {};
                request.header = header;
                if (!readExact(request.settings) || !readExact(request.initTimeoutMs)) {
                    return 3;
                }
                const OBC::PayloadCameraSettings settings =
                    OBC::PayloadHelperProtocolUtils::decodeSettings(request.settings);
                const std::atomic<bool> cancelRequested(false);
                const Fw::CmdResponse response = driver->prepare(
                    settings.readyKind, settings, request.initTimeoutMs, cancelRequested, detailCode);
                reply.response = static_cast<std::uint32_t>(response);
                break;
            }
            case OBC::PayloadHelperProtocol::OpCode::CAPTURE: {
                OBC::PayloadHelperProtocol::CaptureRequestWire request = {};
                request.header = header;
                if (!readExact(request.settings) || !readExact(request.captureTimeoutMs) ||
                    !readExact(request.captureId) || !readExact(request.captureIndex) || !readExact(request.reserved0) ||
                    !readExact(request.bootCount) || !readExact(request.requestedMask) || !readExact(request.appliedMask) ||
                    !readExact(request.tag) || !readExact(request.rawOutputPath) || !readExact(request.previewOutputPath) ||
                    !readExact(request.rawRelativePath) || !readExact(request.previewRelativePath)) {
                    return 4;
                }
                OBC::PayloadCaptureRequest captureRequest = {};
                captureRequest.tag = OBC::PayloadHelperProtocolUtils::decodeBoundedString(request.tag);
                captureRequest.captureId = request.captureId;
                captureRequest.captureIndex = request.captureIndex;
                captureRequest.bootCount = request.bootCount;
                captureRequest.requestedMask = request.requestedMask;
                captureRequest.appliedMask = request.appliedMask;
                captureRequest.rawOutputPath = OBC::PayloadHelperProtocolUtils::decodeBoundedString(request.rawOutputPath);
                captureRequest.previewOutputPath =
                    OBC::PayloadHelperProtocolUtils::decodeBoundedString(request.previewOutputPath);
                captureRequest.rawRelativePath = OBC::PayloadHelperProtocolUtils::decodeBoundedString(request.rawRelativePath);
                captureRequest.previewRelativePath =
                    OBC::PayloadHelperProtocolUtils::decodeBoundedString(request.previewRelativePath);

                OBC::PayloadCaptureMetadata metadata = {};
                metadata.capturePolicy = static_cast<OBC::PayloadCapturePolicy::T>(request.settings.capturePolicy);
                metadata.captureId = request.captureId;
                metadata.captureIndex = request.captureIndex;
                metadata.bootCount = request.bootCount;
                metadata.requestedMask = request.requestedMask;
                metadata.appliedMask = request.appliedMask;
                metadata.requestedSettings = OBC::PayloadHelperProtocolUtils::decodeSettings(request.settings);
                metadata.rawRelativePath = captureRequest.rawRelativePath;
                metadata.previewRelativePath = captureRequest.previewRelativePath;
                captureRequest.metadata = metadata;

                const OBC::PayloadCameraSettings settings =
                    OBC::PayloadHelperProtocolUtils::decodeSettings(request.settings);
                const std::atomic<bool> cancelRequested(false);
                const Fw::CmdResponse response =
                    driver->captureStill(captureRequest, settings, request.captureTimeoutMs, cancelRequested, metadata, detailCode);
                reply.response = static_cast<std::uint32_t>(response);
                reply.metadata = OBC::PayloadHelperProtocolUtils::encodeMetadata(metadata);
                break;
            }
            case OBC::PayloadHelperProtocol::OpCode::SHUTDOWN: {
                const Fw::CmdResponse response = driver->shutdown(detailCode);
                reply.response = static_cast<std::uint32_t>(response);
                break;
            }
            case OBC::PayloadHelperProtocol::OpCode::READ_REGISTER: {
                OBC::PayloadHelperProtocol::RegisterReadRequest request = {};
                request.header = header;
                if (!readExact(request.address) || !readExact(request.timeoutMs)) {
                    return 5;
                }
                U32 value = 0U;
                const Fw::CmdResponse response =
                    driver->readSensorRegister(request.address, request.timeoutMs, value, detailCode);
                reply.response = static_cast<std::uint32_t>(response);
                reply.value = value;
                break;
            }
            case OBC::PayloadHelperProtocol::OpCode::WRITE_REGISTER: {
                OBC::PayloadHelperProtocol::RegisterWriteRequest request = {};
                request.header = header;
                if (!readExact(request.address) || !readExact(request.value) || !readExact(request.timeoutMs) ||
                    !readExact(request.verifyReadback) || !readExact(request.reserved0)) {
                    return 6;
                }
                U32 readbackValue = 0U;
                const Fw::CmdResponse response = driver->writeSensorRegister(
                    request.address, request.value, request.verifyReadback != 0U, request.timeoutMs, readbackValue, detailCode);
                reply.response = static_cast<std::uint32_t>(response);
                reply.readbackValue = readbackValue;
                break;
            }
            default:
                reply.response = static_cast<std::uint32_t>(Fw::CmdResponse::VALIDATION_ERROR);
                detailCode = 1U;
                break;
        }

        reply.detailCode = detailCode;
        if (!writeExact(reply)) {
            return 7;
        }
    }

    return 0;
}
