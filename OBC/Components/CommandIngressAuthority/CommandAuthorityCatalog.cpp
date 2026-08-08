#include "OBC/Components/CommandIngressAuthority/CommandAuthorityCatalog.hpp"

namespace OBC {

namespace {

const CommandAuthorityCatalogEntry COMMAND_AUTHORITY_CATALOG[] = {
    {16777216U, "CdhCore.cmdDisp.CMD_NO_OP", AuthorityCommandClass::DEV_INTERNAL, AuthorityResourceLabel::COMMAND_DISPATCH, false},
    {16777217U, "CdhCore.cmdDisp.CMD_NO_OP_STRING", AuthorityCommandClass::DEV_INTERNAL, AuthorityResourceLabel::COMMAND_DISPATCH, false},
    {16777218U, "CdhCore.cmdDisp.CMD_TEST_CMD_1", AuthorityCommandClass::DEV_INTERNAL, AuthorityResourceLabel::COMMAND_DISPATCH, false},
    {16777219U, "CdhCore.cmdDisp.CMD_CLEAR_TRACKING", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::COMMAND_DISPATCH, false},
    {16781312U, "CdhCore.events.SET_EVENT_FILTER", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::EVENT_FILTER, false},
    {16781314U, "CdhCore.events.SET_ID_FILTER", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::EVENT_FILTER, false},
    {16781315U, "CdhCore.events.DUMP_FILTER_STATE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::EVENT_FILTER, false},
    {16785408U, "CdhCore.health.HLTH_ENABLE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::HEALTH, false},
    {16785409U, "CdhCore.health.HLTH_PING_ENABLE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::HEALTH, false},
    {16785410U, "CdhCore.health.HLTH_CHNG_PING", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::HEALTH, false},
    {16789504U, "CdhCore.version.ENABLE", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::COMMAND_DISPATCH, false},
    {16789505U, "CdhCore.version.VERSION", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::COMMAND_DISPATCH, false},
    {83890176U, "FileHandling.fileDownlink.SendFile", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83890177U, "FileHandling.fileDownlink.Cancel", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83890178U, "FileHandling.fileDownlink.SendPartial", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83894272U, "FileHandling.fileManager.CreateDirectory", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83894273U, "FileHandling.fileManager.MoveFile", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83894274U, "FileHandling.fileManager.RemoveDirectory", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83894275U, "FileHandling.fileManager.RemoveFile", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83894276U, "FileHandling.fileManager.ShellCommand", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83894277U, "FileHandling.fileManager.AppendFile", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83894278U, "FileHandling.fileManager.FileSize", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83894279U, "FileHandling.fileManager.ListDirectory", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {83898368U, "FileHandling.prmDb.PRM_SAVE_FILE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::CONFIG_STORE, false},
    {83898369U, "FileHandling.prmDb.PRM_LOAD_FILE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::CONFIG_STORE, false},
    {83898370U, "FileHandling.prmDb.PRM_COMMIT_STAGED", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::CONFIG_STORE, false},
    {268632064U, "OBCApp.modeManager.MODE_SET", AuthorityCommandClass::MODE_CHANGE, AuthorityResourceLabel::MODE_STATE, false},
    {268632065U, "OBCApp.modeManager.MODE_GET", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::MODE_STATE, true},
    {268633344U, "OBCApp.ttcPassManager.TTC_SET_POLICY", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::MODE_STATE, false},
    {268633345U, "OBCApp.ttcPassManager.TTC_SET_PASS_WINDOW", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::MODE_STATE, false},
    {268633346U, "OBCApp.ttcPassManager.TTC_CLEAR_PASS_WINDOW", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::MODE_STATE, false},
    {268633347U, "OBCApp.ttcPassManager.TTC_GET_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::MODE_STATE, true},
    {268636160U, "OBCApp.watchdogSupervisor.HEALTH_ENABLE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::HEALTH, false},
    {268636161U, "OBCApp.watchdogSupervisor.HEALTH_SET_THRESHOLD", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::HEALTH, false},
    {268636162U, "OBCApp.watchdogSupervisor.GET_WATCHDOG_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::HEALTH, true},
    {268636163U, "OBCApp.watchdogSupervisor.SET_WATCHDOG_CONFIG", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::HEALTH, false},
    {268636164U, "OBCApp.watchdogSupervisor.SET_WATCHDOG_PROBE_SUPPRESSION", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::HEALTH, false},
    {268637440U, "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::HEALTH, true},
    {268640256U, "OBCApp.cspBridge.CSP_INIT", AuthorityCommandClass::CSP_TRAFFIC, AuthorityResourceLabel::CSP, false},
    {268640257U, "OBCApp.cspBridge.CSP_PING", AuthorityCommandClass::CSP_TRAFFIC, AuthorityResourceLabel::CSP, false},
    {268640258U, "OBCApp.cspBridge.CSP_SEND_RAW", AuthorityCommandClass::CSP_TRAFFIC, AuthorityResourceLabel::CSP, false},
    {268644352U, "OBCApp.epsBridge.EPS_GET_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::EPS, true},
    {268644353U, "OBCApp.epsBridge.EPS_SET_PDU", AuthorityCommandClass::POWER_CONTROL, AuthorityResourceLabel::EPS, false},
    {268644354U, "OBCApp.epsBridge.EPS_SET_HEATER", AuthorityCommandClass::POWER_CONTROL, AuthorityResourceLabel::EPS, false},
    {268644355U, "OBCApp.epsBridge.EPS_RESET", AuthorityCommandClass::RESET, AuthorityResourceLabel::EPS, false},
    {268648448U, "OBCApp.adcsBridge.ADCS_SET_MODE", AuthorityCommandClass::ADCS_CONTROL, AuthorityResourceLabel::ADCS, false},
    {268648449U, "OBCApp.adcsBridge.ADCS_SET_TARGET", AuthorityCommandClass::ADCS_CONTROL, AuthorityResourceLabel::ADCS, false},
    {268648450U, "OBCApp.adcsBridge.ADCS_GET_ATTITUDE", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::ADCS, true},
    {268648451U, "OBCApp.adcsBridge.ADCS_CALIBRATE", AuthorityCommandClass::ADCS_CONTROL, AuthorityResourceLabel::ADCS, false},
    {268652544U, "OBCApp.commController.COMM_SET_ACTIVE", AuthorityCommandClass::COMM_CONTROL, AuthorityResourceLabel::COMM_LINK, false},
    {268652545U, "OBCApp.commController.COMM_START_PASS", AuthorityCommandClass::COMM_CONTROL, AuthorityResourceLabel::COMM_LINK, false},
    {268652546U, "OBCApp.commController.COMM_STOP_PASS", AuthorityCommandClass::COMM_CONTROL, AuthorityResourceLabel::COMM_LINK, false},
    {268652547U, "OBCApp.commController.COMM_GET_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::COMM_LINK, true},
    {268656640U, "OBCApp.radioController.RADIO_ENABLE", AuthorityCommandClass::COMM_CONTROL, AuthorityResourceLabel::RADIO, false},
    {268656641U, "OBCApp.radioController.RADIO_SET_POWER", AuthorityCommandClass::COMM_CONTROL, AuthorityResourceLabel::RADIO, false},
    {268656642U, "OBCApp.radioController.RADIO_SET_FREQ", AuthorityCommandClass::COMM_CONTROL, AuthorityResourceLabel::RADIO, false},
    {268656643U, "OBCApp.radioController.RADIO_GET_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::RADIO, true},
    {268664832U, "OBCApp.bootManager.BOOT_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::BOOT, true},
    {268664833U, "OBCApp.bootManager.BOOT_PREPARE_UPDATE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::BOOT, false},
    {268664834U, "OBCApp.bootManager.BOOT_VERIFY_STAGED_IMAGE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::BOOT, false},
    {268664835U, "OBCApp.bootManager.BOOT_ACTIVATE_STAGED_IMAGE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::BOOT, false},
    {268664836U, "OBCApp.bootManager.BOOT_CONFIRM", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::BOOT, false},
    {268664837U, "OBCApp.bootManager.BOOT_ROLLBACK", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::BOOT, false},
    {268664838U, "OBCApp.bootManager.GET_RESET_CAUSE", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::BOOT, true},
    {268664839U, "OBCApp.bootManager.GET_BOOT_COUNT", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::BOOT, true},
    {268666112U, "OBCApp.persistentFaultManager.GET_PERSISTENT_FAULT_HISTORY", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::HEALTH, true},
    {268673024U, "OBCApp.payloadOpsController.PAYLOAD_SET_CAMERA_DEFAULTS", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673025U, "OBCApp.payloadOpsController.PAYLOAD_PREPARE", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673027U, "OBCApp.payloadOpsController.PAYLOAD_ABORT", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673028U, "OBCApp.payloadOpsController.PAYLOAD_SHUTDOWN", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673029U, "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::PAYLOAD, true},
    {268673030U, "OBCApp.payloadOpsController.PAYLOAD_PREPARE_RAW_SENSOR", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673031U, "OBCApp.payloadOpsController.PAYLOAD_SET_AUTO_DEFAULTS", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673032U, "OBCApp.payloadOpsController.PAYLOAD_SET_DETERMINISTIC_DEFAULTS", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673033U, "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_AUTO", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673034U, "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_DETERMINISTIC", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673035U, "OBCApp.payloadOpsController.PAYLOAD_GET_CAPABILITIES", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::PAYLOAD, true},
    {268673036U, "OBCApp.payloadOpsController.PAYLOAD_GET_LAST_CAPTURE_METADATA", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::PAYLOAD, true},
    {268673037U, "OBCApp.payloadOpsController.PAYLOAD_SENSOR_REG_READ", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::PAYLOAD, true},
    {268673038U, "OBCApp.payloadOpsController.PAYLOAD_SENSOR_REG_WRITE", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673039U, "OBCApp.payloadOpsController.PAYLOAD_CAPTURE_RAW", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268673040U, "OBCApp.payloadOpsController.PAYLOAD_PUBLISH_CAPTURE", AuthorityCommandClass::PAYLOAD_CONTROL, AuthorityResourceLabel::PAYLOAD, false},
    {268677120U, "OBCApp.gpsBridge.GPS_GET_STATE", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::GPS, true},
    {268677121U, "OBCApp.gpsBridge.GPS_SET_SOURCE_MODE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::GPS, false},
    {268681216U, "OBCApp.storageHealthBridge.STORAGE_GET_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::STORAGE, true},
    {268693504U, "OBCApp.hkTrendProductProducer.HK_TREND_FLUSH", AuthorityCommandClass::DATA_PRODUCT, AuthorityResourceLabel::DATA_PRODUCTS, false},
    {268693505U, "OBCApp.hkTrendProductProducer.HK_TREND_GET_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::DATA_PRODUCTS, true},
    {268693506U, "OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SET", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::DATA_PRODUCTS, false},
    {268693507U, "OBCApp.hkTrendProductProducer.HK_TREND_TARGET_FILE_BYTES_PRM_SAVE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::DATA_PRODUCTS, false},
    {268697600U, "OBCApp.dpCatalog.BUILD_CATALOG", AuthorityCommandClass::DATA_PRODUCT, AuthorityResourceLabel::DATA_PRODUCTS, false},
    {268697601U, "OBCApp.dpCatalog.START_XMIT_CATALOG", AuthorityCommandClass::DATA_PRODUCT, AuthorityResourceLabel::DATA_PRODUCTS, false},
    {268697602U, "OBCApp.dpCatalog.STOP_XMIT_CATALOG", AuthorityCommandClass::DATA_PRODUCT, AuthorityResourceLabel::DATA_PRODUCTS, false},
    {268697603U, "OBCApp.dpCatalog.CLEAR_CATALOG", AuthorityCommandClass::DATA_PRODUCT, AuthorityResourceLabel::DATA_PRODUCTS, false},
    {268701696U, "OBCApp.dpManager.CLEAR_EVENT_THROTTLE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::DATA_PRODUCTS, false},
    {268705792U, "OBCApp.dpWriter.CLEAR_EVENT_THROTTLE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::DATA_PRODUCTS, false},
    {268730368U, "OBCApp.recoveryExecutor.GET_RECOVERY_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::HEALTH, true},
    {268738560U, "OBCApp.sequenceAdmissionController.SEQ_VALIDATE", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, true},
    {268738561U, "OBCApp.sequenceAdmissionController.SEQ_RUN", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, true},
    {268738562U, "OBCApp.sequenceAdmissionController.SEQ_PREPARE_MANUAL", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, true},
    {268738563U, "OBCApp.sequenceAdmissionController.SEQ_START", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, true},
    {268738564U, "OBCApp.sequenceAdmissionController.SEQ_STEP", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, true},
    {268738565U, "OBCApp.sequenceAdmissionController.SEQ_CANCEL", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, true},
    {268738566U, "OBCApp.sequenceAdmissionController.SEQ_LOG_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::FILE_TRANSFER_SESSION, true},
    {268742656U, "OBCApp.cmdSeqA.CS_RUN", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268742657U, "OBCApp.cmdSeqA.CS_VALIDATE", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268742658U, "OBCApp.cmdSeqA.CS_CANCEL", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268742659U, "OBCApp.cmdSeqA.CS_START", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268742660U, "OBCApp.cmdSeqA.CS_STEP", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268742661U, "OBCApp.cmdSeqA.CS_AUTO", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268742662U, "OBCApp.cmdSeqA.CS_MANUAL", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268742663U, "OBCApp.cmdSeqA.CS_JOIN_WAIT", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268746752U, "OBCApp.cmdSeqB.CS_RUN", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268746753U, "OBCApp.cmdSeqB.CS_VALIDATE", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268746754U, "OBCApp.cmdSeqB.CS_CANCEL", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268746755U, "OBCApp.cmdSeqB.CS_START", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268746756U, "OBCApp.cmdSeqB.CS_STEP", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268746757U, "OBCApp.cmdSeqB.CS_AUTO", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268746758U, "OBCApp.cmdSeqB.CS_MANUAL", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268746759U, "OBCApp.cmdSeqB.CS_JOIN_WAIT", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268750848U, "OBCApp.seqDispatcher.RUN", AuthorityCommandClass::FILE_TRANSFER, AuthorityResourceLabel::FILE_TRANSFER_SESSION, false},
    {268750849U, "OBCApp.seqDispatcher.LOG_STATUS", AuthorityCommandClass::READ_STATUS, AuthorityResourceLabel::FILE_TRANSFER_SESSION, true},
    {268763136U, "OBCApp.systemResources.ENABLE", AuthorityCommandClass::CONFIG_UPDATE, AuthorityResourceLabel::HEALTH, false},
};

}  // namespace

const CommandAuthorityCatalogEntry* findCommandAuthorityCatalogEntry(FwOpcodeType opcode) {
    FwSizeType low = 0;
    FwSizeType high = getCommandAuthorityCatalogEntryCount();
    while (low < high) {
        const FwSizeType mid = low + ((high - low) / 2);
        if (COMMAND_AUTHORITY_CATALOG[mid].opcode < opcode) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    if (low < getCommandAuthorityCatalogEntryCount() && COMMAND_AUTHORITY_CATALOG[low].opcode == opcode) {
        return &COMMAND_AUTHORITY_CATALOG[low];
    }
    return nullptr;
}

FwSizeType getCommandAuthorityCatalogEntryCount() {
    return static_cast<FwSizeType>(sizeof(COMMAND_AUTHORITY_CATALOG) / sizeof(COMMAND_AUTHORITY_CATALOG[0]));
}

AuthorityDecision evaluateCommandAuthority(const AuthorityConfig& config, FwOpcodeType opcode) {
    AuthorityDecision decision;
    if (!config.valid) {
        decision.reason = AuthorityRejectReason::INVALID_CONFIG;
        decision.response = Fw::CmdResponse::EXECUTION_ERROR;
        return decision;
    }
    const CommandAuthorityCatalogEntry* entry = findCommandAuthorityCatalogEntry(opcode);
    if (entry == nullptr) {
        if (isCommManagedAuthority(config)) {
            decision.reason = AuthorityRejectReason::UNKNOWN_OPCODE_RESTRICTED;
            decision.response = Fw::CmdResponse::INVALID_OPCODE;
        } else {
            decision.allow = true;
            decision.reason = AuthorityRejectReason::NONE;
            decision.response = Fw::CmdResponse::OK;
        }
        return decision;
    }
    decision.commandClass = entry->commandClass;
    decision.resource = entry->resource;
    if (!isRestrictedAuthority(config) || entry->uhfBackupAllowed) {
        decision.allow = true;
        decision.reason = AuthorityRejectReason::NONE;
        decision.response = Fw::CmdResponse::OK;
    } else {
        decision.reason = AuthorityRejectReason::POLICY_DENIED;
        decision.response = Fw::CmdResponse::VALIDATION_ERROR;
    }
    return decision;
}

}  // namespace OBC
