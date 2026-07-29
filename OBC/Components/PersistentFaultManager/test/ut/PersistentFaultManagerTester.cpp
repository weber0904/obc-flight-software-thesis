#include "PersistentFaultManagerTester.hpp"

#include <stdlib.h>
#include <unistd.h>

#include "Os/FileSystem.hpp"

namespace OBC {

namespace {

std::string joinPath(const std::string& base, const char* child) {
    return base.back() == '/' ? base + child : base + "/" + child;
}

}  // namespace

PersistentFaultManagerTester::PersistentFaultManagerTester()
    : PersistentFaultManagerGTestBase("PersistentFaultManagerTester", MAX_HISTORY_SIZE),
      m_tempRoot(this->makeTempRoot_()),
      component("PersistentFaultManager") {
    this->initComponents();
    this->connectPorts();
}

PersistentFaultManagerTester::~PersistentFaultManagerTester() {
    this->cleanupTree_();
}

void PersistentFaultManagerTester::testHistoryCommandRequiresConfiguration() {
    this->clearHistory();
    this->sendCmd_GET_PERSISTENT_FAULT_HISTORY(TEST_INSTANCE_ID, 0, 4U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0,
                        this->component.OPCODE_GET_PERSISTENT_FAULT_HISTORY,
                        0,
                        Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_UNAVAILABLE_SIZE(1);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_STATUS_SIZE(0);
}

void PersistentFaultManagerTester::testHistoryCommandReportsLatestFirst() {
    ASSERT_TRUE(this->configure_());
    ASSERT_TRUE(this->component.appendPersistentFaultRecordForRuntime(
        this->makeRecord_(OBC::PersistentFaultRecordKind::BOOT_OBSERVED, 1U, 11U)));
    ASSERT_TRUE(this->component.appendPersistentFaultRecordForRuntime(
        this->makeRecord_(OBC::PersistentFaultRecordKind::INCIDENT_OPENED, 2U, 22U)));

    this->clearHistory();
    this->sendCmd_GET_PERSISTENT_FAULT_HISTORY(TEST_INSTANCE_ID, 1, 2U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0,
                        this->component.OPCODE_GET_PERSISTENT_FAULT_HISTORY,
                        1,
                        Fw::CmdResponse::OK);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_STATUS_SIZE(1);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_STATUS(0, 2U, 2U, OBC::PersistentFaultStoreCopy::COPY_B, 2U);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_RECORD_SIZE(2);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_RECORD(0,
                                                  0U,
                                                  OBC::PersistentFaultRecordKind::INCIDENT_OPENED,
                                                  OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE,
                                                  OBC::RecoveryLevel::R6_OBC_REBOOT,
                                                  OBC::RecoveryAction::OBC_REBOOT,
                                                  OBC::ResetCause::RECOVERY_COMM_FDIR,
                                                  2U,
                                                  1U,
                                                  22U,
                                                  1002U,
                                                  22U,
                                                  OBC::PersistentFaultFlagBootSafeFallback);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_RECORD(1,
                                                  1U,
                                                  OBC::PersistentFaultRecordKind::BOOT_OBSERVED,
                                                  OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE,
                                                  OBC::RecoveryLevel::R6_OBC_REBOOT,
                                                  OBC::RecoveryAction::OBC_REBOOT,
                                                  OBC::ResetCause::RECOVERY_COMM_FDIR,
                                                  1U,
                                                  0U,
                                                  21U,
                                                  1001U,
                                                  11U,
                                                  0U);
}

void PersistentFaultManagerTester::testHistoryCommandClampsLargeLimit() {
    ASSERT_TRUE(this->configure_());
    for (U32 i = 0; i < 3U; i++) {
        ASSERT_TRUE(this->component.appendPersistentFaultRecordForRuntime(
            this->makeRecord_(OBC::PersistentFaultRecordKind::ACTION_REQUESTED, i + 1U, 30U + i)));
    }

    this->clearHistory();
    this->sendCmd_GET_PERSISTENT_FAULT_HISTORY(TEST_INSTANCE_ID, 2, 999U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0,
                        this->component.OPCODE_GET_PERSISTENT_FAULT_HISTORY,
                        2,
                        Fw::CmdResponse::OK);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_STATUS_SIZE(1);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_STATUS(0, 3U, 3U, OBC::PersistentFaultStoreCopy::COPY_A, 3U);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_RECORD_SIZE(3);
}

void PersistentFaultManagerTester::testHistoryCommandReportsEmptyConfiguredSet() {
    ASSERT_TRUE(this->configure_());

    this->clearHistory();
    this->sendCmd_GET_PERSISTENT_FAULT_HISTORY(TEST_INSTANCE_ID, 3, 4U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0,
                        this->component.OPCODE_GET_PERSISTENT_FAULT_HISTORY,
                        3,
                        Fw::CmdResponse::OK);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_STATUS_SIZE(1);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_STATUS(0, 0U, 0U, OBC::PersistentFaultStoreCopy::NONE, 0U);
    ASSERT_EVENTS_PERSISTENT_FAULT_HISTORY_RECORD_SIZE(0);
}

std::string PersistentFaultManagerTester::makeTempRoot_() const {
    char buffer[] = "/tmp/persistent-fault-manager-ut-XXXXXX";
    const char* const path = ::mkdtemp(buffer);
    if (path == nullptr) {
        ADD_FAILURE() << "mkdtemp failed";
        return {};
    }
    return std::string(path);
}

void PersistentFaultManagerTester::cleanupTree_() const {
    if (this->m_tempRoot.empty()) {
        return;
    }
    (void)Os::FileSystem::removeFile((this->m_tempRoot + "/persistent-data/recovery/fault-ring-a.bin").c_str());
    (void)Os::FileSystem::removeFile((this->m_tempRoot + "/persistent-data/recovery/fault-ring-b.bin").c_str());
    (void)Os::FileSystem::removeDirectory((this->m_tempRoot + "/persistent-data/recovery").c_str());
    (void)Os::FileSystem::removeDirectory((this->m_tempRoot + "/persistent-data").c_str());
    (void)Os::FileSystem::removeDirectory(this->m_tempRoot.c_str());
}

bool PersistentFaultManagerTester::configure_() {
    return this->component.configurePersistentRootForRuntime(joinPath(this->m_tempRoot, "persistent-data").c_str());
}

OBC::PersistentFaultRecord PersistentFaultManagerTester::makeRecord_(const OBC::PersistentFaultRecordKind kind,
                                                                     const U32 bootCount,
                                                                     const U32 detail) const {
    OBC::PersistentFaultRecord record = {};
    record.kind = kind;
    record.source = OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE;
    record.level = OBC::RecoveryLevel::R6_OBC_REBOOT;
    record.action = OBC::RecoveryAction::OBC_REBOOT;
    record.resetCause = OBC::ResetCause::RECOVERY_COMM_FDIR;
    record.timestampSec = 1000U + bootCount;
    record.uptimeSec = 20U + bootCount;
    record.bootCount = bootCount;
    record.consecutiveResetCount = bootCount / 2U;
    record.detail = detail;
    record.flags = (bootCount % 2U) == 0U ? OBC::PersistentFaultFlagBootSafeFallback : 0U;
    return record;
}

}  // namespace OBC
