#include "OBC/Components/SecureLinkAuthorizer/SecureLinkAuthorizer.hpp"

#include "Fw/Com/ComPacket.hpp"

#include <algorithm>
#include <cstring>
#include <random>

namespace OBC {

namespace {

constexpr char DEFAULT_MODULE_SERIAL[] = "FPOBCSAT00000001";

bool copyFixedSerial(const char* source, U8 dest[SECURE_LINK_MODULE_SERIAL_SIZE]) {
    if (source == nullptr || std::strlen(source) != SECURE_LINK_MODULE_SERIAL_SIZE) {
        return false;
    }
    std::memcpy(dest, source, SECURE_LINK_MODULE_SERIAL_SIZE);
    return true;
}

bool computeElapsedTime(const Fw::Time& now, const Fw::Time& then, Fw::Time& elapsed) {
    const Fw::Time::Comparison comparison = Fw::Time::compare(now, then);
    if (comparison == Fw::Time::INCOMPARABLE || comparison == Fw::Time::LT) {
        return false;
    }
    elapsed = Fw::Time::sub(now, then);
    return true;
}

U32 remainingTimeoutSeconds(const Fw::Time& elapsed) {
    if (elapsed.getSeconds() >= SECURE_LINK_AUTH_TIMEOUT_SECONDS) {
        return 0U;
    }
    return SECURE_LINK_AUTH_TIMEOUT_SECONDS - elapsed.getSeconds();
}

void unwrapHandshakePacketPayload(const U8*& data, FwSizeType& dataSize) {
    if (data == nullptr || dataSize < sizeof(FwPacketDescriptorType)) {
        return;
    }

    Fw::ExternalSerializeBuffer buffer(const_cast<U8*>(data), dataSize);
    if (buffer.setBuffLen(dataSize) != Fw::FW_SERIALIZE_OK) {
        return;
    }

    FwPacketDescriptorType descriptor = 0U;
    if (buffer.deserializeTo(descriptor) != Fw::FW_SERIALIZE_OK ||
        descriptor != static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_HAND)) {
        return;
    }

    data = buffer.getBuffAddrLeft();
    dataSize = buffer.getDeserializeSizeLeft();
}

}  // namespace

SecureLinkAuthorizer::SecureLinkAuthorizer(const char* compName) : SecureLinkAuthorizerComponentBase(compName) {
    static_assert(sizeof(DEFAULT_MODULE_SERIAL) - 1U == SECURE_LINK_MODULE_SERIAL_SIZE, "Unexpected module serial size");
    static_cast<void>(copyFixedSerial(DEFAULT_MODULE_SERIAL, this->m_moduleSerial.data()));
    this->tlmWrite_SECURE_AUTH_ACTIVE(0U);
    this->tlmWrite_SECURE_AUTH_ACTIVE_PORT(0U);
    this->tlmWrite_SECURE_AUTH_ACTIVE_SERVICE(0U);
    this->tlmWrite_SECURE_AUTH_LAST_STATUS(0U);
    this->tlmWrite_SECURE_AUTH_LAST_REJECT_REASON(0U);
    this->tlmWrite_SECURE_AUTH_CHALLENGE_TOTAL(0U);
    this->tlmWrite_SECURE_AUTH_ESTABLISHED_TOTAL(0U);
    this->tlmWrite_SECURE_AUTH_REVOKE_TOTAL(0U);
    this->tlmWrite_SECURE_AUTH_TIMEOUT_REMAINING(0U);
}

SecureLinkAuthorizer::~SecureLinkAuthorizer() = default;

bool SecureLinkAuthorizer::configureIngressService(FwIndexType ingressPort,
                                                   U8 serviceId,
                                                   const U8* keyBytes,
                                                   FwSizeType keyLength) {
    if (ingressPort >= PORT_COUNT || !isSupportedSecureServiceId(serviceId) || keyBytes == nullptr ||
        keyLength == 0U || keyLength > this->m_ingressKeys[ingressPort].keyBytes.size()) {
        return false;
    }

    IngressKeyConfig& config = this->m_ingressKeys[ingressPort];
    config = IngressKeyConfig();
    config.valid = true;
    config.serviceId = serviceId;
    config.keyLength = keyLength;
    std::memcpy(config.keyBytes.data(), keyBytes, keyLength);
    return true;
}

bool SecureLinkAuthorizer::configureModuleSerial(const char* moduleSerial) {
    return copyFixedSerial(moduleSerial, this->m_moduleSerial.data());
}

void SecureLinkAuthorizer::handshakeUplinkIn_handler(FwIndexType portNum,
                                                     Fw::Buffer& data,
                                                     const ComCfg::FrameContext& context) {
    static_cast<void>(context);
    FW_ASSERT(portNum < PORT_COUNT, portNum);

    const U8* handshakeData = data.getData();
    FwSizeType handshakeSize = data.getSize();
    unwrapHandshakePacketPayload(handshakeData, handshakeSize);

    const SecureHandshakeParseResult parsed = parseSecureHandshakePacket(handshakeData, handshakeSize);
    if (!parsed.valid) {
        this->tlmWrite_SECURE_AUTH_LAST_REJECT_REASON(static_cast<U32>(parsed.reason));
        this->log_WARNING_HI_SECURE_AUTH_PACKET_REJECTED(static_cast<U32>(portNum), 0U, static_cast<U32>(parsed.reason));
        this->bufferReturnOut_out(portNum, data);
        return;
    }

    const U8 serviceId = parsed.message.serviceId;
    const IngressKeyConfig& keyConfig = this->m_ingressKeys[portNum];
    if (!keyConfig.valid || keyConfig.serviceId != serviceId) {
        this->tlmWrite_SECURE_AUTH_LAST_REJECT_REASON(static_cast<U32>(SecureHandshakeParseReason::BAD_SERVICE_ID));
        this->emitAuthStatus_(portNum, serviceId, static_cast<U32>(SecureAuthStatusCode::UNSUPPORTED_SERVICE));
        this->log_WARNING_HI_SECURE_AUTH_PACKET_REJECTED(
            static_cast<U32>(portNum), serviceId, static_cast<U32>(SecureHandshakeParseReason::BAD_SERVICE_ID));
        this->bufferReturnOut_out(portNum, data);
        return;
    }

    switch (parsed.message.type) {
        case SecureHandshakeMessageType::REQ_AUTH:
            static_cast<void>(this->issueChallenge_(portNum, serviceId));
            break;
        case SecureHandshakeMessageType::RESPONSE:
            static_cast<void>(this->establishAuth_(portNum, serviceId, parsed.message.response.data()));
            break;
        case SecureHandshakeMessageType::CHALLENGE:
        case SecureHandshakeMessageType::AUTH_STATUS:
            this->tlmWrite_SECURE_AUTH_LAST_REJECT_REASON(static_cast<U32>(SecureHandshakeParseReason::BAD_MESSAGE_TYPE));
            this->log_WARNING_HI_SECURE_AUTH_PACKET_REJECTED(
                static_cast<U32>(portNum), serviceId, static_cast<U32>(SecureHandshakeParseReason::BAD_MESSAGE_TYPE));
            break;
    }

    this->bufferReturnOut_out(portNum, data);
}

void SecureLinkAuthorizer::authActivityIn_handler(FwIndexType portNum, const SecureAuthActivity& activity) {
    FW_ASSERT(portNum < this->getNum_authActivityIn_InputPorts(), portNum);
    FW_ASSERT(activity.get_ingressPort() < PORT_COUNT, activity.get_ingressPort());
    ActiveAuthState& active = this->m_activeAuth[activity.get_ingressPort()];
    if (active.active && active.serviceId == activity.get_serviceId()) {
        active.lastActivityTime = this->getTime();
        this->publishActiveAuth_();
    }
}

void SecureLinkAuthorizer::authInvalidateIn_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) {
    static_cast<void>(portNum);
    FW_ASSERT(revocation.get_ingressPort() < PORT_COUNT, revocation.get_ingressPort());
    const FwIndexType ingressPort = revocation.get_ingressPort();
    if (this->m_pendingChallenges[ingressPort].active &&
        this->m_pendingChallenges[ingressPort].serviceId == revocation.get_serviceId()) {
        this->clearPendingChallenge_(ingressPort);
    }
    if (this->m_activeAuth[ingressPort].active && this->m_activeAuth[ingressPort].serviceId == revocation.get_serviceId()) {
        this->revokeActiveAuth_(ingressPort, revocation.get_reason(), true);
    }
    this->publishActiveAuth_();
}

void SecureLinkAuthorizer::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    const Fw::Time now = this->getTime();
    for (FwIndexType ingressPort = 0; ingressPort < PORT_COUNT; ++ingressPort) {
        ActiveAuthState& active = this->m_activeAuth[ingressPort];
        if (!active.active) {
            continue;
        }
        Fw::Time elapsed = Fw::ZERO_TIME;
        if (!computeElapsedTime(now, active.lastActivityTime, elapsed)) {
            active.lastActivityTime = now;
            continue;
        }
        if (elapsed.getSeconds() >= SECURE_LINK_AUTH_TIMEOUT_SECONDS) {
            this->revokeActiveAuth_(ingressPort, static_cast<U32>(SecureAuthRevocationReason::TIMEOUT), true);
        }
    }
    this->publishActiveAuth_();
}

bool SecureLinkAuthorizer::issueChallenge_(FwIndexType ingressPort, U8 serviceId) {
    U8 randomNonce[SECURE_LINK_RANDOM_NONCE_SIZE] = {};
    if (!this->fillRandomNonce_(randomNonce)) {
        this->emitAuthStatus_(ingressPort, serviceId, static_cast<U32>(SecureAuthStatusCode::INTERNAL_ERROR));
        return false;
    }

    PendingChallengeState& pending = this->m_pendingChallenges[ingressPort];
    pending = PendingChallengeState();
    pending.active = true;
    pending.serviceId = serviceId;
    std::memcpy(pending.challenge.data(), this->m_moduleSerial.data(), SECURE_LINK_MODULE_SERIAL_SIZE);
    std::memcpy(pending.challenge.data() + SECURE_LINK_MODULE_SERIAL_SIZE, randomNonce, SECURE_LINK_RANDOM_NONCE_SIZE);

    Fw::ComBuffer packet;
    if (!buildSecureChallengePacket(serviceId, pending.challenge.data(), packet)) {
        this->emitAuthStatus_(ingressPort, serviceId, static_cast<U32>(SecureAuthStatusCode::INTERNAL_ERROR));
        return false;
    }

    this->m_challengeTotal++;
    this->tlmWrite_SECURE_AUTH_CHALLENGE_TOTAL(this->m_challengeTotal);
    this->handshakePacketOut_out(ingressPort, packet, 0U);
    this->log_ACTIVITY_HI_SECURE_AUTH_CHALLENGE_ISSUED(static_cast<U32>(ingressPort), serviceId);
    return true;
}

bool SecureLinkAuthorizer::establishAuth_(FwIndexType ingressPort,
                                          U8 serviceId,
                                          const U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE]) {
    const PendingChallengeState& pending = this->m_pendingChallenges[ingressPort];
    const IngressKeyConfig& keyConfig = this->m_ingressKeys[ingressPort];
    if (!pending.active || pending.serviceId != serviceId) {
        this->emitAuthStatus_(ingressPort, serviceId, static_cast<U32>(SecureAuthStatusCode::NOT_AUTHENTICATED));
        return false;
    }

    U8 sessionKey[SECURE_LINK_SESSION_KEY_SIZE] = {};
    if (!deriveSecureSessionKey(
            keyConfig.keyBytes.data(), keyConfig.keyLength, serviceId, pending.challenge.data(), sessionKey) ||
        !verifySecureAuthResponse(sessionKey, response)) {
        this->emitAuthStatus_(ingressPort, serviceId, static_cast<U32>(SecureAuthStatusCode::NOT_AUTHENTICATED));
        return false;
    }

    ActiveAuthState& active = this->m_activeAuth[ingressPort];
    active = ActiveAuthState();
    active.active = true;
    active.serviceId = serviceId;
    std::memcpy(active.sessionKey.data(), sessionKey, sizeof(sessionKey));
    active.lastActivityTime = this->getTime();
    this->clearPendingChallenge_(ingressPort);

    SecureAuthGrant grant;
    grant.set_ingressPort(static_cast<U32>(ingressPort));
    grant.set_serviceId(serviceId);
    SecureSessionKey portKey;
    for (FwSizeType i = 0; i < portKey.SIZE; i++) {
        portKey[i] = active.sessionKey[i];
    }
    grant.set_sessionKey(portKey);
    for (FwIndexType portNum = 0; portNum < this->getNum_authGrantedOut_OutputPorts(); portNum++) {
        if (this->isConnected_authGrantedOut_OutputPort(portNum)) {
            this->authGrantedOut_out(portNum, grant);
        }
    }

    this->m_establishedTotal++;
    this->tlmWrite_SECURE_AUTH_ESTABLISHED_TOTAL(this->m_establishedTotal);
    this->emitAuthStatus_(ingressPort, serviceId, static_cast<U32>(SecureAuthStatusCode::AUTHENTICATED));
    this->log_ACTIVITY_HI_SECURE_AUTH_ESTABLISHED(static_cast<U32>(ingressPort), serviceId);
    this->publishActiveAuth_();
    return true;
}

void SecureLinkAuthorizer::clearPendingChallenge_(FwIndexType ingressPort) {
    this->m_pendingChallenges[ingressPort] = PendingChallengeState();
}

void SecureLinkAuthorizer::clearActiveAuth_(FwIndexType ingressPort) {
    this->m_activeAuth[ingressPort] = ActiveAuthState();
}

void SecureLinkAuthorizer::revokeActiveAuth_(FwIndexType ingressPort, U32 reason, bool notifyCommandGate) {
    ActiveAuthState& active = this->m_activeAuth[ingressPort];
    if (!active.active) {
        return;
    }

    const U8 serviceId = active.serviceId;
    this->m_revokeTotal++;
    this->tlmWrite_SECURE_AUTH_REVOKE_TOTAL(this->m_revokeTotal);
    if (notifyCommandGate) {
        SecureAuthRevocation revocation;
        revocation.set_ingressPort(static_cast<U32>(ingressPort));
        revocation.set_serviceId(serviceId);
        revocation.set_reason(reason);
        for (FwIndexType portNum = 0; portNum < this->getNum_authRevokedOut_OutputPorts(); portNum++) {
            if (this->isConnected_authRevokedOut_OutputPort(portNum)) {
                this->authRevokedOut_out(portNum, revocation);
            }
        }
    }
    this->clearActiveAuth_(ingressPort);
    this->log_ACTIVITY_HI_SECURE_AUTH_REVOKED(static_cast<U32>(ingressPort), serviceId, reason);
}

void SecureLinkAuthorizer::publishActiveAuth_() {
    const Fw::Time now = this->getTime();
    for (FwIndexType ingressPort = 0; ingressPort < PORT_COUNT; ++ingressPort) {
        const ActiveAuthState& active = this->m_activeAuth[ingressPort];
        if (active.active) {
            Fw::Time elapsed = Fw::ZERO_TIME;
            this->tlmWrite_SECURE_AUTH_ACTIVE(1U);
            this->tlmWrite_SECURE_AUTH_ACTIVE_PORT(static_cast<U32>(ingressPort));
            this->tlmWrite_SECURE_AUTH_ACTIVE_SERVICE(active.serviceId);
            if (computeElapsedTime(now, active.lastActivityTime, elapsed)) {
                this->tlmWrite_SECURE_AUTH_TIMEOUT_REMAINING(remainingTimeoutSeconds(elapsed));
            } else {
                this->tlmWrite_SECURE_AUTH_TIMEOUT_REMAINING(SECURE_LINK_AUTH_TIMEOUT_SECONDS);
            }
            return;
        }
    }
    this->tlmWrite_SECURE_AUTH_ACTIVE(0U);
    this->tlmWrite_SECURE_AUTH_ACTIVE_PORT(0U);
    this->tlmWrite_SECURE_AUTH_ACTIVE_SERVICE(0U);
    this->tlmWrite_SECURE_AUTH_TIMEOUT_REMAINING(0U);
}

void SecureLinkAuthorizer::emitAuthStatus_(FwIndexType ingressPort, U8 serviceId, U32 statusCode) {
    Fw::ComBuffer packet;
    if (buildSecureAuthStatusPacket(serviceId, statusCode, packet)) {
        this->handshakePacketOut_out(ingressPort, packet, 0U);
    }
    this->tlmWrite_SECURE_AUTH_LAST_STATUS(statusCode);
}

bool SecureLinkAuthorizer::fillRandomNonce_(U8 randomNonce[SECURE_LINK_RANDOM_NONCE_SIZE]) {
    if (randomNonce == nullptr) {
        return false;
    }
    std::random_device device;
    for (FwSizeType i = 0; i < SECURE_LINK_RANDOM_NONCE_SIZE; i++) {
        randomNonce[i] = static_cast<U8>(device() & 0xFFU);
    }
    return true;
}

}  // namespace OBC
