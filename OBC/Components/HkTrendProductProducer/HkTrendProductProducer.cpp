#include "OBC/Components/HkTrendProductProducer/HkTrendProductProducer.hpp"

namespace OBC {

constexpr U32 HkTrendProductProducer::DEFAULT_PERIOD_TICKS;
constexpr U32 HkTrendProductProducer::DEFAULT_TARGET_FILE_BYTES;
constexpr U32 HkTrendProductProducer::MAX_TARGET_FILE_BYTES;
constexpr U32 HkTrendProductProducer::MAX_PENDING_CHUNKS;

namespace {

constexpr U16 HK_TREND_VERSION = 6U;
constexpr U16 HK_TREND_CHUNK_META_VERSION = 1U;

template <typename Tag, typename Tag::type Member>
struct PrivateMemberAccessor {
    friend constexpr typename Tag::type get(Tag) { return Member; }
};

struct TargetFileBytesMemberTag {
    using type = U32 HkTrendProductProducerComponentBase::*;
    friend constexpr type get(TargetFileBytesMemberTag);
};

struct TargetFileBytesValidMemberTag {
    using type = Fw::ParamValid HkTrendProductProducerComponentBase::*;
    friend constexpr type get(TargetFileBytesValidMemberTag);
};

struct ParamLockMemberTag {
    using type = Os::Mutex HkTrendProductProducerComponentBase::*;
    friend constexpr type get(ParamLockMemberTag);
};

template struct PrivateMemberAccessor<TargetFileBytesMemberTag,
                                      &HkTrendProductProducerComponentBase::m_HK_TREND_TARGET_FILE_BYTES>;
template struct PrivateMemberAccessor<TargetFileBytesValidMemberTag,
                                      &HkTrendProductProducerComponentBase::m_param_HK_TREND_TARGET_FILE_BYTES_valid>;
template struct PrivateMemberAccessor<ParamLockMemberTag, &HkTrendProductProducerComponentBase::m_paramLock>;

bool isZeroTime(const Fw::Time& time) {
    return time.getTimeBase() == TimeBase::TB_NONE && time.getSeconds() == 0U && time.getUSeconds() == 0U;
}

}  // namespace

HkTrendProductProducer::HkTrendProductProducer(const char* const compName)
    : HkTrendProductProducerComponentBase(compName),
      m_source(nullptr),
      m_periodTicks(DEFAULT_PERIOD_TICKS),
      m_ticksUntilCapture(0U),
      m_targetFileBytes(DEFAULT_TARGET_FILE_BYTES),
      m_nextSampleSequence(0U),
      m_nextChunkSequence(0U),
      m_lastSequence(0U),
      m_lastChunkSequence(0U),
      m_productCount(0U),
      m_lastSizeBytes(0U),
      m_lastError(HkTrendProductStatus::OK),
      m_pendingSamples(),
      m_pendingChunkSampleCount(0U) {}

HkTrendProductProducer::~HkTrendProductProducer() = default;

void HkTrendProductProducer::configureRuntime(OBC::StateData::IStateSnapshotSource* source) {
    Os::ScopeLock lock(this->m_lock);
    this->m_source = source;
    this->resetTicks_();
    this->m_targetFileBytes = this->normalizeTargetFileBytes_(this->m_targetFileBytes);
    this->m_nextSampleSequence = 0U;
    this->m_nextChunkSequence = 0U;
    this->m_lastSequence = 0U;
    this->m_lastChunkSequence = 0U;
    this->m_productCount = 0U;
    this->m_lastSizeBytes = 0U;
    this->m_lastError = HkTrendProductStatus::OK;
    this->m_pendingSamples.clear();
    this->m_pendingChunkSampleCount = 0U;
    this->publishState_(HkTrendProductStatus::OK);
}

void HkTrendProductProducer::configurePeriodForTest(U32 periodTicks) {
    Os::ScopeLock lock(this->m_lock);
    this->m_periodTicks = periodTicks == 0U ? 1U : periodTicks;
    this->resetTicks_();
}

void HkTrendProductProducer::configureTargetFileBytesForTest(U32 targetFileBytes) {
    Os::ScopeLock lock(this->m_lock);
    this->m_targetFileBytes = this->normalizeTargetFileBytes_(targetFileBytes);
    if (this->m_pendingChunkSampleCount > 0U && this->getPendingEstimatedBytes_() > this->m_targetFileBytes) {
        static_cast<void>(this->flushPending_(OBC::HkTrendFlushReason::THRESHOLD_CHANGE));
    } else {
        this->recomputePendingChunkSampleCount_();
        this->publishTelemetry_();
    }
}

HkTrendProductStatus HkTrendProductProducer::captureNow() {
    Os::ScopeLock lock(this->m_lock);
    if (this->m_source == nullptr) {
        this->publishState_(HkTrendProductStatus::NOT_CONFIGURED);
        this->log_WARNING_HI_HK_TREND_SOURCE_UNAVAILABLE();
        return HkTrendProductStatus::NOT_CONFIGURED;
    }

    OBC::StateData::StateSnapshot snapshot = {};
    if (!this->m_source->readStateSnapshot(snapshot)) {
        this->publishState_(HkTrendProductStatus::SOURCE_UNAVAILABLE);
        this->log_WARNING_HI_HK_TREND_SOURCE_UNAVAILABLE();
        return HkTrendProductStatus::SOURCE_UNAVAILABLE;
    }

    if (!this->haveProductPorts_()) {
        this->publishState_(HkTrendProductStatus::DP_PORTS_NOT_CONNECTED);
        this->log_WARNING_HI_HK_TREND_PRODUCT_REJECTED(this->m_nextSampleSequence,
                                                       static_cast<U32>(HkTrendProductStatus::DP_PORTS_NOT_CONNECTED));
        return HkTrendProductStatus::DP_PORTS_NOT_CONNECTED;
    }

    const U32 sequence = this->m_nextSampleSequence;
    OBC::HkTrendRecordV6 record = this->buildRecord_(snapshot, sequence);

    const bool chunkIsBacklogged = this->m_pendingSamples.size() > this->m_pendingChunkSampleCount;
    const bool appendWouldOverflow =
        this->m_pendingChunkSampleCount > 0U &&
        this->computePendingPacketSize_(this->m_pendingChunkSampleCount + 1U) > this->m_targetFileBytes;
    if (chunkIsBacklogged || appendWouldOverflow) {
        const HkTrendProductStatus flushStatus = this->flushPending_(OBC::HkTrendFlushReason::SIZE_THRESHOLD);
        if (flushStatus != HkTrendProductStatus::OK) {
            const HkTrendProductStatus retainStatus = this->retainPendingSample_(record, sequence);
            if (retainStatus == HkTrendProductStatus::OK) {
                this->publishTelemetry_();
                return flushStatus;
            }
            return retainStatus;
        }
    }

    const HkTrendProductStatus retainStatus = this->retainPendingSample_(record, sequence);
    if (retainStatus != HkTrendProductStatus::OK) {
        return retainStatus;
    }
    this->publishState_(HkTrendProductStatus::OK);
    return HkTrendProductStatus::OK;
}

void HkTrendProductProducer::HK_TREND_FLUSH_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    Os::ScopeLock lock(this->m_lock);
    const HkTrendProductStatus status = this->flushPending_(OBC::HkTrendFlushReason::MANUAL_FLUSH);
    this->cmdResponse_out(opCode,
                          cmdSeq,
                          status == HkTrendProductStatus::OK ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR);
}

void HkTrendProductProducer::HK_TREND_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    Os::ScopeLock lock(this->m_lock);
    this->log_ACTIVITY_LO_HK_TREND_STATUS(this->m_targetFileBytes,
                                          static_cast<U32>(this->m_pendingSamples.size()),
                                          this->getPendingEstimatedBytes_(),
                                          this->m_nextSampleSequence,
                                          this->m_nextChunkSequence);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void HkTrendProductProducer::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    if (this->m_ticksUntilCapture > 0U) {
        this->m_ticksUntilCapture--;
        return;
    }
    static_cast<void>(this->captureNow());
    this->resetTicks_();
}

void HkTrendProductProducer::parameterUpdated(FwPrmIdType id) {
    if (id != PARAMID_HK_TREND_TARGET_FILE_BYTES) {
        return;
    }

    Fw::ParamValid valid = Fw::ParamValid::INVALID;
    const U32 targetFileBytes = this->paramGet_HK_TREND_TARGET_FILE_BYTES(valid);
    Os::ScopeLock lock(this->m_lock);
    if (valid != Fw::ParamValid::VALID) {
        this->m_targetFileBytes = DEFAULT_TARGET_FILE_BYTES;
        this->publishTelemetry_();
        return;
    }
    this->m_targetFileBytes = this->normalizeTargetFileBytes_(targetFileBytes);
    if (this->m_targetFileBytes != targetFileBytes) {
        auto& paramLock = this->*get(ParamLockMemberTag{});
        paramLock.lock();
        this->*get(TargetFileBytesMemberTag{}) = this->m_targetFileBytes;
        this->*get(TargetFileBytesValidMemberTag{}) = Fw::ParamValid::VALID;
        paramLock.unLock();
    }

    if (this->m_pendingChunkSampleCount > 0U && this->getPendingEstimatedBytes_() > this->m_targetFileBytes) {
        static_cast<void>(this->flushPending_(OBC::HkTrendFlushReason::THRESHOLD_CHANGE));
    } else {
        this->recomputePendingChunkSampleCount_();
        this->publishTelemetry_();
    }
}

void HkTrendProductProducer::parametersLoaded() {
    this->parameterUpdated(PARAMID_HK_TREND_TARGET_FILE_BYTES);
}

OBC::HkTrendRecordV6 HkTrendProductProducer::buildRecord_(const OBC::StateData::StateSnapshot& snapshot,
                                                          U32 sequence) const {
    const OBC::StateData::ReducedStateV1 reduced = OBC::StateData::reduceStateSnapshot(snapshot);
    const Fw::Time recordTime = isZeroTime(snapshot.timestamp) ? this->getTime() : snapshot.timestamp;

    OBC::HkTrendRecordV6 record = {};
    record.set_version(HK_TREND_VERSION);
    record.set_sequence(sequence);
    record.set_timeBase(static_cast<U32>(recordTime.getTimeBase()));
    record.set_timeContext(static_cast<U32>(recordTime.getContext()));
    record.set_timeSeconds(recordTime.getSeconds());
    record.set_timeUSeconds(recordTime.getUSeconds());
    record.set_mode(reduced.mode);
    record.set_uptimeSec(reduced.uptimeSec);
    record.set_rebootCount(reduced.rebootCount);
    record.set_activeBootSlot(reduced.activeBootSlot);
    record.set_lastResetReason(snapshot.lastResetReason);

    record.set_haveEpsStatus(snapshot.haveEpsStatus);
    record.set_epsVbat(reduced.batteryVoltage);
    record.set_epsIbat(reduced.batteryCurrent);
    record.set_epsSoc(reduced.batterySoc);
    record.set_epsVSolar(snapshot.haveEpsStatus ? snapshot.eps.vsolar : 0.0F);
    record.set_epsISolar(snapshot.haveEpsStatus ? snapshot.eps.isolar : 0.0F);
    record.set_epsTempBat(reduced.batteryTempC);
    record.set_epsPowerOut(snapshot.haveEpsStatus ? snapshot.eps.power_out : 0.0F);
    record.set_epsPduStatus(snapshot.haveEpsStatus ? snapshot.eps.pdu_status : 0U);
    record.set_epsSunlight(snapshot.haveEpsStatus ? snapshot.eps.sunlight : 0U);
    record.set_epsHeaterEnabled(snapshot.haveEpsStatus ? snapshot.eps.heater_enabled : 0U);
    record.set_epsOvercurrentFlags(snapshot.haveEpsStatus ? snapshot.eps.overcurrent_flags : 0U);

    record.set_haveAdcsState(snapshot.haveAdcsState);
    record.set_adcsMode(reduced.adcsMode);
    record.set_adcsRateNorm(reduced.adcsRateNorm);
    if (snapshot.haveAdcsState) {
        const auto& adcs = snapshot.adcs;
        record.set_adcsSensorValid(adcs.sensor_valid);
        record.set_adcsQ0(adcs.q0);
        record.set_adcsQ1(adcs.q1);
        record.set_adcsQ2(adcs.q2);
        record.set_adcsQ3(adcs.q3);
        record.set_adcsOmegaX(adcs.omega_x);
        record.set_adcsOmegaY(adcs.omega_y);
        record.set_adcsOmegaZ(adcs.omega_z);
        record.set_adcsMagX(adcs.mag_x);
        record.set_adcsMagY(adcs.mag_y);
        record.set_adcsMagZ(adcs.mag_z);
        record.set_adcsPointingErrorDeg(adcs.pointing_error_deg);
    }

    record.set_haveGpsState(snapshot.haveGpsState);
    record.set_gpsHasSample(snapshot.haveGpsState && snapshot.gps.hasSample);
    record.set_gpsFixValid(reduced.gpsFixValid);
    record.set_gpsSourceMode(snapshot.haveGpsState ? static_cast<U8>(snapshot.gps.sourceMode) : 0U);
    record.set_gpsLatitudeDeg(snapshot.haveGpsState ? snapshot.gps.latitudeDeg : 0.0);
    record.set_gpsLongitudeDeg(snapshot.haveGpsState ? snapshot.gps.longitudeDeg : 0.0);
    record.set_gpsAltitudeMeters(snapshot.haveGpsState ? snapshot.gps.altitudeMeters : 0.0F);
    record.set_gpsSpeedMetersPerSecond(snapshot.haveGpsState ? snapshot.gps.speedMetersPerSecond : 0.0F);
    record.set_gpsCourseDegrees(snapshot.haveGpsState ? snapshot.gps.courseDegrees : 0.0F);
    record.set_gpsSatelliteCount(snapshot.haveGpsState ? snapshot.gps.satelliteCount : 0U);
    record.set_gpsHdop(snapshot.haveGpsState ? snapshot.gps.hdop : 0.0F);
    record.set_gpsUtcSecondsOfDay(snapshot.haveGpsState ? snapshot.gps.utcSecondsOfDay : 0U);
    record.set_gpsUtcDateYmd(snapshot.haveGpsState ? snapshot.gps.utcDateYmd : 0U);
    record.set_gpsAcceptedSentences(reduced.gpsAcceptedSentences);
    record.set_gpsRejectedSentences(reduced.gpsRejectedSentences);

    record.set_haveStorageHealth(snapshot.haveStorageHealth);
    record.set_storageWarningActive(snapshot.haveStorageHealth && snapshot.storage.warningActive);
    record.set_storageWarningMask(reduced.storageWarningMask);
    record.set_storageDegradedMask(reduced.storageDegradedMask);
    record.set_storageScanCount(snapshot.haveStorageHealth ? snapshot.storage.scanCount : 0U);
    record.set_storageScanErrorCount(snapshot.haveStorageHealth ? snapshot.storage.scanErrorCount : 0U);
    record.set_storagePersistentExists(snapshot.haveStorageHealth && snapshot.storage.persistent.exists);
    record.set_storagePersistentScanOk(snapshot.haveStorageHealth && snapshot.storage.persistent.scanOk);
    record.set_storagePersistentFileCount(snapshot.haveStorageHealth ? snapshot.storage.persistent.fileCount : 0U);
    record.set_storagePersistentBytes(snapshot.haveStorageHealth ? snapshot.storage.persistent.totalBytes : 0U);
    record.set_storagePersistentErrorCode(snapshot.haveStorageHealth ? snapshot.storage.persistent.errorCode : 0U);
    record.set_storagePersistentQuotaBytes(snapshot.haveStorageHealth ? snapshot.storage.persistent.quotaBytes : 0U);
    record.set_storagePersistentWatermarkBytes(snapshot.haveStorageHealth ? snapshot.storage.persistent.watermarkBytes : 0U);
    record.set_storagePersistentQuotaStatus(snapshot.haveStorageHealth ? snapshot.storage.persistent.quotaStatus : 0U);
    record.set_storagePersistentRetentionStatus(snapshot.haveStorageHealth ? snapshot.storage.persistent.retentionStatus : 0U);
    record.set_storageStagingExists(snapshot.haveStorageHealth && snapshot.storage.staging.exists);
    record.set_storageStagingScanOk(snapshot.haveStorageHealth && snapshot.storage.staging.scanOk);
    record.set_storageStagingFileCount(snapshot.haveStorageHealth ? snapshot.storage.staging.fileCount : 0U);
    record.set_storageStagingBytes(snapshot.haveStorageHealth ? snapshot.storage.staging.totalBytes : 0U);
    record.set_storageStagingErrorCode(snapshot.haveStorageHealth ? snapshot.storage.staging.errorCode : 0U);
    record.set_storageStagingQuotaBytes(snapshot.haveStorageHealth ? snapshot.storage.staging.quotaBytes : 0U);
    record.set_storageStagingWatermarkBytes(snapshot.haveStorageHealth ? snapshot.storage.staging.watermarkBytes : 0U);
    record.set_storageStagingQuotaStatus(snapshot.haveStorageHealth ? snapshot.storage.staging.quotaStatus : 0U);
    record.set_storageStagingRetentionStatus(snapshot.haveStorageHealth ? snapshot.storage.staging.retentionStatus : 0U);
    record.set_storageLogsExists(snapshot.haveStorageHealth && snapshot.storage.logs.exists);
    record.set_storageLogsScanOk(snapshot.haveStorageHealth && snapshot.storage.logs.scanOk);
    record.set_storageLogsFileCount(snapshot.haveStorageHealth ? snapshot.storage.logs.fileCount : 0U);
    record.set_storageLogsBytes(snapshot.haveStorageHealth ? snapshot.storage.logs.totalBytes : 0U);
    record.set_storageLogsErrorCode(snapshot.haveStorageHealth ? snapshot.storage.logs.errorCode : 0U);
    record.set_storageLogsQuotaBytes(snapshot.haveStorageHealth ? snapshot.storage.logs.quotaBytes : 0U);
    record.set_storageLogsWatermarkBytes(snapshot.haveStorageHealth ? snapshot.storage.logs.watermarkBytes : 0U);
    record.set_storageLogsQuotaStatus(snapshot.haveStorageHealth ? snapshot.storage.logs.quotaStatus : 0U);
    record.set_storageLogsRetentionStatus(snapshot.haveStorageHealth ? snapshot.storage.logs.retentionStatus : 0U);
    record.set_storageDataProductsExists(snapshot.haveStorageHealth && snapshot.storage.dataProducts.exists);
    record.set_storageDataProductsScanOk(snapshot.haveStorageHealth && snapshot.storage.dataProducts.scanOk);
    record.set_storageDataProductsFileCount(snapshot.haveStorageHealth ? snapshot.storage.dataProducts.fileCount : 0U);
    record.set_storageDataProductsBytes(snapshot.haveStorageHealth ? snapshot.storage.dataProducts.totalBytes : 0U);
    record.set_storageDataProductsErrorCode(snapshot.haveStorageHealth ? snapshot.storage.dataProducts.errorCode : 0U);
    record.set_storageDataProductsQuotaBytes(snapshot.haveStorageHealth ? snapshot.storage.dataProducts.quotaBytes : 0U);
    record.set_storageDataProductsWatermarkBytes(snapshot.haveStorageHealth ? snapshot.storage.dataProducts.watermarkBytes : 0U);
    record.set_storageDataProductsQuotaStatus(snapshot.haveStorageHealth ? snapshot.storage.dataProducts.quotaStatus : 0U);
    record.set_storageDataProductsRetentionStatus(snapshot.haveStorageHealth ? snapshot.storage.dataProducts.retentionStatus : 0U);

    record.set_commActiveBand(snapshot.commActiveBand);
    record.set_commPassActive(snapshot.commPassActive);
    record.set_commPassRemainingSec(reduced.commPassRemainingSec);
    record.set_commTotalPasses(reduced.commTotalPasses);
    record.set_cspInitialized(snapshot.cspInitialized);
    record.set_cspLocalNodeId(snapshot.cspLocalNodeId);
    record.set_cspTxPackets(reduced.cspTxPackets);
    record.set_cspRxPackets(reduced.cspRxPackets);
    record.set_cspErrorCount(snapshot.cspErrorCount);
    record.set_cspFreeBuffers(snapshot.cspFreeBuffers);
    record.set_uartConnected(snapshot.uartConnected);
    record.set_uartTxBytes(snapshot.uartTxBytes);
    record.set_uartRxBytes(snapshot.uartRxBytes);
    record.set_uartTxErrors(snapshot.uartTxErrors);
    record.set_uartRxErrors(snapshot.uartRxErrors);
    record.set_radioLinkConnected(snapshot.radioLinkConnected);
    record.set_haveRadioStatus(snapshot.haveRadioStatus);
    if (snapshot.haveRadioStatus) {
        record.set_radioEnabled(snapshot.radioEnabled);
        record.set_radioPowerDbm(snapshot.radioPowerDbm);
        record.set_radioFreqHz(snapshot.radioFreqHz);
        record.set_radioTemperatureC(snapshot.radioTemperatureC);
        record.set_radioRssiDbm(snapshot.radioRssiDbm);
    }
    record.set_radioTxBytes(reduced.radioTxBytes);
    record.set_radioRxBytes(reduced.radioRxBytes);
    record.set_radioTxErrors(snapshot.radioTxErrors);
    record.set_radioRxErrors(snapshot.radioRxErrors);
    record.set_pendingBootSlot(snapshot.pendingBootSlot);
    record.set_lastKnownGoodBootSlot(snapshot.lastKnownGoodBootSlot);
    record.set_bootConfirmed(snapshot.bootConfirmed);
    record.set_bootStageVerified(snapshot.bootStageVerified);
    record.set_bootExpectedSize(snapshot.bootExpectedSize);
    record.set_bootLastBootAttemptTime(snapshot.bootLastBootAttemptTime);
    record.set_bootLastErrorCode(snapshot.bootLastErrorCode);
    record.set_bootRemainingConfirmSeconds(snapshot.bootRemainingConfirmSeconds);
    record.set_bootUpdateProgress(snapshot.bootUpdateProgress);
    record.set_healthMask(reduced.healthMask);
    record.set_faultMask(reduced.faultMask);
    record.set_qualityMask(reduced.qualityMask);
    return record;
}

HkTrendProductStatus HkTrendProductProducer::flushPending_(OBC::HkTrendFlushReason reason) {
    if (this->m_pendingChunkSampleCount == 0U) {
        return HkTrendProductStatus::OK;
    }

    if (!this->haveProductPorts_()) {
        this->publishState_(HkTrendProductStatus::DP_PORTS_NOT_CONNECTED);
        this->log_WARNING_HI_HK_TREND_PRODUCT_REJECTED(this->m_pendingSamples.front().get_sequence(),
                                                       static_cast<U32>(HkTrendProductStatus::DP_PORTS_NOT_CONNECTED));
        return HkTrendProductStatus::DP_PORTS_NOT_CONNECTED;
    }

    const FwSizeType sampleCount = this->m_pendingChunkSampleCount;
    const FwSizeType dpSize = this->computePendingDataSize_(sampleCount);
    DpContainer container;
    const Fw::Success stat = this->dpGet_HkTrendContainer(dpSize, container);
    if (stat == Fw::Success::FAILURE) {
        this->publishState_(HkTrendProductStatus::DP_BUFFER_UNAVAILABLE);
        this->log_WARNING_HI_HK_TREND_PRODUCT_REJECTED(this->m_pendingSamples.front().get_sequence(),
                                                       static_cast<U32>(HkTrendProductStatus::DP_BUFFER_UNAVAILABLE));
        return HkTrendProductStatus::DP_BUFFER_UNAVAILABLE;
    }

    OBC::HkTrendChunkMetaV1 meta = {};
    meta.set_version(HK_TREND_CHUNK_META_VERSION);
    meta.set_chunkSequence(this->m_nextChunkSequence);
    meta.set_firstSampleSequence(this->m_pendingSamples.front().get_sequence());
    meta.set_lastSampleSequence(this->m_pendingSamples.at(sampleCount - 1U).get_sequence());
    meta.set_sampleCount(static_cast<U32>(sampleCount));
    meta.set_targetFileBytes(this->m_targetFileBytes);
    meta.set_flushReason(reason);

    Fw::SerializeStatus serializeStatus = container.serializeRecord_HkTrendChunkMeta(meta);
    if (serializeStatus == Fw::SerializeStatus::FW_SERIALIZE_OK) {
        serializeStatus = container.serializeRecord_HkTrendRecord(this->m_pendingSamples.data(), sampleCount);
    }
    if (serializeStatus != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        Fw::Buffer buffer = container.getBuffer();
        container.invalidateBuffer();
        this->productBufferReturnOut_out(0, buffer);
        this->publishState_(HkTrendProductStatus::SERIALIZE_ERROR);
        this->log_WARNING_HI_HK_TREND_PRODUCT_REJECTED(this->m_pendingSamples.front().get_sequence(),
                                                       static_cast<U32>(HkTrendProductStatus::SERIALIZE_ERROR));
        return HkTrendProductStatus::SERIALIZE_ERROR;
    }

    this->m_lastSizeBytes = static_cast<U32>(container.getPacketSize());
    this->dpSend(container);
    this->m_lastChunkSequence = this->m_nextChunkSequence;
    this->m_nextChunkSequence++;
    this->m_productCount++;
    this->m_pendingSamples.erase(this->m_pendingSamples.begin(), this->m_pendingSamples.begin() + sampleCount);
    this->recomputePendingChunkSampleCount_();
    this->publishState_(HkTrendProductStatus::OK);
    this->log_ACTIVITY_LO_HK_TREND_PRODUCT_WRITTEN(meta.get_chunkSequence(),
                                                   meta.get_firstSampleSequence(),
                                                   meta.get_lastSampleSequence(),
                                                   meta.get_sampleCount(),
                                                   this->m_lastSizeBytes,
                                                   reason);
    return HkTrendProductStatus::OK;
}

bool HkTrendProductProducer::haveProductPorts_() {
    return this->isConnected_productGetOut_OutputPort(0) && this->isConnected_productSendOut_OutputPort(0) &&
           this->isConnected_productBufferReturnOut_OutputPort(0);
}

U32 HkTrendProductProducer::normalizeTargetFileBytes_(U32 requested) const {
    const U32 minimum = this->computePendingPacketSize_(1U);
    if (requested < minimum) {
        return minimum;
    }
    if (requested > MAX_TARGET_FILE_BYTES) {
        return MAX_TARGET_FILE_BYTES;
    }
    return requested;
}

FwSizeType HkTrendProductProducer::computePendingDataSize_(FwSizeType sampleCount) const {
    if (sampleCount == 0U) {
        return 0U;
    }
    return sizeof(FwDpIdType) + sizeof(FwSizeStoreType) + sampleCount * OBC::HkTrendRecordV6::SERIALIZED_SIZE +
           sizeof(FwDpIdType) + OBC::HkTrendChunkMetaV1::SERIALIZED_SIZE;
}

U32 HkTrendProductProducer::computePendingPacketSize_(FwSizeType sampleCount) const {
    if (sampleCount == 0U) {
        return 0U;
    }
    return static_cast<U32>(Fw::DpContainer::getPacketSizeForDataSize(this->computePendingDataSize_(sampleCount)));
}

FwSizeType HkTrendProductProducer::computeMaxPendingSampleCount_() const {
    FwSizeType maxChunkSampleCount = 0U;
    for (FwSizeType sampleCount = 1U;; sampleCount++) {
        if (this->computePendingPacketSize_(sampleCount) > MAX_TARGET_FILE_BYTES) {
            break;
        }
        maxChunkSampleCount = sampleCount;
    }
    return maxChunkSampleCount > 0U ? (maxChunkSampleCount * MAX_PENDING_CHUNKS) : 1U;
}

U32 HkTrendProductProducer::getPendingEstimatedBytes_() const {
    return this->computePendingPacketSize_(this->m_pendingChunkSampleCount);
}

void HkTrendProductProducer::recomputePendingChunkSampleCount_() {
    this->m_pendingChunkSampleCount = 0U;
    for (FwSizeType sampleCount = 1U; sampleCount <= static_cast<FwSizeType>(this->m_pendingSamples.size());
         sampleCount++) {
        if (this->computePendingPacketSize_(sampleCount) > this->m_targetFileBytes) {
            break;
        }
        this->m_pendingChunkSampleCount = sampleCount;
    }
}

HkTrendProductStatus HkTrendProductProducer::retainPendingSample_(const OBC::HkTrendRecordV6& record, U32 sequence) {
    if (this->m_pendingSamples.size() >= static_cast<size_t>(this->computeMaxPendingSampleCount_())) {
        this->m_lastSequence = sequence;
        this->m_nextSampleSequence++;
        this->publishState_(HkTrendProductStatus::BACKLOG_FULL);
        this->log_WARNING_HI_HK_TREND_PRODUCT_REJECTED(sequence,
                                                       static_cast<U32>(HkTrendProductStatus::BACKLOG_FULL));
        return HkTrendProductStatus::BACKLOG_FULL;
    }

    this->m_pendingSamples.push_back(record);
    this->recomputePendingChunkSampleCount_();
    this->m_lastSequence = sequence;
    this->m_nextSampleSequence++;
    return HkTrendProductStatus::OK;
}

void HkTrendProductProducer::publishState_(HkTrendProductStatus status) {
    this->m_lastError = status;
    this->publishTelemetry_();
}

void HkTrendProductProducer::publishTelemetry_() {
    this->tlmWrite_HK_TREND_PRODUCT_COUNT(this->m_productCount);
    this->tlmWrite_HK_TREND_LAST_SEQUENCE(this->m_lastSequence);
    this->tlmWrite_HK_TREND_LAST_SIZE_BYTES(this->m_lastSizeBytes);
    this->tlmWrite_HK_TREND_LAST_ERROR(static_cast<U32>(this->m_lastError));
    this->tlmWrite_HK_TREND_LAST_CHUNK_SEQUENCE(this->m_lastChunkSequence);
    this->tlmWrite_HK_TREND_PENDING_SAMPLE_COUNT(static_cast<U32>(this->m_pendingSamples.size()));
    this->tlmWrite_HK_TREND_PENDING_ESTIMATED_BYTES(this->getPendingEstimatedBytes_());
    this->tlmWrite_HK_TREND_TARGET_FILE_BYTES(this->m_targetFileBytes);
}

void HkTrendProductProducer::resetTicks_() {
    this->m_ticksUntilCapture = this->m_periodTicks > 0U ? (this->m_periodTicks - 1U) : 0U;
}

}  // namespace OBC
