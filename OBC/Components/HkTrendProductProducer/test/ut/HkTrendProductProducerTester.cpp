#include "HkTrendProductProducerTester.hpp"

namespace OBC {

bool HkTrendProductProducerTester::FakeSnapshotSource::readStateSnapshot(
    OBC::StateData::StateSnapshot& output) const {
    if (!this->available) {
        return false;
    }
    output = this->snapshot;
    return true;
}

HkTrendProductProducerTester::HkTrendProductProducerTester()
    : HkTrendProductProducerGTestBase("HkTrendProductProducerTester", MAX_HISTORY_SIZE),
      m_source(),
      m_dpBuffer(),
      component("HkTrendProductProducer") {
    this->initComponents();
    this->connectPorts();
    this->configureNominal_();
    this->component.configureRuntime(&this->m_source);
}

HkTrendProductProducerTester::~HkTrendProductProducerTester() = default;

void HkTrendProductProducerTester::testManualFlushEmitsChunkWithRawValues() {
    this->clearHistory();

    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);
    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_TLM_HK_TREND_PENDING_SAMPLE_COUNT_SIZE(1);
    ASSERT_TLM_HK_TREND_PENDING_SAMPLE_COUNT(0, 1U);

    this->sendCmd_HK_TREND_FLUSH(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
    ASSERT_PRODUCT_GET_SIZE(1);
    ASSERT_PRODUCT_SEND_SIZE(1);
    ASSERT_EVENTS_HK_TREND_PRODUCT_WRITTEN_SIZE(1);
    ASSERT_EVENTS_HK_TREND_PRODUCT_WRITTEN(0, 0U, 0U, 0U, 1U, computePacketSizeForSamples_(1U),
                                           OBC::HkTrendFlushReason::MANUAL_FLUSH);

    DecodedSentProduct decoded = {};
    ASSERT_TRUE(this->decodeLastSentProduct_(decoded));
    ASSERT_EQ(decoded.metaRecordId, 1U);
    ASSERT_EQ(decoded.sampleRecordId, 0U);
    ASSERT_EQ(decoded.meta.get_version(), 1U);
    ASSERT_EQ(decoded.meta.get_chunkSequence(), 0U);
    ASSERT_EQ(decoded.meta.get_firstSampleSequence(), 0U);
    ASSERT_EQ(decoded.meta.get_lastSampleSequence(), 0U);
    ASSERT_EQ(decoded.meta.get_sampleCount(), 1U);
    ASSERT_EQ(decoded.meta.get_targetFileBytes(), 8192U);
    ASSERT_EQ(decoded.meta.get_flushReason(), OBC::HkTrendFlushReason::MANUAL_FLUSH);
    ASSERT_EQ(decoded.samples.size(), 1U);

    const OBC::HkTrendRecordV6& record = decoded.samples.at(0);
    ASSERT_EQ(record.get_version(), 6U);
    ASSERT_EQ(record.get_sequence(), 0U);
    ASSERT_EQ(record.get_mode(), OBC::SatMode::IDLE);
    ASSERT_TRUE(record.get_haveEpsStatus());
    ASSERT_FLOAT_EQ(record.get_epsVbat(), 8.2F);
    ASSERT_FLOAT_EQ(record.get_epsIbat(), 1.3F);
    ASSERT_TRUE(record.get_haveAdcsState());
    ASSERT_DOUBLE_EQ(record.get_adcsQ0(), 0.91);
    ASSERT_DOUBLE_EQ(record.get_adcsQ1(), 0.12);
    ASSERT_DOUBLE_EQ(record.get_adcsQ2(), 0.22);
    ASSERT_DOUBLE_EQ(record.get_adcsQ3(), 0.31);
    ASSERT_FLOAT_EQ(record.get_adcsMagX(), 0.21F);
    ASSERT_FLOAT_EQ(record.get_adcsMagY(), 0.22F);
    ASSERT_FLOAT_EQ(record.get_adcsMagZ(), 0.23F);
    ASSERT_FLOAT_EQ(record.get_adcsPointingErrorDeg(), 1.5F);
    ASSERT_TRUE(record.get_haveGpsState());
    ASSERT_TRUE(record.get_gpsFixValid());
    ASSERT_FLOAT_EQ(record.get_gpsAltitudeMeters(), 120.5F);
    ASSERT_FLOAT_EQ(record.get_gpsSpeedMetersPerSecond(), 11.52F);
    ASSERT_FLOAT_EQ(record.get_gpsCourseDegrees(), 84.4F);
    ASSERT_EQ(record.get_gpsSatelliteCount(), 8U);
    ASSERT_FLOAT_EQ(record.get_gpsHdop(), 0.9F);
    ASSERT_EQ(record.get_gpsUtcSecondsOfDay(), 45296U);
    ASSERT_EQ(record.get_gpsUtcDateYmd(), 19940323U);
    ASSERT_EQ(record.get_gpsAcceptedSentences(), 11U);
    ASSERT_TRUE(record.get_haveStorageHealth());
    ASSERT_TRUE(record.get_storagePersistentExists());
    ASSERT_EQ(record.get_storagePersistentFileCount(), 2U);
    ASSERT_EQ(record.get_storagePersistentBytes(), 512U);
    ASSERT_TRUE(record.get_storageStagingExists());
    ASSERT_EQ(record.get_storageStagingFileCount(), 1U);
    ASSERT_EQ(record.get_storageStagingBytes(), 256U);
    ASSERT_TRUE(record.get_storageLogsExists());
    ASSERT_EQ(record.get_storageLogsFileCount(), 5U);
    ASSERT_EQ(record.get_storageLogsBytes(), 768U);
    ASSERT_TRUE(record.get_storageDataProductsExists());
    ASSERT_TRUE(record.get_storageDataProductsScanOk());
    ASSERT_EQ(record.get_storageDataProductsFileCount(), 3U);
    ASSERT_EQ(record.get_storageDataProductsBytes(), 2048U);
    ASSERT_EQ(record.get_storageDataProductsErrorCode(), 0U);
    ASSERT_EQ(record.get_storageDataProductsQuotaBytes(), 4096U);
    ASSERT_EQ(record.get_storageDataProductsWatermarkBytes(), 1024U);
    ASSERT_EQ(record.get_storageDataProductsQuotaStatus(), OBC::STORAGE::STORAGE_QUOTA_OK);
    ASSERT_EQ(record.get_storageDataProductsRetentionStatus(), OBC::STORAGE::STORAGE_RETENTION_OBSERVE_ONLY);
    ASSERT_EQ(record.get_commActiveBand(), OBC::CommBand::UHF);
    ASSERT_TRUE(record.get_cspInitialized());
    ASSERT_EQ(record.get_cspLocalNodeId(), 5U);
    ASSERT_EQ(record.get_cspTxPackets(), 17U);
    ASSERT_EQ(record.get_cspRxPackets(), 19U);
    ASSERT_EQ(record.get_cspErrorCount(), 2U);
    ASSERT_EQ(record.get_cspFreeBuffers(), 42U);
    ASSERT_TRUE(record.get_uartConnected());
    ASSERT_EQ(record.get_uartTxBytes(), 33U);
    ASSERT_EQ(record.get_uartRxBytes(), 44U);
    ASSERT_EQ(record.get_uartTxErrors(), 1U);
    ASSERT_EQ(record.get_uartRxErrors(), 2U);
    ASSERT_TRUE(record.get_haveRadioStatus());
    ASSERT_TRUE(record.get_radioEnabled());
    ASSERT_EQ(record.get_radioPowerDbm(), 14U);
    ASSERT_EQ(record.get_radioFreqHz(), 437000000U);
    ASSERT_FLOAT_EQ(record.get_radioTemperatureC(), 28.25F);
    ASSERT_EQ(record.get_radioRssiDbm(), -72);
    ASSERT_EQ(record.get_radioTxBytes(), 64U);
    ASSERT_EQ(record.get_radioRxBytes(), 128U);
    ASSERT_EQ(record.get_radioTxErrors(), 3U);
    ASSERT_EQ(record.get_radioRxErrors(), 4U);
    ASSERT_EQ(record.get_pendingBootSlot(), OBC::BootSlot::SLOT_B);
    ASSERT_EQ(record.get_lastKnownGoodBootSlot(), OBC::BootSlot::SLOT_A);
    ASSERT_FALSE(record.get_bootConfirmed());
    ASSERT_TRUE(record.get_bootStageVerified());
    ASSERT_EQ(record.get_bootExpectedSize(), 65536U);
    ASSERT_EQ(record.get_bootLastBootAttemptTime(), 123456U);
    ASSERT_EQ(record.get_bootLastErrorCode(), 7U);
    ASSERT_EQ(record.get_bootRemainingConfirmSeconds(), 89U);
    ASSERT_EQ(record.get_bootUpdateProgress(), 67U);

    const U32 lastProductCountIndex = this->tlmHistory_HK_TREND_PRODUCT_COUNT->size() - 1U;
    const U32 lastChunkIndex = this->tlmHistory_HK_TREND_LAST_CHUNK_SEQUENCE->size() - 1U;
    const U32 lastPendingIndex = this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->size() - 1U;
    ASSERT_EQ(this->tlmHistory_HK_TREND_PRODUCT_COUNT->at(lastProductCountIndex).arg, 1U);
    ASSERT_EQ(this->tlmHistory_HK_TREND_LAST_SEQUENCE->at(lastProductCountIndex).arg, 0U);
    ASSERT_EQ(this->tlmHistory_HK_TREND_LAST_CHUNK_SEQUENCE->at(lastChunkIndex).arg, 0U);
    ASSERT_EQ(this->tlmHistory_HK_TREND_LAST_SIZE_BYTES->at(lastProductCountIndex).arg, computePacketSizeForSamples_(1U));
    ASSERT_EQ(this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->at(lastPendingIndex).arg, 0U);
}

void HkTrendProductProducerTester::testCadenceWaitsBetweenSamples() {
    this->component.configurePeriodForTest(2U);
    this->clearHistory();

    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_PRODUCT_SEND_SIZE(0);
    const U32 pendingIndex = this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->size() - 1U;
    ASSERT_EQ(this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->at(pendingIndex).arg, 2U);

    this->sendCmd_HK_TREND_FLUSH(TEST_INSTANCE_ID, 0);
    ASSERT_PRODUCT_SEND_SIZE(1);

    DecodedSentProduct decoded = {};
    ASSERT_TRUE(this->decodeLastSentProduct_(decoded));
    ASSERT_EQ(decoded.samples.size(), 2U);
    ASSERT_EQ(decoded.samples.at(0).get_sequence(), 0U);
    ASSERT_EQ(decoded.samples.at(1).get_sequence(), 1U);
}

void HkTrendProductProducerTester::testSizeThresholdChunkingKeepsContiguousSequences() {
    this->component.configureTargetFileBytesForTest(computePacketSizeForSamples_(2U));
    this->clearHistory();

    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);
    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);
    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);

    ASSERT_PRODUCT_SEND_SIZE(1);
    ASSERT_EVENTS_HK_TREND_PRODUCT_WRITTEN_SIZE(1);
    ASSERT_EVENTS_HK_TREND_PRODUCT_WRITTEN(0, 0U, 0U, 1U, 2U, computePacketSizeForSamples_(2U),
                                           OBC::HkTrendFlushReason::SIZE_THRESHOLD);

    DecodedSentProduct firstChunk = {};
    ASSERT_TRUE(this->decodeLastSentProduct_(firstChunk));
    ASSERT_EQ(firstChunk.samples.size(), 2U);
    ASSERT_EQ(firstChunk.samples.at(0).get_sequence(), 0U);
    ASSERT_EQ(firstChunk.samples.at(1).get_sequence(), 1U);

    const U32 pendingIndex = this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->size() - 1U;
    ASSERT_EQ(this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->at(pendingIndex).arg, 1U);

    this->sendCmd_HK_TREND_FLUSH(TEST_INSTANCE_ID, 0);
    ASSERT_PRODUCT_SEND_SIZE(2);

    DecodedSentProduct secondChunk = {};
    ASSERT_TRUE(this->decodeLastSentProduct_(secondChunk));
    ASSERT_EQ(secondChunk.meta.get_chunkSequence(), 1U);
    ASSERT_EQ(secondChunk.meta.get_firstSampleSequence(), 2U);
    ASSERT_EQ(secondChunk.meta.get_lastSampleSequence(), 2U);
    ASSERT_EQ(secondChunk.meta.get_flushReason(), OBC::HkTrendFlushReason::MANUAL_FLUSH);
    ASSERT_EQ(secondChunk.samples.size(), 1U);
    ASSERT_EQ(secondChunk.samples.at(0).get_sequence(), 2U);
}

void HkTrendProductProducerTester::testMissingStateUsesValidityFlags() {
    this->m_source.snapshot.haveEpsStatus = false;
    this->m_source.snapshot.haveAdcsState = false;
    this->m_source.snapshot.haveGpsState = false;
    this->m_source.snapshot.haveStorageHealth = false;
    this->m_source.snapshot.haveRadioStatus = false;
    this->clearHistory();

    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);
    this->sendCmd_HK_TREND_FLUSH(TEST_INSTANCE_ID, 0);

    DecodedSentProduct decoded = {};
    ASSERT_TRUE(this->decodeLastSentProduct_(decoded));
    ASSERT_EQ(decoded.samples.size(), 1U);

    const OBC::HkTrendRecordV6& record = decoded.samples.at(0);
    ASSERT_FALSE(record.get_haveEpsStatus());
    ASSERT_FLOAT_EQ(record.get_epsVbat(), 0.0F);
    ASSERT_FALSE(record.get_haveAdcsState());
    ASSERT_DOUBLE_EQ(record.get_adcsQ0(), 0.0);
    ASSERT_FLOAT_EQ(record.get_adcsMagX(), 0.0F);
    ASSERT_FALSE(record.get_haveGpsState());
    ASSERT_FALSE(record.get_gpsFixValid());
    ASSERT_EQ(record.get_gpsUtcDateYmd(), 0U);
    ASSERT_FALSE(record.get_haveStorageHealth());
    ASSERT_FALSE(record.get_storageDataProductsExists());
    ASSERT_EQ(record.get_storageDataProductsBytes(), 0U);
    ASSERT_FALSE(record.get_haveRadioStatus());
    ASSERT_FALSE(record.get_radioEnabled());
    ASSERT_EQ(record.get_radioFreqHz(), 0U);
    ASSERT_NE(record.get_qualityMask() & OBC::StateData::QUALITY_EPS_MISSING, 0U);
    ASSERT_NE(record.get_qualityMask() & OBC::StateData::QUALITY_ADCS_MISSING, 0U);
    ASSERT_NE(record.get_qualityMask() & OBC::StateData::QUALITY_GPS_MISSING, 0U);
    ASSERT_NE(record.get_qualityMask() & OBC::StateData::QUALITY_STORAGE_MISSING, 0U);
    ASSERT_NE(record.get_qualityMask() & OBC::StateData::QUALITY_RADIO_MISSING, 0U);
}

void HkTrendProductProducerTester::testFlushCommandOnEmptyChunkIsNoOp() {
    this->clearHistory();

    this->sendCmd_HK_TREND_FLUSH(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_EVENTS_HK_TREND_PRODUCT_WRITTEN_SIZE(0);
}

void HkTrendProductProducerTester::testGetStatusCommandReportsPendingState() {
    this->clearHistory();

    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);
    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);

    this->sendCmd_HK_TREND_GET_STATUS(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
    ASSERT_EVENTS_HK_TREND_STATUS_SIZE(1);
    ASSERT_EVENTS_HK_TREND_STATUS(0, 8192U, 2U, computePacketSizeForSamples_(2U), 2U, 0U);
}

void HkTrendProductProducerTester::testThresholdChangeFlushesImmediately() {
    this->clearHistory();

    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);
    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);

    this->paramSet_HK_TREND_TARGET_FILE_BYTES(computePacketSizeForSamples_(1U), Fw::ParamValid::VALID);
    this->component.loadParameters();

    ASSERT_PRODUCT_SEND_SIZE(1);
    ASSERT_EVENTS_HK_TREND_PRODUCT_WRITTEN_SIZE(1);
    ASSERT_EVENTS_HK_TREND_PRODUCT_WRITTEN(0, 0U, 0U, 1U, 2U, computePacketSizeForSamples_(2U),
                                           OBC::HkTrendFlushReason::THRESHOLD_CHANGE);

    DecodedSentProduct decoded = {};
    ASSERT_TRUE(this->decodeLastSentProduct_(decoded));
    ASSERT_EQ(decoded.meta.get_flushReason(), OBC::HkTrendFlushReason::THRESHOLD_CHANGE);
    ASSERT_EQ(decoded.samples.size(), 2U);
}

void HkTrendProductProducerTester::testParameterLoadDefaultPathUses8192() {
    this->component.configureTargetFileBytesForTest(computePacketSizeForSamples_(1U));
    this->paramSet_HK_TREND_TARGET_FILE_BYTES(9999U, Fw::ParamValid::UNINIT);
    this->clearHistory();

    this->component.loadParameters();

    ASSERT_TLM_HK_TREND_TARGET_FILE_BYTES_SIZE(1);
    ASSERT_TLM_HK_TREND_TARGET_FILE_BYTES(0, 8192U);
}

void HkTrendProductProducerTester::testOutOfRangeParameterIsClampedAndPersisted() {
    this->paramSet_HK_TREND_TARGET_FILE_BYTES(HkTrendProductProducer::MAX_TARGET_FILE_BYTES + 111U, Fw::ParamValid::VALID);
    this->clearHistory();

    this->component.loadParameters();

    ASSERT_TLM_HK_TREND_TARGET_FILE_BYTES_SIZE(1);
    ASSERT_TLM_HK_TREND_TARGET_FILE_BYTES(0, HkTrendProductProducer::MAX_TARGET_FILE_BYTES);

    this->paramSet_HK_TREND_TARGET_FILE_BYTES(HkTrendProductProducer::MAX_TARGET_FILE_BYTES, Fw::ParamValid::VALID);
    this->paramSave_HK_TREND_TARGET_FILE_BYTES(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
}

void HkTrendProductProducerTester::testUnavailableSourceReportsError() {
    this->m_source.available = false;
    this->clearHistory();

    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::SOURCE_UNAVAILABLE);
    ASSERT_EVENTS_HK_TREND_SOURCE_UNAVAILABLE_SIZE(1);
    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_TLM_HK_TREND_LAST_ERROR_SIZE(1);
    ASSERT_TLM_HK_TREND_LAST_ERROR(0, static_cast<U32>(OBC::HkTrendProductStatus::SOURCE_UNAVAILABLE));
}

void HkTrendProductProducerTester::testSerializeFailureReturnsDpBuffer() {
    this->m_returnShortDpBuffer = true;
    this->clearHistory();

    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);
    this->sendCmd_HK_TREND_FLUSH(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_PRODUCT_GET_SIZE(1);
    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_from_productBufferReturnOut_SIZE(1);
    ASSERT_EVENTS_HK_TREND_PRODUCT_REJECTED_SIZE(1);
    ASSERT_EVENTS_HK_TREND_PRODUCT_REJECTED(0, 0U, static_cast<U32>(OBC::HkTrendProductStatus::SERIALIZE_ERROR));

    const Fw::Buffer returned = this->fromPortHistory_productBufferReturnOut->at(0).fwBuffer;
    ASSERT_EQ(returned.getData(), this->m_dpBuffer);
    ASSERT_EQ(returned.getSize(), static_cast<U32>(Fw::DpContainer::MIN_PACKET_SIZE + sizeof(FwDpIdType)));

    const U32 errorIndex = this->tlmHistory_HK_TREND_LAST_ERROR->size() - 1U;
    const U32 pendingIndex = this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->size() - 1U;
    ASSERT_EQ(this->tlmHistory_HK_TREND_LAST_ERROR->at(errorIndex).arg,
              static_cast<U32>(OBC::HkTrendProductStatus::SERIALIZE_ERROR));
    ASSERT_EQ(this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->at(pendingIndex).arg, 1U);
}

void HkTrendProductProducerTester::testThresholdFlushFailureRetainsCurrentSample() {
    this->component.configureTargetFileBytesForTest(computePacketSizeForSamples_(1U));
    this->clearHistory();

    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);
    this->m_returnShortDpBuffer = true;
    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::SERIALIZE_ERROR);

    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_from_productBufferReturnOut_SIZE(1);
    ASSERT_TLM_HK_TREND_PENDING_SAMPLE_COUNT_SIZE(3);
    ASSERT_TLM_HK_TREND_PENDING_SAMPLE_COUNT(2, 2U);

    this->m_returnShortDpBuffer = false;
    this->sendCmd_HK_TREND_FLUSH(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
    ASSERT_PRODUCT_SEND_SIZE(1);

    DecodedSentProduct firstChunk = {};
    ASSERT_TRUE(this->decodeLastSentProduct_(firstChunk));
    ASSERT_EQ(firstChunk.samples.size(), 1U);
    ASSERT_EQ(firstChunk.meta.get_firstSampleSequence(), 0U);
    ASSERT_EQ(firstChunk.meta.get_lastSampleSequence(), 0U);
    ASSERT_EQ(firstChunk.samples.at(0).get_sequence(), 0U);

    this->clearHistory();
    this->sendCmd_HK_TREND_FLUSH(TEST_INSTANCE_ID, 1);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
    ASSERT_PRODUCT_SEND_SIZE(1);

    DecodedSentProduct secondChunk = {};
    ASSERT_TRUE(this->decodeLastSentProduct_(secondChunk));
    ASSERT_EQ(secondChunk.samples.size(), 1U);
    ASSERT_EQ(secondChunk.meta.get_firstSampleSequence(), 1U);
    ASSERT_EQ(secondChunk.meta.get_lastSampleSequence(), 1U);
    ASSERT_EQ(secondChunk.samples.at(0).get_sequence(), 1U);
}

void HkTrendProductProducerTester::testRetainedBacklogIsBoundedWhenFlushKeepsFailing() {
    this->component.configureTargetFileBytesForTest(computePacketSizeForSamples_(1U));
    this->clearHistory();

    ASSERT_EQ(this->component.captureNow(), OBC::HkTrendProductStatus::OK);

    this->m_returnShortDpBuffer = true;
    const U32 maxPendingSamples = computeMaxPendingSamples_();
    for (U32 captureIndex = 1U; captureIndex < (maxPendingSamples + 3U); captureIndex++) {
        const OBC::HkTrendProductStatus status = this->component.captureNow();
        const OBC::HkTrendProductStatus expected =
            captureIndex < maxPendingSamples ? OBC::HkTrendProductStatus::SERIALIZE_ERROR
                                             : OBC::HkTrendProductStatus::BACKLOG_FULL;
        ASSERT_EQ(status, expected);
    }

    ASSERT_GE(this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->size(), 1U);
    ASSERT_EQ(this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT
                  ->at(this->tlmHistory_HK_TREND_PENDING_SAMPLE_COUNT->size() - 1U)
                  .arg,
              maxPendingSamples);

    ASSERT_GE(this->eventHistory_HK_TREND_PRODUCT_REJECTED->size(), 3U);
    std::vector<U32> backlogRejectedSequences;
    for (U32 i = 0; i < this->eventHistory_HK_TREND_PRODUCT_REJECTED->size(); i++) {
        const auto& rejected = this->eventHistory_HK_TREND_PRODUCT_REJECTED->at(i);
        if (rejected.code == static_cast<U32>(OBC::HkTrendProductStatus::BACKLOG_FULL)) {
            backlogRejectedSequences.push_back(rejected.sequence);
        }
    }
    ASSERT_EQ(backlogRejectedSequences.size(), 3U);
    ASSERT_EQ(backlogRejectedSequences.at(0), maxPendingSamples);
    ASSERT_EQ(backlogRejectedSequences.at(1), maxPendingSamples + 1U);
    ASSERT_EQ(backlogRejectedSequences.at(2), maxPendingSamples + 2U);

    const U32 lastErrorIndex = this->tlmHistory_HK_TREND_LAST_ERROR->size() - 1U;
    const U32 lastSequenceIndex = this->tlmHistory_HK_TREND_LAST_SEQUENCE->size() - 1U;
    ASSERT_EQ(this->tlmHistory_HK_TREND_LAST_ERROR->at(lastErrorIndex).arg,
              static_cast<U32>(OBC::HkTrendProductStatus::BACKLOG_FULL));
    ASSERT_EQ(this->tlmHistory_HK_TREND_LAST_SEQUENCE->at(lastSequenceIndex).arg, maxPendingSamples + 2U);

    this->m_returnShortDpBuffer = false;
    this->clearHistory();
    this->sendCmd_HK_TREND_FLUSH(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
    ASSERT_PRODUCT_SEND_SIZE(1);

    DecodedSentProduct flushedChunk = {};
    ASSERT_TRUE(this->decodeLastSentProduct_(flushedChunk));
    ASSERT_EQ(flushedChunk.samples.size(), 1U);
    ASSERT_EQ(flushedChunk.meta.get_firstSampleSequence(), 0U);
    ASSERT_EQ(flushedChunk.meta.get_lastSampleSequence(), 0U);
    ASSERT_EQ(flushedChunk.samples.at(0).get_sequence(), 0U);
}

Fw::Success::T HkTrendProductProducerTester::productGet_handler(FwDpIdType id,
                                                                FwSizeType dataSize,
                                                                Fw::Buffer& buffer) {
    this->pushProductGetEntry(id, dataSize);
    if (dataSize > sizeof(this->m_dpBuffer)) {
        return Fw::Success::FAILURE;
    }
    const FwSizeType size =
        this->m_returnShortDpBuffer ? (Fw::DpContainer::MIN_PACKET_SIZE + sizeof(FwDpIdType)) : dataSize;
    buffer.set(this->m_dpBuffer, size);
    return Fw::Success::SUCCESS;
}

bool HkTrendProductProducerTester::decodeLastSentProduct_(DecodedSentProduct& decoded) const {
    if (this->productSendHistory->size() == 0U) {
        return false;
    }

    Fw::Buffer buffer = this->productSendHistory->at(this->productSendHistory->size() - 1U).buffer;
    auto deserializer = buffer.getDeserializer();
    if (deserializer.moveDeserToOffset(Fw::DpContainer::DATA_OFFSET) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return false;
    }

    decoded = {};
    if (deserializer.deserializeTo(decoded.metaRecordId) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return false;
    }
    if (deserializer.deserializeTo(decoded.meta) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return false;
    }
    if (deserializer.deserializeTo(decoded.sampleRecordId) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return false;
    }

    FwSizeType sampleCount = 0U;
    if (deserializer.deserializeSize(sampleCount) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return false;
    }

    decoded.samples.clear();
    decoded.samples.reserve(sampleCount);
    for (FwSizeType i = 0; i < sampleCount; i++) {
        OBC::HkTrendRecordV6 sample = {};
        if (deserializer.deserializeTo(sample) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
            return false;
        }
        decoded.samples.push_back(sample);
    }

    return true;
}

U32 HkTrendProductProducerTester::computePacketSizeForSamples_(FwSizeType sampleCount) {
    const FwSizeType dataSize = sizeof(FwDpIdType) + OBC::HkTrendChunkMetaV1::SERIALIZED_SIZE + sizeof(FwDpIdType) +
                                sizeof(FwSizeStoreType) + sampleCount * OBC::HkTrendRecordV6::SERIALIZED_SIZE;
    return static_cast<U32>(Fw::DpContainer::getPacketSizeForDataSize(dataSize));
}

U32 HkTrendProductProducerTester::computeMaxPendingSamples_() {
    U32 maxChunkSamples = 0U;
    for (FwSizeType sampleCount = 1U;; sampleCount++) {
        if (computePacketSizeForSamples_(sampleCount) > OBC::HkTrendProductProducer::MAX_TARGET_FILE_BYTES) {
            break;
        }
        maxChunkSamples = static_cast<U32>(sampleCount);
    }
    return maxChunkSamples * OBC::HkTrendProductProducer::MAX_PENDING_CHUNKS;
}

void HkTrendProductProducerTester::configureNominal_() {
    OBC::StateData::StateSnapshot& snapshot = this->m_source.snapshot;
    this->m_source.available = true;
    this->m_returnShortDpBuffer = false;
    this->paramSet_HK_TREND_TARGET_FILE_BYTES(8192U, Fw::ParamValid::VALID);
    snapshot = {};
    snapshot.timestamp = Fw::Time(TimeBase::TB_WORKSTATION_TIME, 0U, 200U, 0U);
    snapshot.mode = OBC::SatMode::IDLE;
    snapshot.uptimeSec = 123U;
    snapshot.rebootCount = 2U;
    snapshot.activeBootSlot = OBC::BootSlot::SLOT_A;
    snapshot.haveEpsStatus = true;
    snapshot.eps.vbat = 8.2F;
    snapshot.eps.ibat = 1.3F;
    snapshot.eps.soc = 74.0F;
    snapshot.eps.vsolar = 9.8F;
    snapshot.eps.isolar = 0.7F;
    snapshot.eps.temp_bat = 22.5F;
    snapshot.haveAdcsState = true;
    snapshot.adcs.mode = 2U;
    snapshot.adcs.sensor_valid = 1U;
    snapshot.adcs.q0 = 0.91;
    snapshot.adcs.q1 = 0.12;
    snapshot.adcs.q2 = 0.22;
    snapshot.adcs.q3 = 0.31;
    snapshot.adcs.omega_x = 0.01F;
    snapshot.adcs.omega_y = 0.02F;
    snapshot.adcs.omega_z = 0.03F;
    snapshot.adcs.mag_x = 0.21F;
    snapshot.adcs.mag_y = 0.22F;
    snapshot.adcs.mag_z = 0.23F;
    snapshot.adcs.pointing_error_deg = 1.5F;
    snapshot.haveGpsState = true;
    snapshot.gps.hasSample = true;
    snapshot.gps.fixValid = true;
    snapshot.gps.latitudeDeg = 25.0;
    snapshot.gps.longitudeDeg = 121.0;
    snapshot.gps.altitudeMeters = 120.5F;
    snapshot.gps.speedMetersPerSecond = 11.52F;
    snapshot.gps.courseDegrees = 84.4F;
    snapshot.gps.satelliteCount = 8U;
    snapshot.gps.hdop = 0.9F;
    snapshot.gps.utcSecondsOfDay = 45296U;
    snapshot.gps.utcDateYmd = 19940323U;
    snapshot.gps.acceptedSentenceCount = 11U;
    snapshot.haveStorageHealth = true;
    snapshot.storage.warningMask = 0U;
    snapshot.storage.persistent.exists = true;
    snapshot.storage.persistent.scanOk = true;
    snapshot.storage.persistent.fileCount = 2U;
    snapshot.storage.persistent.totalBytes = 512U;
    snapshot.storage.persistent.quotaBytes = 4096U;
    snapshot.storage.persistent.watermarkBytes = 1024U;
    snapshot.storage.persistent.quotaStatus = OBC::STORAGE::STORAGE_QUOTA_OK;
    snapshot.storage.persistent.retentionStatus = OBC::STORAGE::STORAGE_RETENTION_OBSERVE_ONLY;
    snapshot.storage.staging.exists = true;
    snapshot.storage.staging.scanOk = true;
    snapshot.storage.staging.fileCount = 1U;
    snapshot.storage.staging.totalBytes = 256U;
    snapshot.storage.staging.quotaBytes = 2048U;
    snapshot.storage.staging.watermarkBytes = 512U;
    snapshot.storage.staging.quotaStatus = OBC::STORAGE::STORAGE_QUOTA_OK;
    snapshot.storage.staging.retentionStatus = OBC::STORAGE::STORAGE_RETENTION_OBSERVE_ONLY;
    snapshot.storage.logs.exists = true;
    snapshot.storage.logs.scanOk = true;
    snapshot.storage.logs.fileCount = 5U;
    snapshot.storage.logs.totalBytes = 768U;
    snapshot.storage.logs.quotaBytes = 2048U;
    snapshot.storage.logs.watermarkBytes = 512U;
    snapshot.storage.logs.quotaStatus = OBC::STORAGE::STORAGE_QUOTA_OK;
    snapshot.storage.logs.retentionStatus = OBC::STORAGE::STORAGE_RETENTION_OBSERVE_ONLY;
    snapshot.storage.dataProducts.exists = true;
    snapshot.storage.dataProducts.scanOk = true;
    snapshot.storage.dataProducts.fileCount = 3U;
    snapshot.storage.dataProducts.totalBytes = 2048U;
    snapshot.storage.dataProducts.errorCode = 0U;
    snapshot.storage.dataProducts.quotaBytes = 4096U;
    snapshot.storage.dataProducts.watermarkBytes = 1024U;
    snapshot.storage.dataProducts.quotaStatus = OBC::STORAGE::STORAGE_QUOTA_OK;
    snapshot.storage.dataProducts.retentionStatus = OBC::STORAGE::STORAGE_RETENTION_OBSERVE_ONLY;
    snapshot.commActiveBand = OBC::CommBand::UHF;
    snapshot.cspInitialized = true;
    snapshot.cspLocalNodeId = 5U;
    snapshot.cspTxPackets = 17U;
    snapshot.cspRxPackets = 19U;
    snapshot.cspErrorCount = 2U;
    snapshot.cspFreeBuffers = 42U;
    snapshot.uartConnected = true;
    snapshot.uartTxBytes = 33U;
    snapshot.uartRxBytes = 44U;
    snapshot.uartTxErrors = 1U;
    snapshot.uartRxErrors = 2U;
    snapshot.haveRadioStatus = true;
    snapshot.radioEnabled = true;
    snapshot.radioPowerDbm = 14U;
    snapshot.radioFreqHz = 437000000U;
    snapshot.radioTemperatureC = 28.25F;
    snapshot.radioRssiDbm = -72;
    snapshot.radioLinkConnected = true;
    snapshot.radioTxBytes = 64U;
    snapshot.radioRxBytes = 128U;
    snapshot.radioTxErrors = 3U;
    snapshot.radioRxErrors = 4U;
    snapshot.pendingBootSlot = OBC::BootSlot::SLOT_B;
    snapshot.lastKnownGoodBootSlot = OBC::BootSlot::SLOT_A;
    snapshot.bootConfirmed = false;
    snapshot.bootStageVerified = true;
    snapshot.bootExpectedSize = 65536U;
    snapshot.bootLastBootAttemptTime = 123456U;
    snapshot.bootLastErrorCode = 7U;
    snapshot.bootRemainingConfirmSeconds = 89U;
    snapshot.bootUpdateProgress = 67U;
}

}  // namespace OBC
