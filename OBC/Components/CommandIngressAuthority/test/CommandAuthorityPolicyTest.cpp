#include <gtest/gtest.h>

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityCatalog.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandIngressAuthority.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandSessionSequence.hpp"
#include "OBC/Components/SequenceAdmissionController/OfficialSequenceOpcodes.hpp"

namespace {

constexpr FwOpcodeType OPCODE_MODE_SET = 268632064U;
constexpr FwOpcodeType OPCODE_MODE_GET = 268632065U;
constexpr FwOpcodeType OPCODE_EPS_GET_STATUS = 268644352U;
constexpr FwOpcodeType OPCODE_ADCS_GET_ATTITUDE = 268648450U;
constexpr FwOpcodeType OPCODE_GPS_GET_STATE = 268677120U;
constexpr FwOpcodeType OPCODE_PERSISTENT_FAULT_HISTORY = 268666112U;
constexpr FwOpcodeType OPCODE_RADIO_GET_STATUS = 268656643U;
constexpr FwOpcodeType OPCODE_STORAGE_GET_STATUS = 268681216U;
constexpr FwOpcodeType OPCODE_BOOT_STATUS = 268664832U;
constexpr FwOpcodeType OPCODE_EPS_SET_PDU = 268644353U;
constexpr FwOpcodeType OPCODE_PAYLOAD_SET_CAMERA_DEFAULTS = 268673024U;
constexpr FwOpcodeType OPCODE_PAYLOAD_PREPARE = 268673025U;
constexpr FwOpcodeType OPCODE_PAYLOAD_ABORT = 268673027U;
constexpr FwOpcodeType OPCODE_PAYLOAD_SHUTDOWN = 268673028U;
constexpr FwOpcodeType OPCODE_PAYLOAD_GET_STATUS = 268673029U;
constexpr FwOpcodeType OPCODE_PAYLOAD_PREPARE_RAW_SENSOR = 268673030U;
constexpr FwOpcodeType OPCODE_PAYLOAD_SET_AUTO_DEFAULTS = 268673031U;
constexpr FwOpcodeType OPCODE_PAYLOAD_SET_DETERMINISTIC_DEFAULTS = 268673032U;
constexpr FwOpcodeType OPCODE_PAYLOAD_CAPTURE_AUTO = 268673033U;
constexpr FwOpcodeType OPCODE_PAYLOAD_CAPTURE_DETERMINISTIC = 268673034U;
constexpr FwOpcodeType OPCODE_PAYLOAD_GET_CAPABILITIES = 268673035U;
constexpr FwOpcodeType OPCODE_PAYLOAD_GET_LAST_CAPTURE_METADATA = 268673036U;
constexpr FwOpcodeType OPCODE_PAYLOAD_SENSOR_REG_READ = 268673037U;
constexpr FwOpcodeType OPCODE_PAYLOAD_SENSOR_REG_WRITE = 268673038U;
constexpr FwOpcodeType OPCODE_PAYLOAD_CAPTURE_RAW = 268673039U;
constexpr FwOpcodeType OPCODE_PAYLOAD_PUBLISH_CAPTURE = 268673040U;
constexpr FwOpcodeType OPCODE_UNKNOWN = 0xDEADBEEFU;

OBC::CommandSessionKey sessionKey(FwIndexType ingressPort,
                                  OBC::AuthorityLinkIdentity identity,
                                  OBC::AuthorityLinkRole role,
                                  U32 sessionId) {
    OBC::CommandSessionKey key;
    key.ingressPort = ingressPort;
    key.linkIdentity = identity;
    key.linkRole = role;
    key.sessionId = sessionId;
    return key;
}

}  // namespace

TEST(CommandAuthorityPolicy, UHFBackupAllowlistMatchesV1Policy) {
    const OBC::AuthorityConfig uhf = OBC::authorityConfigFromProfile("uhf-backup");
    const FwOpcodeType allowed[] = {OPCODE_MODE_GET,
                                   OPCODE_EPS_GET_STATUS,
                                   OPCODE_ADCS_GET_ATTITUDE,
                                   OPCODE_GPS_GET_STATE,
                                   OPCODE_PERSISTENT_FAULT_HISTORY,
                                   OPCODE_RADIO_GET_STATUS,
                                   OPCODE_STORAGE_GET_STATUS,
                                   OPCODE_BOOT_STATUS};
    for (const FwOpcodeType opcode : allowed) {
        const OBC::AuthorityDecision decision = OBC::evaluateCommandAuthority(uhf, opcode);
        EXPECT_TRUE(decision.allow) << opcode;
        EXPECT_EQ(decision.response, Fw::CmdResponse::OK) << opcode;
    }
}

TEST(CommandAuthorityPolicy, UHFBackupRejectsHighAuthorityAndUnknownCommands) {
    const OBC::AuthorityConfig uhf = OBC::authorityConfigFromProfile("uhf-backup");

    OBC::AuthorityDecision decision = OBC::evaluateCommandAuthority(uhf, OPCODE_MODE_SET);
    EXPECT_FALSE(decision.allow);
    EXPECT_EQ(decision.reason, OBC::AuthorityRejectReason::POLICY_DENIED);
    EXPECT_EQ(decision.response, Fw::CmdResponse::VALIDATION_ERROR);

    decision = OBC::evaluateCommandAuthority(uhf, OPCODE_EPS_SET_PDU);
    EXPECT_FALSE(decision.allow);
    EXPECT_EQ(decision.reason, OBC::AuthorityRejectReason::POLICY_DENIED);
    EXPECT_EQ(decision.response, Fw::CmdResponse::VALIDATION_ERROR);

    decision = OBC::evaluateCommandAuthority(uhf, OPCODE_UNKNOWN);
    EXPECT_FALSE(decision.allow);
    EXPECT_EQ(decision.reason, OBC::AuthorityRejectReason::UNKNOWN_OPCODE_RESTRICTED);
    EXPECT_EQ(decision.response, Fw::CmdResponse::INVALID_OPCODE);
}

TEST(CommandAuthorityPolicy, SbandBackupUsesRestrictedLowRiskPolicy) {
    OBC::AuthorityConfig sbandBackup = OBC::authorityConfigFromProfile("sband-primary");
    sbandBackup.role = OBC::AuthorityLinkRole::BACKUP;

    EXPECT_TRUE(OBC::evaluateCommandAuthority(sbandBackup, OPCODE_MODE_GET).allow);
    const OBC::AuthorityDecision denied = OBC::evaluateCommandAuthority(sbandBackup, OPCODE_MODE_SET);
    EXPECT_FALSE(denied.allow);
    EXPECT_EQ(denied.reason, OBC::AuthorityRejectReason::POLICY_DENIED);
    EXPECT_EQ(denied.response, Fw::CmdResponse::VALIDATION_ERROR);
}

TEST(CommandAuthorityPolicy, ExplicitPrimaryProfileAllowsKnownAndRejectsUnknownCatalogMisses) {
    const OBC::AuthorityConfig sband = OBC::authorityConfigFromProfile("sband-primary");
    EXPECT_TRUE(OBC::evaluateCommandAuthority(sband, OPCODE_MODE_SET).allow);
    const OBC::AuthorityDecision unknown = OBC::evaluateCommandAuthority(sband, OPCODE_UNKNOWN);
    EXPECT_FALSE(unknown.allow);
    EXPECT_EQ(unknown.reason, OBC::AuthorityRejectReason::UNKNOWN_OPCODE_RESTRICTED);
    EXPECT_EQ(unknown.response, Fw::CmdResponse::INVALID_OPCODE);
}

TEST(CommandAuthorityPolicy, DevAndInternalProfilesPreserveDispatcherBehavior) {
    const OBC::AuthorityConfig dev = OBC::authorityConfigFromProfile("dev-direct");
    EXPECT_TRUE(dev.valid);
    EXPECT_TRUE(OBC::evaluateCommandAuthority(dev, OPCODE_MODE_SET).allow);
    EXPECT_TRUE(OBC::evaluateCommandAuthority(dev, OPCODE_UNKNOWN).allow);

    const OBC::AuthorityConfig internal = OBC::authorityConfigFromProfile("internal");
    EXPECT_TRUE(internal.valid);
    EXPECT_TRUE(OBC::evaluateCommandAuthority(internal, OPCODE_MODE_SET).allow);
    EXPECT_TRUE(OBC::evaluateCommandAuthority(internal, OPCODE_UNKNOWN).allow);
}

TEST(CommandAuthorityPolicy, FutureUhfPrimaryAfterFailoverIsVocabularyOnlyFullRole) {
    OBC::AuthorityConfig futureFailover;
    futureFailover.identity = OBC::AuthorityLinkIdentity::UHF;
    futureFailover.role = OBC::AuthorityLinkRole::PRIMARY_AFTER_FAILOVER;
    futureFailover.valid = true;

    EXPECT_TRUE(OBC::evaluateCommandAuthority(futureFailover, OPCODE_MODE_SET).allow);
    const OBC::AuthorityDecision unknown = OBC::evaluateCommandAuthority(futureFailover, OPCODE_UNKNOWN);
    EXPECT_FALSE(unknown.allow);
    EXPECT_EQ(unknown.reason, OBC::AuthorityRejectReason::UNKNOWN_OPCODE_RESTRICTED);
    EXPECT_EQ(unknown.response, Fw::CmdResponse::INVALID_OPCODE);
}

TEST(CommandAuthorityPolicy, MissingOrUnknownProfileDeniesByDefault) {
    EXPECT_FALSE(OBC::authorityConfigFromProfile(nullptr).valid);
    EXPECT_FALSE(OBC::authorityConfigFromProfile("bogus").valid);

    const OBC::AuthorityDecision decision =
        OBC::evaluateCommandAuthority(OBC::authorityConfigFromProfile("bogus"), OPCODE_MODE_GET);
    EXPECT_FALSE(decision.allow);
    EXPECT_EQ(decision.reason, OBC::AuthorityRejectReason::INVALID_CONFIG);
    EXPECT_EQ(decision.response, Fw::CmdResponse::EXECUTION_ERROR);
}

TEST(CommandSessionSequence, AcceptsFirstAndIncreasingSequence) {
    OBC::CommandSequenceWindow window;
    const OBC::CommandSessionKey key =
        sessionKey(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 100U);

    EXPECT_EQ(window.evaluateAndAccept(key, 10U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(key, 11U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(key, 42U), OBC::CommandSequenceResult::ACCEPTED);
}

TEST(CommandSessionSequence, RejectsDuplicateAndLowerSequence) {
    OBC::CommandSequenceWindow window;
    const OBC::CommandSessionKey key =
        sessionKey(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 100U);

    EXPECT_EQ(window.evaluateAndAccept(key, 10U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(key, 10U), OBC::CommandSequenceResult::REJECTED_NOT_INCREASING);
    EXPECT_EQ(window.evaluateAndAccept(key, 9U), OBC::CommandSequenceResult::REJECTED_NOT_INCREASING);
    EXPECT_EQ(window.evaluateAndAccept(key, 11U), OBC::CommandSequenceResult::ACCEPTED);
}

TEST(CommandSessionSequence, SeparatesIngressPortSessionAndRoleEpoch) {
    OBC::CommandSequenceWindow window;
    const OBC::CommandSessionKey sbandPrimary =
        sessionKey(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 100U);
    const OBC::CommandSessionKey differentPort =
        sessionKey(1, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 100U);
    const OBC::CommandSessionKey differentSession =
        sessionKey(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 101U);
    const OBC::CommandSessionKey differentRole =
        sessionKey(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY_AFTER_FAILOVER, 100U);

    EXPECT_EQ(window.evaluateAndAccept(sbandPrimary, 10U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(differentPort, 10U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(differentSession, 10U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(differentRole, 10U), OBC::CommandSequenceResult::ACCEPTED);
}

TEST(CommandSessionSequence, ResetSessionAllowsNewBaseline) {
    OBC::CommandSequenceWindow window;
    const OBC::CommandSessionKey key =
        sessionKey(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 100U);

    EXPECT_EQ(window.evaluateAndAccept(key, 10U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_TRUE(window.resetSession(key));
    EXPECT_EQ(window.evaluateAndAccept(key, 5U), OBC::CommandSequenceResult::ACCEPTED);
}

TEST(CommandAuthorityPolicy, LegacySessionOpenOpcodeIsNotInCurrentCatalog) {
    constexpr FwOpcodeType OPCODE_SESSION_OPEN = 268718080U;
    EXPECT_EQ(OBC::findCommandAuthorityCatalogEntry(OPCODE_SESSION_OPEN), nullptr);

    const OBC::AuthorityDecision decision =
        OBC::evaluateCommandAuthority(OBC::authorityConfigFromProfile("sband-primary"), OPCODE_SESSION_OPEN);
    EXPECT_FALSE(decision.allow);
    EXPECT_EQ(decision.reason, OBC::AuthorityRejectReason::UNKNOWN_OPCODE_RESTRICTED);
    EXPECT_EQ(decision.response, Fw::CmdResponse::INVALID_OPCODE);
}

TEST(CommandAuthorityPolicy, PersistentFaultHistoryIsBoundedReadStatusSurface) {
    const OBC::CommandAuthorityCatalogEntry* entry =
        OBC::findCommandAuthorityCatalogEntry(OPCODE_PERSISTENT_FAULT_HISTORY);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->opcode, OPCODE_PERSISTENT_FAULT_HISTORY);
    EXPECT_EQ(entry->commandClass, OBC::AuthorityCommandClass::READ_STATUS);
    EXPECT_EQ(entry->resource, OBC::AuthorityResourceLabel::HEALTH);
    EXPECT_TRUE(entry->uhfBackupAllowed);

    const OBC::AuthorityDecision backupDecision =
        OBC::evaluateCommandAuthority(OBC::authorityConfigFromProfile("uhf-backup"),
                                      OPCODE_PERSISTENT_FAULT_HISTORY);
    EXPECT_TRUE(backupDecision.allow);
    EXPECT_EQ(backupDecision.response, Fw::CmdResponse::OK);
}

TEST(CommandAuthorityPolicy, SystemResourcesEnableRemainsControlledRuntimeSurface) {
    const OBC::CommandAuthorityCatalogEntry* entry =
        OBC::findCommandAuthorityCatalogEntry(OBC::OBC_SYSTEM_RESOURCES_ENABLE_OPCODE);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->commandClass, OBC::AuthorityCommandClass::CONFIG_UPDATE);
    EXPECT_EQ(entry->resource, OBC::AuthorityResourceLabel::HEALTH);
    EXPECT_FALSE(entry->uhfBackupAllowed);

    const OBC::AuthorityDecision backupDecision =
        OBC::evaluateCommandAuthority(OBC::authorityConfigFromProfile("uhf-backup"),
                                      OBC::OBC_SYSTEM_RESOURCES_ENABLE_OPCODE);
    EXPECT_FALSE(backupDecision.allow);
    EXPECT_EQ(backupDecision.reason, OBC::AuthorityRejectReason::POLICY_DENIED);
    EXPECT_EQ(backupDecision.response, Fw::CmdResponse::VALIDATION_ERROR);

    const OBC::AuthorityDecision primaryDecision =
        OBC::evaluateCommandAuthority(OBC::authorityConfigFromProfile("sband-primary"),
                                      OBC::OBC_SYSTEM_RESOURCES_ENABLE_OPCODE);
    EXPECT_TRUE(primaryDecision.allow);
}

TEST(CommandAuthorityPolicy, PayloadCatalogEntriesUseGovernedPayloadLabels) {
    const FwOpcodeType payloadControls[] = {OPCODE_PAYLOAD_SET_CAMERA_DEFAULTS,
                                            OPCODE_PAYLOAD_PREPARE,
                                            OPCODE_PAYLOAD_PREPARE_RAW_SENSOR,
                                            OPCODE_PAYLOAD_SET_AUTO_DEFAULTS,
                                            OPCODE_PAYLOAD_SET_DETERMINISTIC_DEFAULTS,
                                            OPCODE_PAYLOAD_CAPTURE_AUTO,
                                            OPCODE_PAYLOAD_CAPTURE_DETERMINISTIC,
                                            OPCODE_PAYLOAD_SENSOR_REG_WRITE,
                                            OPCODE_PAYLOAD_CAPTURE_RAW,
                                            OPCODE_PAYLOAD_PUBLISH_CAPTURE,
                                            OPCODE_PAYLOAD_ABORT,
                                            OPCODE_PAYLOAD_SHUTDOWN};
    for (const FwOpcodeType opcode : payloadControls) {
        const OBC::CommandAuthorityCatalogEntry* entry = OBC::findCommandAuthorityCatalogEntry(opcode);
        ASSERT_NE(entry, nullptr);
        EXPECT_EQ(entry->commandClass, OBC::AuthorityCommandClass::PAYLOAD_CONTROL) << opcode;
        EXPECT_EQ(entry->resource, OBC::AuthorityResourceLabel::PAYLOAD) << opcode;
        EXPECT_FALSE(entry->uhfBackupAllowed) << opcode;
    }

    const OBC::CommandAuthorityCatalogEntry* statusEntry = OBC::findCommandAuthorityCatalogEntry(OPCODE_PAYLOAD_GET_STATUS);
    ASSERT_NE(statusEntry, nullptr);
    EXPECT_EQ(statusEntry->commandClass, OBC::AuthorityCommandClass::READ_STATUS);
    EXPECT_EQ(statusEntry->resource, OBC::AuthorityResourceLabel::PAYLOAD);
    EXPECT_TRUE(statusEntry->uhfBackupAllowed);

    const FwOpcodeType payloadReads[] = {
        OPCODE_PAYLOAD_GET_CAPABILITIES, OPCODE_PAYLOAD_GET_LAST_CAPTURE_METADATA, OPCODE_PAYLOAD_SENSOR_REG_READ};
    for (const FwOpcodeType opcode : payloadReads) {
        const OBC::CommandAuthorityCatalogEntry* entry = OBC::findCommandAuthorityCatalogEntry(opcode);
        ASSERT_NE(entry, nullptr);
        EXPECT_EQ(entry->commandClass, OBC::AuthorityCommandClass::READ_STATUS) << opcode;
        EXPECT_EQ(entry->resource, OBC::AuthorityResourceLabel::PAYLOAD) << opcode;
        EXPECT_TRUE(entry->uhfBackupAllowed) << opcode;
    }
}

TEST(CommandAuthorityPolicy, RestrictedBackupAllowsPayloadStatusButDeniesPayloadControl) {
    const OBC::AuthorityConfig backup = OBC::authorityConfigFromProfile("uhf-backup");

    const OBC::AuthorityDecision statusDecision = OBC::evaluateCommandAuthority(backup, OPCODE_PAYLOAD_GET_STATUS);
    EXPECT_TRUE(statusDecision.allow);
    EXPECT_EQ(statusDecision.commandClass, OBC::AuthorityCommandClass::READ_STATUS);
    EXPECT_EQ(statusDecision.resource, OBC::AuthorityResourceLabel::PAYLOAD);
    EXPECT_EQ(statusDecision.response, Fw::CmdResponse::OK);

    const FwOpcodeType payloadReads[] = {
        OPCODE_PAYLOAD_GET_CAPABILITIES, OPCODE_PAYLOAD_GET_LAST_CAPTURE_METADATA, OPCODE_PAYLOAD_SENSOR_REG_READ};
    for (const FwOpcodeType opcode : payloadReads) {
        const OBC::AuthorityDecision readDecision = OBC::evaluateCommandAuthority(backup, opcode);
        EXPECT_TRUE(readDecision.allow) << opcode;
        EXPECT_EQ(readDecision.commandClass, OBC::AuthorityCommandClass::READ_STATUS) << opcode;
        EXPECT_EQ(readDecision.resource, OBC::AuthorityResourceLabel::PAYLOAD) << opcode;
        EXPECT_EQ(readDecision.response, Fw::CmdResponse::OK) << opcode;
    }

    const FwOpcodeType payloadControls[] = {OPCODE_PAYLOAD_SET_CAMERA_DEFAULTS,
                                            OPCODE_PAYLOAD_PREPARE,
                                            OPCODE_PAYLOAD_PREPARE_RAW_SENSOR,
                                            OPCODE_PAYLOAD_SET_AUTO_DEFAULTS,
                                            OPCODE_PAYLOAD_SET_DETERMINISTIC_DEFAULTS,
                                            OPCODE_PAYLOAD_CAPTURE_AUTO,
                                            OPCODE_PAYLOAD_CAPTURE_DETERMINISTIC,
                                            OPCODE_PAYLOAD_SENSOR_REG_WRITE,
                                            OPCODE_PAYLOAD_CAPTURE_RAW,
                                            OPCODE_PAYLOAD_PUBLISH_CAPTURE,
                                            OPCODE_PAYLOAD_ABORT,
                                            OPCODE_PAYLOAD_SHUTDOWN};
    for (const FwOpcodeType opcode : payloadControls) {
        const OBC::AuthorityDecision decision = OBC::evaluateCommandAuthority(backup, opcode);
        EXPECT_FALSE(decision.allow) << opcode;
        EXPECT_EQ(decision.commandClass, OBC::AuthorityCommandClass::PAYLOAD_CONTROL) << opcode;
        EXPECT_EQ(decision.resource, OBC::AuthorityResourceLabel::PAYLOAD) << opcode;
        EXPECT_EQ(decision.reason, OBC::AuthorityRejectReason::POLICY_DENIED) << opcode;
        EXPECT_EQ(decision.response, Fw::CmdResponse::VALIDATION_ERROR) << opcode;
    }
}

TEST(CommandSessionSequence, ResetSourceClearsAllSessionsForSameEpoch) {
    OBC::CommandSequenceWindow window;
    const OBC::CommandSessionKey sessionA =
        sessionKey(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 100U);
    const OBC::CommandSessionKey sessionB =
        sessionKey(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 101U);
    const OBC::CommandSessionKey otherPort =
        sessionKey(1, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 100U);

    EXPECT_EQ(window.evaluateAndAccept(sessionA, 10U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(sessionB, 20U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(otherPort, 30U), OBC::CommandSequenceResult::ACCEPTED);

    window.resetSource(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY);

    EXPECT_EQ(window.evaluateAndAccept(sessionA, 1U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(sessionB, 2U), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(otherPort, 29U), OBC::CommandSequenceResult::REJECTED_NOT_INCREASING);
}

TEST(CommandSessionSequence, WraparoundRequiresExplicitReset) {
    OBC::CommandSequenceWindow window;
    const OBC::CommandSessionKey key =
        sessionKey(0, OBC::AuthorityLinkIdentity::SBAND, OBC::AuthorityLinkRole::PRIMARY, 100U);

    EXPECT_EQ(window.evaluateAndAccept(key, 0xFFFFFFFFU), OBC::CommandSequenceResult::ACCEPTED);
    EXPECT_EQ(window.evaluateAndAccept(key, 0U), OBC::CommandSequenceResult::REJECTED_NOT_INCREASING);
    EXPECT_TRUE(window.resetSession(key));
    EXPECT_EQ(window.evaluateAndAccept(key, 0U), OBC::CommandSequenceResult::ACCEPTED);
}
