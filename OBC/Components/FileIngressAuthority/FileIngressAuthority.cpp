#include "OBC/Components/FileIngressAuthority/FileIngressAuthority.hpp"

#include "Fw/Com/ComPacket.hpp"
#include "Fw/FilePacket/FilePacket.hpp"
#include "Fw/Types/String.hpp"

#include <algorithm>

namespace OBC {

namespace {

enum class PacketDropReason : U32 {
    DENIED_TRANSFER = 100U,
    INVALID_PACKET = 101U,
    AUTH_REQUIRED = 102U,
    ROLE_DENIED = 103U,
};

std::string pathNameToString(const Fw::FilePacket::PathName& pathName) {
    return std::string(pathName.getValue(), pathName.getValue() + pathName.getLength());
}

bool rewriteStartPacket(Fw::Buffer& wireBuffer,
                        const Fw::FilePacket::StartPacket& originalStart,
                        const std::string& rewrittenDestinationPath) {
    Fw::FilePacket::StartPacket rewrittenStart;
    const std::string sourcePath = pathNameToString(originalStart.getSourcePath());
    rewrittenStart.initialize(originalStart.getFileSize(), sourcePath.c_str(), rewrittenDestinationPath.c_str());

    Fw::FilePacket rewrittenPacket;
    rewrittenPacket.fromStartPacket(rewrittenStart);
    Fw::Buffer packetBuffer(wireBuffer.getData() + sizeof(FwPacketDescriptorType),
                            wireBuffer.getSize() - static_cast<Fw::Buffer::SizeType>(sizeof(FwPacketDescriptorType)));
    return rewrittenPacket.toBuffer(packetBuffer) == Fw::FW_SERIALIZE_OK;
}

}  // namespace

FileIngressAuthority::FileIngressAuthority(const char* compName) : FileIngressAuthorityComponentBase(compName) {
    this->tlmWrite_FILE_INGRESS_ACCEPTED_STARTS(0U);
    this->tlmWrite_FILE_INGRESS_REJECTED_STARTS(0U);
    this->tlmWrite_FILE_INGRESS_DROPPED_PACKETS(0U);
    this->tlmWrite_FILE_INGRESS_DROP_MODE(0U);
    this->tlmWrite_FILE_INGRESS_AUTH_ACTIVE(0U);
    this->tlmWrite_FILE_INGRESS_ALLOWED_PORTS(0U);
}

FileIngressAuthority::~FileIngressAuthority() = default;

bool FileIngressAuthority::configureRuntime(const std::string& runtimeRoot, const std::string& workingRoot) {
    return this->m_policy.configure(runtimeRoot, workingRoot);
}

const FileIngressPolicy& FileIngressAuthority::getPolicy() const {
    return this->m_policy;
}

void FileIngressAuthority::bufferSendIn_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    FW_ASSERT(portNum < this->m_ingressState.size(), static_cast<FwAssertArgType>(portNum));
    if (buffer.getSize() < sizeof(FwPacketDescriptorType)) {
        this->m_droppedPackets++;
        this->tlmWrite_FILE_INGRESS_DROPPED_PACKETS(this->m_droppedPackets);
        this->log_WARNING_LO_FILE_INGRESS_PACKET_DROPPED(static_cast<U32>(Fw::ComPacketType::FW_PACKET_UNKNOWN),
                                                         static_cast<U32>(PacketDropReason::INVALID_PACKET));
        this->returnBuffer(buffer, portNum);
        return;
    }

    FwPacketDescriptorType packetType = static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_UNKNOWN);
    Fw::SerializeStatus status = buffer.getDeserializer().deserializeTo(packetType);
    if (status != Fw::FW_SERIALIZE_OK ||
        packetType != static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_FILE)) {
        this->m_droppedPackets++;
        this->tlmWrite_FILE_INGRESS_DROPPED_PACKETS(this->m_droppedPackets);
        this->log_WARNING_LO_FILE_INGRESS_PACKET_DROPPED(static_cast<U32>(packetType),
                                                         static_cast<U32>(PacketDropReason::INVALID_PACKET));
        this->returnBuffer(buffer, portNum);
        return;
    }

    Fw::Buffer packetBuffer(buffer.getData() + sizeof(packetType),
                            buffer.getSize() - static_cast<Fw::Buffer::SizeType>(sizeof(packetType)));
    Fw::FilePacket filePacket;
    status = filePacket.fromBuffer(packetBuffer);
    if (status != Fw::FW_SERIALIZE_OK) {
        this->m_droppedPackets++;
        this->tlmWrite_FILE_INGRESS_DROPPED_PACKETS(this->m_droppedPackets);
        this->log_WARNING_LO_FILE_INGRESS_PACKET_DROPPED(static_cast<U32>(Fw::ComPacketType::FW_PACKET_FILE),
                                                         static_cast<U32>(PacketDropReason::INVALID_PACKET));
        this->returnBuffer(buffer, portNum);
        return;
    }

    const Fw::FilePacket::Type headerType = filePacket.asHeader().getType();
    if (headerType == Fw::FilePacket::T_START) {
        FileIngressRejectReason reason = FileIngressRejectReason::NONE;
        std::string canonicalPhysicalPath;
        const Fw::FilePacket::StartPacket& startPacket = filePacket.asStartPacket();
        const std::string destinationPath = pathNameToString(startPacket.getDestinationPath());
        IngressRuntimeState& state = this->m_ingressState[portNum];
        if (!this->m_policy.validateDestinationPath(destinationPath, reason, canonicalPhysicalPath)) {
            this->m_rejectedStarts++;
            this->setDropTransfer_(portNum, true);
            this->tlmWrite_FILE_INGRESS_REJECTED_STARTS(this->m_rejectedStarts);
            const Fw::String pathArg(destinationPath.c_str());
            this->log_WARNING_HI_FILE_INGRESS_START_REJECTED(pathArg, static_cast<U32>(reason));
            this->returnBuffer(buffer, portNum);
            return;
        }
        if (!state.secureAuthActive) {
            this->m_rejectedStarts++;
            this->setDropTransfer_(portNum, true);
            this->tlmWrite_FILE_INGRESS_REJECTED_STARTS(this->m_rejectedStarts);
            const Fw::String pathArg(destinationPath.c_str());
            this->log_WARNING_HI_FILE_INGRESS_START_REJECTED(pathArg, static_cast<U32>(PacketDropReason::AUTH_REQUIRED));
            this->returnBuffer(buffer, portNum);
            return;
        }
        if (!state.fileAllowed) {
            this->m_rejectedStarts++;
            this->setDropTransfer_(portNum, true);
            this->tlmWrite_FILE_INGRESS_REJECTED_STARTS(this->m_rejectedStarts);
            const Fw::String pathArg(destinationPath.c_str());
            this->log_WARNING_HI_FILE_INGRESS_START_REJECTED(pathArg, static_cast<U32>(PacketDropReason::ROLE_DENIED));
            this->returnBuffer(buffer, portNum);
            return;
        }

        const std::string leaf = destinationPath.substr(std::strlen(FileIngressPolicy::LOGICAL_PREFIX));
        const std::string rewrittenDestinationPath = this->m_policy.getRuntimePrefix() + leaf;
        if (!rewriteStartPacket(buffer, startPacket, rewrittenDestinationPath)) {
            this->m_rejectedStarts++;
            this->setDropTransfer_(portNum, true);
            this->tlmWrite_FILE_INGRESS_REJECTED_STARTS(this->m_rejectedStarts);
            const Fw::String pathArg(destinationPath.c_str());
            this->log_WARNING_HI_FILE_INGRESS_START_REJECTED(
                pathArg, static_cast<U32>(FileIngressRejectReason::RUNTIME_REWRITE_FAILED));
            this->returnBuffer(buffer, portNum);
            return;
        }

        this->m_acceptedStarts++;
        this->setDropTransfer_(portNum, false);
        this->tlmWrite_FILE_INGRESS_ACCEPTED_STARTS(this->m_acceptedStarts);
        const Fw::String pathArg(destinationPath.c_str());
        this->log_ACTIVITY_LO_FILE_INGRESS_START_ACCEPTED(pathArg);
        this->refreshAuthActivity_(portNum);
        this->forwardBuffer(buffer, portNum);
        return;
    }

    IngressRuntimeState& state = this->m_ingressState[portNum];
    if (state.dropTransfer) {
        this->m_droppedPackets++;
        this->tlmWrite_FILE_INGRESS_DROPPED_PACKETS(this->m_droppedPackets);
        this->log_WARNING_LO_FILE_INGRESS_PACKET_DROPPED(static_cast<U32>(headerType),
                                                         static_cast<U32>(PacketDropReason::DENIED_TRANSFER));
        this->returnBuffer(buffer, portNum);
        return;
    }
    if (!state.secureAuthActive) {
        this->setDropTransfer_(portNum, true);
        this->m_droppedPackets++;
        this->tlmWrite_FILE_INGRESS_DROPPED_PACKETS(this->m_droppedPackets);
        this->log_WARNING_LO_FILE_INGRESS_PACKET_DROPPED(static_cast<U32>(headerType),
                                                         static_cast<U32>(PacketDropReason::AUTH_REQUIRED));
        this->returnBuffer(buffer, portNum);
        return;
    }
    if (!state.fileAllowed) {
        this->setDropTransfer_(portNum, true);
        this->m_droppedPackets++;
        this->tlmWrite_FILE_INGRESS_DROPPED_PACKETS(this->m_droppedPackets);
        this->log_WARNING_LO_FILE_INGRESS_PACKET_DROPPED(static_cast<U32>(headerType),
                                                         static_cast<U32>(PacketDropReason::ROLE_DENIED));
        this->returnBuffer(buffer, portNum);
        return;
    }

    this->refreshAuthActivity_(portNum);
    this->forwardBuffer(buffer, portNum);
}

void FileIngressAuthority::bufferReturnIn_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    static_cast<void>(portNum);
    this->returnBuffer(buffer, this->forgetForwardSource_(buffer));
}

void FileIngressAuthority::authGrantedIn_handler(FwIndexType portNum, const SecureAuthGrant& grant) {
    static_cast<void>(portNum);
    const FwIndexType ingressPort = static_cast<FwIndexType>(grant.get_ingressPort());
    if (ingressPort >= this->m_ingressState.size()) {
        FW_ASSERT(0, static_cast<FwAssertArgType>(grant.get_ingressPort()));
        return;
    }
    IngressRuntimeState& state = this->m_ingressState[ingressPort];
    state.secureAuthActive = true;
    state.serviceId = static_cast<U8>(grant.get_serviceId());
    this->tlmWrite_FILE_INGRESS_AUTH_ACTIVE(1U);
}

void FileIngressAuthority::authRevokedIn_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) {
    static_cast<void>(portNum);
    const FwIndexType ingressPort = static_cast<FwIndexType>(revocation.get_ingressPort());
    if (ingressPort >= this->m_ingressState.size()) {
        FW_ASSERT(0, static_cast<FwAssertArgType>(revocation.get_ingressPort()));
        return;
    }
    IngressRuntimeState& state = this->m_ingressState[ingressPort];
    if (state.secureAuthActive && state.serviceId == static_cast<U8>(revocation.get_serviceId())) {
        state.secureAuthActive = false;
        state.serviceId = 0U;
        this->setDropTransfer_(ingressPort, true);
    }
    const bool anyActive =
        std::any_of(this->m_ingressState.begin(), this->m_ingressState.end(), [](const IngressRuntimeState& ingressState) {
            return ingressState.secureAuthActive;
        });
    this->tlmWrite_FILE_INGRESS_AUTH_ACTIVE(anyActive ? 1U : 0U);
}

void FileIngressAuthority::filePolicyIn_handler(FwIndexType portNum, const FileIngressPolicyState& policy) {
    static_cast<void>(portNum);
    const FwIndexType ingressPort = static_cast<FwIndexType>(policy.get_ingressPort());
    if (ingressPort >= this->m_ingressState.size()) {
        FW_ASSERT(0, static_cast<FwAssertArgType>(policy.get_ingressPort()));
        return;
    }
    IngressRuntimeState& state = this->m_ingressState[ingressPort];
    state.fileAllowed = policy.get_fileAllowed();
    if (!state.fileAllowed) {
        this->setDropTransfer_(ingressPort, true);
    }
    this->tlmWrite_FILE_INGRESS_ALLOWED_PORTS(this->allowedPortCount_());
}

void FileIngressAuthority::setDropTransfer_(FwIndexType portNum, bool enabled) {
    FW_ASSERT(portNum < this->m_ingressState.size(), static_cast<FwAssertArgType>(portNum));
    this->m_ingressState[portNum].dropTransfer = enabled;
    this->tlmWrite_FILE_INGRESS_DROP_MODE(this->isAnyDropTransferActive_() ? 1U : 0U);
}

bool FileIngressAuthority::isAnyDropTransferActive_() const {
    for (const IngressRuntimeState& state : this->m_ingressState) {
        if (state.dropTransfer) {
            return true;
        }
    }
    return false;
}

void FileIngressAuthority::refreshAuthActivity_(FwIndexType portNum) {
    const IngressRuntimeState& state = this->m_ingressState[portNum];
    if (!state.secureAuthActive || state.serviceId == 0U || !this->isConnected_authActivityOut_OutputPort(0)) {
        return;
    }
    SecureAuthActivity activity;
    activity.set_ingressPort(static_cast<U32>(portNum));
    activity.set_serviceId(state.serviceId);
    activity.set_sequenceNumber(0U);
    this->authActivityOut_out(0, activity);
}

U32 FileIngressAuthority::allowedPortCount_() const {
    U32 count = 0U;
    for (const IngressRuntimeState& state : this->m_ingressState) {
        if (state.fileAllowed) {
            count += 1U;
        }
    }
    return count;
}

void FileIngressAuthority::returnBuffer(Fw::Buffer& buffer, FwIndexType portNum) {
    this->bufferReturnOut_out(portNum, buffer);
}

void FileIngressAuthority::rememberForwardSource_(const Fw::Buffer& buffer, FwIndexType portNum) {
    this->m_forwardOrigins[buffer.getData()] = portNum;
}

FwIndexType FileIngressAuthority::forgetForwardSource_(const Fw::Buffer& buffer) {
    const U8* const key = buffer.getData();
    const auto it = this->m_forwardOrigins.find(key);
    FW_ASSERT(it != this->m_forwardOrigins.end());
    const FwIndexType portNum = it->second;
    this->m_forwardOrigins.erase(it);
    return portNum;
}

void FileIngressAuthority::forwardBuffer(Fw::Buffer& buffer, FwIndexType portNum) {
    this->rememberForwardSource_(buffer, portNum);
    this->bufferSendOut_out(0, buffer);
}

}  // namespace OBC
