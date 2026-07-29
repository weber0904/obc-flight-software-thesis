#include "PayloadOpsControllerTester.hpp"

#include <thread>
#include <type_traits>
#include <vector>

#include "Fw/Dp/DpContainer.hpp"
#include "Fw/Types/StringTemplate.hpp"
#include "simulators/eps/EpsTypes.hpp"

namespace OBC {

namespace {

constexpr U32 DETAIL_SESSION_REPREPARE_REQUIRED_EXPECTED = 25U;

}

namespace {

constexpr U8 STUB_JPEG_BYTES[] = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
    0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xD9,
};

U32 widthForResolution_(OBC::PayloadResolutionPreset preset) {
    switch (preset.e) {
        case OBC::PayloadResolutionPreset::PRESET_VGA_640X480:
            return 640U;
        case OBC::PayloadResolutionPreset::PRESET_FULL_3280X2464:
            return 3280U;
        case OBC::PayloadResolutionPreset::PRESET_HD_1280X720:
        default:
            return 1280U;
    }
}

U32 heightForResolution_(OBC::PayloadResolutionPreset preset) {
    switch (preset.e) {
        case OBC::PayloadResolutionPreset::PRESET_VGA_640X480:
            return 480U;
        case OBC::PayloadResolutionPreset::PRESET_FULL_3280X2464:
            return 2464U;
        case OBC::PayloadResolutionPreset::PRESET_HD_1280X720:
        default:
            return 720U;
    }
}

}  // namespace

class PayloadOpsControllerTester::DeterministicDualArtifactDriver final : public OBC::IPiCameraDriver {
  public:
    const char* getName() const override { return "dual-artifact-test"; }

    bool isAvailable() const override { return true; }

    OBC::PayloadCapabilities getCapabilities() const override {
        OBC::PayloadCapabilities capabilities = {};
        capabilities.backendName = "dual-artifact-test";
        capabilities.cameraModel = "OV5647-dual-artifact-test";
        capabilities.supportedMask = OBC::PayloadFieldMask::RESOLUTION | OBC::PayloadFieldMask::JPEG_QUALITY |
                                     OBC::PayloadFieldMask::HFLIP | OBC::PayloadFieldMask::VFLIP |
                                     OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100 |
                                     OBC::PayloadFieldMask::AWB_MODE | OBC::PayloadFieldMask::METERING_MODE |
                                     OBC::PayloadFieldMask::EV_COMP_X100;
        capabilities.offOnlyMask = OBC::PayloadFieldMask::RESOLUTION | OBC::PayloadFieldMask::JPEG_QUALITY |
                                   OBC::PayloadFieldMask::HFLIP | OBC::PayloadFieldMask::VFLIP;
        capabilities.autoMutableMask = OBC::PayloadFieldMask::AWB_MODE | OBC::PayloadFieldMask::METERING_MODE |
                                       OBC::PayloadFieldMask::EV_COMP_X100;
        capabilities.deterministicMutableMask =
            OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100;
        capabilities.rawRegisterSupported = true;
        return capabilities;
    }

    Fw::CmdResponse prepare(const OBC::PayloadReadyKind readyKind,
                            const OBC::PayloadCameraSettings& settings,
                            U32 initTimeoutMs,
                            const std::atomic<bool>& cancelRequested,
                            U32& detailCode) override {
        static_cast<void>(readyKind);
        static_cast<void>(initTimeoutMs);
        if (cancelRequested.load()) {
            detailCode = 1U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        this->m_preparedSettings = settings;
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse captureStill(const OBC::PayloadCaptureRequest& request,
                                 const OBC::PayloadCameraSettings& settings,
                                 U32 captureTimeoutMs,
                                 const std::atomic<bool>& cancelRequested,
                                 OBC::PayloadCaptureMetadata& metadata,
                                 U32& detailCode) override {
        static_cast<void>(captureTimeoutMs);
        if (cancelRequested.load()) {
            detailCode = 2U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        const U32 width = widthForResolution_(this->m_preparedSettings.resolution);
        const U32 height = heightForResolution_(this->m_preparedSettings.resolution);
        const std::size_t rawBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 2U;
        std::vector<U8> raw(rawBytes, 0U);
        for (std::size_t index = 0; index < raw.size(); ++index) {
            raw[index] = static_cast<U8>((index + request.captureId + request.captureIndex) & 0xFFU);
        }

        if (!OBC::TestSupport::writeBinaryFile(request.rawOutputPath, raw)) {
            detailCode = 3U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (!OBC::TestSupport::writeBinaryFile(
                request.previewOutputPath, std::vector<U8>(std::begin(STUB_JPEG_BYTES), std::end(STUB_JPEG_BYTES)))) {
            detailCode = 4U;
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        metadata = request.metadata;
        metadata.backendName = "dual-artifact-test";
        metadata.cameraModel = "OV5647-dual-artifact-test";
        metadata.captureTimeSec = 1700000000U + request.captureId;
        metadata.captureTimeUsec = request.captureIndex;
        metadata.pixelFormat = OBC::PayloadPixelFormat::PIXEL_YUYV;
        metadata.imageWidth = width;
        metadata.imageHeight = height;
        metadata.rawBytes = static_cast<U32>(raw.size());
        metadata.previewJpegBytes = static_cast<U32>(sizeof(STUB_JPEG_BYTES));
        metadata.appliedSettings = settings;
        metadata.capturePolicy = settings.capturePolicy;
        metadata.actualExposureUsec =
            settings.capturePolicy.e == OBC::PayloadCapturePolicy::CAPTURE_AUTO ? 23456U : settings.exposureUsec;
        metadata.actualGainX100 =
            settings.capturePolicy.e == OBC::PayloadCapturePolicy::CAPTURE_AUTO ? 163U : settings.gainX100;
        metadata.actualAwbValid = settings.capturePolicy.e == OBC::PayloadCapturePolicy::CAPTURE_AUTO;
        metadata.actualAwbColorTemperatureK = metadata.actualAwbValid ? 4912U : 0U;
        metadata.actualAwbRedGainX1000 = metadata.actualAwbValid ? 1684U : 0U;
        metadata.actualAwbBlueGainX1000 = metadata.actualAwbValid ? 1422U : 0U;
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse shutdown(U32& detailCode) override {
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse readSensorRegister(U32 address, U32 timeoutMs, U32& value, U32& detailCode) override {
        static_cast<void>(address);
        static_cast<void>(timeoutMs);
        value = this->m_registerValue;
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    Fw::CmdResponse writeSensorRegister(U32 address,
                                        U32 value,
                                        bool verifyReadback,
                                        U32 timeoutMs,
                                        U32& readbackValue,
                                        U32& detailCode) override {
        static_cast<void>(address);
        static_cast<void>(verifyReadback);
        static_cast<void>(timeoutMs);
        this->m_registerValue = value & 0xFFU;
        readbackValue = this->m_registerValue;
        detailCode = 0U;
        return Fw::CmdResponse::OK;
    }

    void abort() override {}

    const OBC::PayloadCameraSettings& getPreparedSettings() const { return this->m_preparedSettings; }

  private:
    OBC::PayloadCameraSettings m_preparedSettings = {};
    U32 m_registerValue = 0x10U;
};

PayloadOpsControllerTester::PayloadOpsControllerTester()
    : PayloadOpsControllerGTestBase("PayloadOpsControllerTester", MAX_HISTORY_SIZE),
      m_runtimeRoot(),
      m_mode(),
      m_eps(),
      m_boot(),
      m_driver(new DeterministicDualArtifactDriver()),
      m_dpBuffer(),
      component("PayloadOpsController") {
    EXPECT_TRUE(this->m_runtimeRoot.valid());
    this->initComponents();
    this->connectPorts();
    EXPECT_TRUE(this->component.configureRuntime(this->m_runtimeRoot.path(),
                                                 &this->m_mode,
                                                 &this->m_eps,
                                                 &this->m_boot,
                                                 std::unique_ptr<OBC::IPiCameraDriver>(this->m_driver)));
}

PayloadOpsControllerTester::~PayloadOpsControllerTester() = default;

bool PayloadOpsControllerTester::FakeEpsControl::getCachedStatusForRuntime(OBC::EPS::StatusData& status) const {
    status = {};
    status.soc = 80.0F;
    return true;
}

Fw::CmdResponse PayloadOpsControllerTester::FakeEpsControl::setPayloadProxyPower(bool enabled,
                                                                                 OBC::EPS::StatusData& status) {
    status = {};
    status.soc = 80.0F;
    this->callCount += 1U;
    this->lastEnabled = enabled;
    return Fw::CmdResponse::OK;
}

Fw::Success::T PayloadOpsControllerTester::productGet_handler(FwDpIdType id,
                                                              FwSizeType dataSize,
                                                              Fw::Buffer& buffer) {
    this->pushProductGetEntry(id, dataSize);
    if (this->m_failNextProductGet) {
        this->m_failNextProductGet = false;
        return Fw::Success::FAILURE;
    }
    if (dataSize > sizeof(this->m_dpBuffer)) {
        return Fw::Success::FAILURE;
    }
    buffer.set(this->m_dpBuffer.data(), dataSize);
    return Fw::Success::SUCCESS;
}

void PayloadOpsControllerTester::driveTicks(U32 count, U32 sleepMs) {
    for (U32 index = 0; index < count; ++index) {
        this->invoke_to_schedIn(0, 0U);
        if (sleepMs > 0U) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        }
    }
}

void PayloadOpsControllerTester::waitForProductSendCount_(U32 expectedCount, U32 maxTicks, U32 sleepMs) {
    for (U32 tick = 0U; tick < maxTicks && this->productSendHistory->size() < expectedCount; ++tick) {
        this->driveTicks(1U, sleepMs);
    }
    ASSERT_GE(this->productSendHistory->size(), expectedCount);
}

void PayloadOpsControllerTester::prepareGeneric_() {
    const U32 startingResponses = this->cmdResponseHistory->size();
    this->sendCmd_PAYLOAD_PREPARE(TEST_INSTANCE_ID, startingResponses);
    this->driveTicks(250U, 10U);
    ASSERT_EQ(this->cmdResponseHistory->size(), startingResponses + 1U);
    ASSERT_CMD_RESPONSE(startingResponses, this->component.OPCODE_PAYLOAD_PREPARE, startingResponses, Fw::CmdResponse::OK);
    ASSERT_EQ(this->getStatus_().state, OBC::PayloadState::PSTATE_READY);
}

void PayloadOpsControllerTester::prepareNonRaw_() {
    const U32 startingResponses = this->cmdResponseHistory->size();
    this->sendCmd_PAYLOAD_PREPARE(TEST_INSTANCE_ID, startingResponses);
    this->driveTicks(250U, 10U);
    ASSERT_EQ(this->cmdResponseHistory->size(), startingResponses + 1U);
    ASSERT_CMD_RESPONSE(startingResponses, this->component.OPCODE_PAYLOAD_PREPARE, startingResponses, Fw::CmdResponse::OK);
    ASSERT_EQ(this->getStatus_().state, OBC::PayloadState::PSTATE_READY);
}

void PayloadOpsControllerTester::captureDeterministic_(U32 cmdSeq,
                                                       U8 captureIndex,
                                                       const char* tag,
                                                       U32 applyMask,
                                                       U32 exposureUsec,
                                                       U32 gainX100) {
    this->sendCmd_PAYLOAD_CAPTURE_DETERMINISTIC(TEST_INSTANCE_ID,
                                                cmdSeq,
                                                captureIndex,
                                                Fw::StringTemplate<64>(tag),
                                                applyMask,
                                                exposureUsec,
                                                gainX100);
}

std::string PayloadOpsControllerTester::dataProductPathFromSendHistory_(U32 historyIndex) const {
    if (this->productSendHistory->size() <= historyIndex) {
        return std::string();
    }

    Fw::Buffer buffer = this->productSendHistory->at(historyIndex).buffer;
    OBC::PayloadOpsControllerComponentBase::DpContainer container;
    container.setBuffer(buffer);
    if (container.deserializeHeader() != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return std::string();
    }

    auto deserializer = buffer.getDeserializer();
    if (deserializer.moveDeserToOffset(Fw::DpContainer::DATA_OFFSET) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return std::string();
    }

    FwDpIdType recordId = 0U;
    if (deserializer.deserializeTo(recordId) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return std::string();
    }
    if (recordId != this->component.getIdBase() +
                        OBC::PayloadOpsControllerComponentBase::RecordId::PayloadCaptureArtifactHeader) {
        return std::string();
    }

    OBC::PayloadCaptureArtifactHeaderV2 header = {};
    if (deserializer.deserializeTo(header) != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return std::string();
    }
    const bool raw = header.get_artifactKind() == OBC::PayloadArtifactKind::RAW_FRAME;
    const char* relative = raw ? header.get_rawDataProductRelativePath().toChar()
                               : header.get_previewDataProductRelativePath().toChar();
    return OBC::TestSupport::joinPath(this->m_runtimeRoot.path(), relative);
}

void PayloadOpsControllerTester::completePendingPublishes_() {
    U32 completedWrites = 0U;
    for (U32 guard = 0U; guard < 128U; ++guard) {
        const OBC::PayloadStatusSnapshot status = this->getStatus_();
        if (status.state != OBC::PayloadState::PSTATE_PUBLISHING) {
            ASSERT_FALSE(status.busy);
            return;
        }
        ASSERT_GT(this->productSendHistory->size(), completedWrites);
        const std::string path = this->dataProductPathFromSendHistory_(completedWrites);
        ASSERT_FALSE(path.empty());
        Fw::Buffer buffer = this->productSendHistory->at(completedWrites).buffer;
        this->invoke_to_dpWrittenIn(0, Fw::String(path.c_str()), 25U, buffer.getSize());
        this->invoke_to_schedIn(0, 0U);
        completedWrites += 1U;
    }
    FAIL() << "Timed out completing pending payload data-product publishes";
}

std::string PayloadOpsControllerTester::rawPathForIndex_(U8 captureIndex) const {
    char leaf[16] = {};
    std::snprintf(leaf, sizeof(leaf), "PIC%02X.bin", captureIndex);
    return TestSupport::joinPath(this->m_runtimeRoot.path(), TestSupport::joinPath("persistent-data/payload/camera", leaf));
}

std::string PayloadOpsControllerTester::previewPathForIndex_(U8 captureIndex) const {
    char leaf[16] = {};
    std::snprintf(leaf, sizeof(leaf), "PIC%02X.jpg", captureIndex);
    return TestSupport::joinPath(this->m_runtimeRoot.path(), TestSupport::joinPath("persistent-data/payload/camera", leaf));
}

std::string PayloadOpsControllerTester::manifestPathForIndex_(U8 captureIndex) const {
    char leaf[20] = {};
    std::snprintf(leaf, sizeof(leaf), "PIC%02X.meta.bin", captureIndex);
    return TestSupport::joinPath(this->m_runtimeRoot.path(),
                                 TestSupport::joinPath("persistent-data/payload/camera/catalog", leaf));
}

OBC::PayloadCaptureMetadata PayloadOpsControllerTester::getLastMetadata_() const {
    OBC::PayloadCaptureMetadata metadata = {};
    EXPECT_TRUE(this->component.getLastCaptureMetadataForRuntime(metadata));
    return metadata;
}

OBC::PayloadStatusSnapshot PayloadOpsControllerTester::getStatus_() const {
    OBC::PayloadStatusSnapshot status = {};
    EXPECT_TRUE(this->component.getStatusSnapshotForRuntime(status));
    return status;
}

void PayloadOpsControllerTester::testCaptureDeterministicWritesDualArtifactsAndAutoPublishesPreview() {
    this->clearHistory();
    this->prepareNonRaw_();

    const U32 sendBaseline = this->productSendHistory->size();
    this->captureDeterministic_(1, 0x2AU, "dual");
    this->waitForProductSendCount_(sendBaseline + 1U);

    ASSERT_CMD_RESPONSE_SIZE(2);
    ASSERT_CMD_RESPONSE(1, this->component.OPCODE_PAYLOAD_CAPTURE_DETERMINISTIC, 1, Fw::CmdResponse::OK);
    ASSERT_EQ(this->eventHistory_PAYLOAD_CAPTURED->size(), 0U);
    ASSERT_EQ(this->productSendHistory->size(), 1U);

    OBC::PayloadCaptureMetadata metadata = this->getLastMetadata_();
    ASSERT_EQ(metadata.captureId, 1U);
    ASSERT_EQ(metadata.captureIndex, 0x2AU);
    ASSERT_EQ(metadata.rawRelativePath, "persistent-data/payload/camera/PIC2A.bin");
    ASSERT_EQ(metadata.previewRelativePath, "persistent-data/payload/camera/PIC2A.jpg");
    ASSERT_FALSE(metadata.previewDataProductRelativePath.empty());
    ASSERT_FALSE(metadata.previewDataProductPublished);
    ASSERT_FALSE(metadata.rawDataProductPublished);
    ASSERT_EQ(metadata.rawBytes, 1280U * 720U * 2U);
    ASSERT_EQ(metadata.previewJpegBytes, static_cast<U32>(sizeof(STUB_JPEG_BYTES)));
    ASSERT_FALSE(TestSupport::readBinaryFile(this->rawPathForIndex_(0x2AU)).empty());
    ASSERT_FALSE(TestSupport::readBinaryFile(this->previewPathForIndex_(0x2AU)).empty());

    this->completePendingPublishes_();

    metadata = this->getLastMetadata_();
    ASSERT_TRUE(metadata.previewDataProductPublished);
    ASSERT_FALSE(metadata.rawDataProductPublished);
    ASSERT_GT(metadata.previewDataProductBytes, 0U);
    ASSERT_EQ(metadata.lastPublishedArtifactKind, OBC::PayloadArtifactKind::PREVIEW_JPEG);
    ASSERT_EQ(this->eventHistory_PAYLOAD_CAPTURED->size(), 1U);
    ASSERT_EVENTS_PAYLOAD_CAPTURED(0,
                                   1U,
                                   0x2AU,
                                   "persistent-data/payload/camera/PIC2A.bin",
                                   "persistent-data/payload/camera/PIC2A.jpg");
    ASSERT_TLM_PAYLOAD_LAST_CAPTURE_INDEX_SIZE(1);
    ASSERT_TLM_PAYLOAD_LAST_CAPTURE_INDEX(0, 0x2AU);
    ASSERT_TLM_PAYLOAD_LAST_DATA_PRODUCT_PUBLISHED_SIZE(1);
    ASSERT_TLM_PAYLOAD_LAST_DATA_PRODUCT_PUBLISHED(0, true);
    ASSERT_EQ(this->getStatus_().state, OBC::PayloadState::PSTATE_READY);
}

void PayloadOpsControllerTester::testCaptureDeterministicAllowsZeroCaptureIndex() {
    this->clearHistory();
    this->prepareNonRaw_();

    U32 sendBaseline = this->productSendHistory->size();
    this->captureDeterministic_(1, 0x2AU, "seed");
    this->waitForProductSendCount_(sendBaseline + 1U);
    this->completePendingPublishes_();

    this->clearHistory();
    sendBaseline = this->productSendHistory->size();
    this->captureDeterministic_(2, 0x00U, "zero");
    this->waitForProductSendCount_(sendBaseline + 1U);
    this->completePendingPublishes_();

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_CAPTURE_DETERMINISTIC, 2, Fw::CmdResponse::OK);

    const OBC::PayloadCaptureMetadata metadata = this->getLastMetadata_();
    ASSERT_EQ(metadata.captureIndex, 0x00U);
    ASSERT_EQ(metadata.rawRelativePath, "persistent-data/payload/camera/PIC00.bin");
    ASSERT_EQ(metadata.previewRelativePath, "persistent-data/payload/camera/PIC00.jpg");
    ASSERT_TRUE(metadata.previewDataProductPublished);
    ASSERT_TLM_PAYLOAD_LAST_CAPTURE_INDEX_SIZE(1);
    ASSERT_TLM_PAYLOAD_LAST_CAPTURE_INDEX(0, 0x00U);
    ASSERT_EVENTS_PAYLOAD_CAPTURED(0,
                                   2U,
                                   0x00U,
                                   "persistent-data/payload/camera/PIC00.bin",
                                   "persistent-data/payload/camera/PIC00.jpg");
}

void PayloadOpsControllerTester::testPublishRawPromotesStoredCapture() {
    this->clearHistory();
    this->sendCmd_PAYLOAD_SET_CAMERA_DEFAULTS(
        TEST_INSTANCE_ID, 0, OBC::PayloadResolutionPreset::PRESET_VGA_640X480, 90U, false, false);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SET_CAMERA_DEFAULTS, 0, Fw::CmdResponse::OK);
    this->prepareNonRaw_();

    const U32 captureSendBaseline = this->productSendHistory->size();
    this->captureDeterministic_(2, 0x2BU, "raw", 0U, 10000U, 100U);
    this->waitForProductSendCount_(captureSendBaseline + 1U);
    this->completePendingPublishes_();

    this->clearHistory();
    const U32 rawSendBaseline = this->productSendHistory->size();
    this->sendCmd_PAYLOAD_PUBLISH_CAPTURE(TEST_INSTANCE_ID, 3, 0x2BU, OBC::PayloadArtifactKind::RAW_FRAME);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_PUBLISH_CAPTURE, 3, Fw::CmdResponse::OK);
    this->waitForProductSendCount_(rawSendBaseline + 1U);
    this->completePendingPublishes_();

    const OBC::PayloadCaptureMetadata metadata = this->getLastMetadata_();
    ASSERT_TRUE(metadata.previewDataProductPublished);
    ASSERT_TRUE(metadata.rawDataProductPublished);
    ASSERT_GT(metadata.previewDataProductBytes, 0U);
    ASSERT_GT(metadata.rawDataProductBytes, 0U);
    ASSERT_EQ(metadata.lastPublishedArtifactKind, OBC::PayloadArtifactKind::RAW_FRAME);
    ASSERT_FALSE(metadata.rawDataProductRelativePath.empty());
    ASSERT_TLM_PAYLOAD_LAST_RAW_DATA_PRODUCT_PUBLISHED_SIZE(1);
    ASSERT_TLM_PAYLOAD_LAST_RAW_DATA_PRODUCT_PUBLISHED(0, true);
    ASSERT_TLM_PAYLOAD_LAST_PUBLISHED_ARTIFACT_KIND_SIZE(1);
    ASSERT_TLM_PAYLOAD_LAST_PUBLISHED_ARTIFACT_KIND(0, OBC::PayloadArtifactKind::RAW_FRAME);
    ASSERT_EQ(this->getStatus_().state, OBC::PayloadState::PSTATE_READY);
}

void PayloadOpsControllerTester::testPublishRawRejectsFullResolution() {
    this->clearHistory();
    this->sendCmd_PAYLOAD_SET_CAMERA_DEFAULTS(
        TEST_INSTANCE_ID, 0, OBC::PayloadResolutionPreset::PRESET_FULL_3280X2464, 90U, false, false);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SET_CAMERA_DEFAULTS, 0, Fw::CmdResponse::OK);
    this->prepareNonRaw_();

    const U32 sendBaseline = this->productSendHistory->size();
    this->captureDeterministic_(4, 0x2CU, "full", 0U, 10000U, 100U);
    this->waitForProductSendCount_(sendBaseline + 1U, 250U);
    this->completePendingPublishes_();

    this->clearHistory();
    this->sendCmd_PAYLOAD_PUBLISH_CAPTURE(TEST_INSTANCE_ID, 5, 0x2CU, OBC::PayloadArtifactKind::RAW_FRAME);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_PUBLISH_CAPTURE, 5, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EQ(this->productSendHistory->size(), 0U);
    ASSERT_EQ(this->getStatus_().lastResult, OBC::PayloadResultCode::PRESULT_STORAGE_FAILED);
    ASSERT_EQ(this->getStatus_().lastDetail, 24U);

    const OBC::PayloadCaptureMetadata metadata = this->getLastMetadata_();
    ASSERT_FALSE(metadata.rawDataProductPublished);
    ASSERT_TRUE(metadata.previewDataProductPublished);
}

void PayloadOpsControllerTester::testReusingCaptureIndexOverwritesLocalArtifacts() {
    this->clearHistory();
    this->prepareNonRaw_();

    const U32 firstSendBaseline = this->productSendHistory->size();
    this->captureDeterministic_(6, 0x2DU, "first");
    this->waitForProductSendCount_(firstSendBaseline + 1U);
    this->completePendingPublishes_();

    const std::string rawPath = this->rawPathForIndex_(0x2DU);
    const std::string previewPath = this->previewPathForIndex_(0x2DU);
    ASSERT_TRUE(TestSupport::writeBinaryFile(rawPath, std::vector<U8>{0xAAU, 0xBBU}));
    ASSERT_TRUE(TestSupport::writeBinaryFile(previewPath, std::vector<U8>{0x00U}));

    this->clearHistory();
    const U32 secondSendBaseline = this->productSendHistory->size();
    this->captureDeterministic_(7, 0x2DU, "second");
    this->waitForProductSendCount_(secondSendBaseline + 1U);
    this->completePendingPublishes_();

    const std::vector<U8> rawBytes = TestSupport::readBinaryFile(rawPath);
    const std::vector<U8> previewBytes = TestSupport::readBinaryFile(previewPath);
    ASSERT_GT(rawBytes.size(), 2U);
    ASSERT_GT(previewBytes.size(), 2U);
    ASSERT_NE(rawBytes[0], 0xAAU);
    ASSERT_EQ(previewBytes[0], 0xFFU);
    ASSERT_EQ(previewBytes[1], 0xD8U);

    const OBC::PayloadCaptureMetadata metadata = this->getLastMetadata_();
    ASSERT_EQ(metadata.captureIndex, 0x2DU);
    ASSERT_EQ(metadata.captureId, 2U);
    ASSERT_EQ(metadata.rawRelativePath, "persistent-data/payload/camera/PIC2D.bin");
    ASSERT_EQ(metadata.previewRelativePath, "persistent-data/payload/camera/PIC2D.jpg");
}

void PayloadOpsControllerTester::testCaptureDeterministicRequiresPrepare() {
    this->clearHistory();

    this->captureDeterministic_(8, 0x2EU, "no-prepare");

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_CAPTURE_DETERMINISTIC, 8, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EQ(this->eventHistory_PAYLOAD_CAPTURED->size(), 0U);
    ASSERT_EQ(this->getStatus_().lastResult, OBC::PayloadResultCode::PRESULT_REJECTED_NOT_READY);
}

void PayloadOpsControllerTester::testSingleReadyAllowsAutoThenDeterministicWithoutReprepare() {
    this->clearHistory();
    this->prepareGeneric_();

    const U32 autoSendBaseline = this->productSendHistory->size();
    this->sendCmd_PAYLOAD_CAPTURE_AUTO(TEST_INSTANCE_ID,
                                       1,
                                       0x31U,
                                       Fw::StringTemplate<64>("auto"),
                                       0U,
                                       OBC::PayloadAwbMode::AWB_AUTO,
                                       OBC::PayloadMeteringMode::METER_CENTRE,
                                       0);
    this->waitForProductSendCount_(autoSendBaseline + 1U);
    this->completePendingPublishes_();

    OBC::PayloadCaptureMetadata metadata = this->getLastMetadata_();
    ASSERT_EQ(metadata.capturePolicy, OBC::PayloadCapturePolicy::CAPTURE_AUTO);
    ASSERT_EQ(metadata.captureId, 1U);
    ASSERT_TRUE(metadata.actualAwbValid);
    ASSERT_EQ(metadata.actualExposureUsec, 23456U);
    ASSERT_EQ(metadata.actualGainX100, 163U);
    ASSERT_EQ(metadata.actualAwbColorTemperatureK, 4912U);

    this->clearHistory();
    const U32 deterministicSendBaseline = this->productSendHistory->size();
    this->sendCmd_PAYLOAD_CAPTURE_DETERMINISTIC(TEST_INSTANCE_ID,
                                                2,
                                                0x32U,
                                                Fw::StringTemplate<64>("deterministic"),
                                                OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100,
                                                33000U,
                                                450U);
    this->waitForProductSendCount_(deterministicSendBaseline + 1U);
    this->completePendingPublishes_();

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_CAPTURE_DETERMINISTIC, 2, Fw::CmdResponse::OK);
    metadata = this->getLastMetadata_();
    ASSERT_EQ(metadata.capturePolicy, OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC);
    ASSERT_EQ(metadata.captureId, 2U);
    ASSERT_EQ(metadata.requestedMask, OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100);
    ASSERT_EQ(metadata.actualExposureUsec, 33000U);
    ASSERT_EQ(metadata.actualGainX100, 450U);
    ASSERT_FALSE(metadata.actualAwbValid);
    ASSERT_EQ(this->getStatus_().state, OBC::PayloadState::PSTATE_READY);
}

void PayloadOpsControllerTester::testRetiredImageTuningMaskIsRejected() {
    this->clearHistory();
    this->prepareGeneric_();

    this->clearHistory();
    const U32 sendBaseline = this->productSendHistory->size();
    this->sendCmd_PAYLOAD_CAPTURE_AUTO(TEST_INSTANCE_ID,
                                       41,
                                       0x38U,
                                       Fw::StringTemplate<64>("retired-mask"),
                                       OBC::PayloadFieldMask::BRIGHTNESS_X100,
                                       OBC::PayloadAwbMode::AWB_AUTO,
                                       OBC::PayloadMeteringMode::METER_CENTRE,
                                       0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_CAPTURE_AUTO, 41, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EQ(this->eventHistory_PAYLOAD_CAPTURED->size(), 0U);
    ASSERT_EQ(this->productSendHistory->size(), sendBaseline);
    ASSERT_EQ(this->getStatus_().lastResult, OBC::PayloadResultCode::PRESULT_REJECTED_UNSUPPORTED);
}

void PayloadOpsControllerTester::testSharedReadyPrepareUsesAutoWarmupDefaults() {
    this->clearHistory();

    this->sendCmd_PAYLOAD_SET_AUTO_DEFAULTS(
        TEST_INSTANCE_ID, 1, OBC::PayloadAwbMode::AWB_CLOUDY, OBC::PayloadMeteringMode::METER_MATRIX, -125);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SET_AUTO_DEFAULTS, 1, Fw::CmdResponse::OK);

    this->clearHistory();
    this->prepareGeneric_();

    const OBC::PayloadCameraSettings& preparedSettings = this->m_driver->getPreparedSettings();
    ASSERT_EQ(preparedSettings.readyKind, OBC::PayloadReadyKind::READY_NON_RAW);
    ASSERT_EQ(preparedSettings.capturePolicy, OBC::PayloadCapturePolicy::CAPTURE_AUTO);
    ASSERT_EQ(preparedSettings.awbMode, OBC::PayloadAwbMode::AWB_CLOUDY);
    ASSERT_EQ(preparedSettings.meteringMode, OBC::PayloadMeteringMode::METER_MATRIX);
    ASSERT_EQ(preparedSettings.evCompX100, -125);
}

void PayloadOpsControllerTester::testSetCameraDefaultsRejectsWhilePrepared() {
    this->clearHistory();
    this->prepareGeneric_();

    this->clearHistory();
    this->sendCmd_PAYLOAD_SET_CAMERA_DEFAULTS(
        TEST_INSTANCE_ID, 3, OBC::PayloadResolutionPreset::PRESET_VGA_640X480, 80U, false, false);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SET_CAMERA_DEFAULTS, 3, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EQ(this->getStatus_().lastResult, OBC::PayloadResultCode::PRESULT_REJECTED_BUSY);
}

void PayloadOpsControllerTester::testRawSensorSessionStillRequiresSpecialPrepare() {
    this->clearHistory();
    this->prepareGeneric_();

    this->clearHistory();
    this->sendCmd_PAYLOAD_SENSOR_REG_READ(TEST_INSTANCE_ID, 1, 0x350BU);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SENSOR_REG_READ, 1, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EQ(this->getStatus_().lastResult, OBC::PayloadResultCode::PRESULT_REJECTED_NOT_READY);

    this->clearHistory();
    this->sendCmd_PAYLOAD_PREPARE_RAW_SENSOR(TEST_INSTANCE_ID, 2);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_PREPARE_RAW_SENSOR, 2, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EQ(this->getStatus_().lastResult, OBC::PayloadResultCode::PRESULT_REJECTED_NOT_READY);
    ASSERT_EQ(this->getStatus_().lastDetail, DETAIL_SESSION_REPREPARE_REQUIRED_EXPECTED);

    this->clearHistory();
    this->sendCmd_PAYLOAD_SHUTDOWN(TEST_INSTANCE_ID, 3);
    this->driveTicks(250U, 10U);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SHUTDOWN, 3, Fw::CmdResponse::OK);

    this->clearHistory();
    this->sendCmd_PAYLOAD_PREPARE_RAW_SENSOR(TEST_INSTANCE_ID, 4);
    this->driveTicks(250U, 10U);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_PREPARE_RAW_SENSOR, 4, Fw::CmdResponse::OK);

    this->clearHistory();
    this->sendCmd_PAYLOAD_CAPTURE_DETERMINISTIC(TEST_INSTANCE_ID,
                                                22,
                                                0x37U,
                                                Fw::StringTemplate<64>("raw-ready-reject"),
                                                OBC::PayloadFieldMask::EXPOSURE_USEC | OBC::PayloadFieldMask::GAIN_X100,
                                                33000U,
                                                450U);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_CAPTURE_DETERMINISTIC, 22, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EQ(this->getStatus_().lastResult, OBC::PayloadResultCode::PRESULT_REJECTED_NOT_READY);

    this->clearHistory();
    this->sendCmd_PAYLOAD_SENSOR_REG_READ(TEST_INSTANCE_ID, 5, 0x350BU);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SENSOR_REG_READ, 5, Fw::CmdResponse::OK);
    ASSERT_EQ(this->eventHistory_PAYLOAD_SENSOR_REGISTER_VALUE->size(), 1U);

    this->clearHistory();
    this->sendCmd_PAYLOAD_PREPARE(TEST_INSTANCE_ID, 6);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_PREPARE, 6, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EQ(this->getStatus_().lastResult, OBC::PayloadResultCode::PRESULT_REJECTED_NOT_READY);
    ASSERT_EQ(this->getStatus_().lastDetail, DETAIL_SESSION_REPREPARE_REQUIRED_EXPECTED);

    this->clearHistory();
    this->sendCmd_PAYLOAD_SHUTDOWN(TEST_INSTANCE_ID, 7);
    this->driveTicks(250U, 10U);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SHUTDOWN, 7, Fw::CmdResponse::OK);

    this->clearHistory();
    this->sendCmd_PAYLOAD_PREPARE(TEST_INSTANCE_ID, 8);
    this->driveTicks(250U, 10U);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_PREPARE, 8, Fw::CmdResponse::OK);

    this->clearHistory();
    this->sendCmd_PAYLOAD_SENSOR_REG_READ(TEST_INSTANCE_ID, 9, 0x350BU);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SENSOR_REG_READ, 9, Fw::CmdResponse::VALIDATION_ERROR);
}

void PayloadOpsControllerTester::testPreviewPublishIgnoresUnrelatedDpWriterNotifications() {
    this->clearHistory();
    this->prepareNonRaw_();

    const U32 sendBaseline = this->productSendHistory->size();
    this->captureDeterministic_(9, 0x2FU, "noise");
    this->waitForProductSendCount_(sendBaseline + 1U);

    ASSERT_EQ(this->getStatus_().state, OBC::PayloadState::PSTATE_PUBLISHING);
    const std::string previewDpPath = this->dataProductPathFromSendHistory_(0U);
    ASSERT_FALSE(previewDpPath.empty());

    const std::string unrelatedDpPath =
        TestSupport::joinPath(this->m_runtimeRoot.path(), "data-products/Dp_268693505_00000001_00000001.fdp");
    this->invoke_to_dpWrittenIn(0, Fw::String(unrelatedDpPath.c_str()), 25U, 469U);
    this->invoke_to_schedIn(0, 0U);

    ASSERT_EQ(this->getStatus_().state, OBC::PayloadState::PSTATE_PUBLISHING);
    ASSERT_EQ(this->eventHistory_PAYLOAD_CAPTURED->size(), 0U);

    Fw::Buffer buffer = this->productSendHistory->at(0).buffer;
    this->invoke_to_dpWrittenIn(0, Fw::String(previewDpPath.c_str()), 25U, buffer.getSize());
    this->invoke_to_schedIn(0, 0U);

    const OBC::PayloadCaptureMetadata metadata = this->getLastMetadata_();
    ASSERT_TRUE(metadata.previewDataProductPublished);
    ASSERT_FALSE(metadata.rawDataProductPublished);
    ASSERT_EQ(metadata.lastPublishedArtifactKind, OBC::PayloadArtifactKind::PREVIEW_JPEG);
    ASSERT_EQ(this->eventHistory_PAYLOAD_CAPTURED->size(), 1U);
    ASSERT_EQ(this->getStatus_().state, OBC::PayloadState::PSTATE_READY);
}

void PayloadOpsControllerTester::testPublishCaptureRejectsSynchronousFirstSliceFailure() {
    this->clearHistory();
    this->sendCmd_PAYLOAD_SET_CAMERA_DEFAULTS(
        TEST_INSTANCE_ID, 0, OBC::PayloadResolutionPreset::PRESET_VGA_640X480, 90U, false, false);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_SET_CAMERA_DEFAULTS, 0, Fw::CmdResponse::OK);
    this->prepareNonRaw_();

    const U32 captureSendBaseline = this->productSendHistory->size();
    this->captureDeterministic_(1, 0x30U, "sync-fail", 0U, 10000U, 100U);
    this->waitForProductSendCount_(captureSendBaseline + 1U);
    this->completePendingPublishes_();

    this->clearHistory();
    this->m_failNextProductGet = true;
    this->sendCmd_PAYLOAD_PUBLISH_CAPTURE(TEST_INSTANCE_ID, 2, 0x30U, OBC::PayloadArtifactKind::RAW_FRAME);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_PUBLISH_CAPTURE, 2, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_FALSE(this->getStatus_().busy);
    ASSERT_EQ(this->getStatus_().state, OBC::PayloadState::PSTATE_READY);
    ASSERT_EQ(this->getStatus_().lastResult, OBC::PayloadResultCode::PRESULT_STORAGE_FAILED);

    this->clearHistory();
    this->sendCmd_PAYLOAD_GET_STATUS(TEST_INSTANCE_ID, 3);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_GET_STATUS, 3, Fw::CmdResponse::OK);
}

void PayloadOpsControllerTester::testPublishCaptureReadsLegacyManifestCompatibility() {
    this->clearHistory();

    const U8 captureIndex = 0x36U;
    const std::string rawRelativePath = "persistent-data/payload/camera/PIC36.bin";
    const std::string previewRelativePath = "persistent-data/payload/camera/PIC36.jpg";
    const std::string previewDpRelativePath = "data-products/Dp_legacy_preview.fdp";
    const std::string rawDpRelativePath = "data-products/Dp_legacy_raw.fdp";

    OBC::PayloadCaptureArtifactHeaderV2 header = {};
    header.set_version(2U);
    header.set_captureId(77U);
    header.set_captureIndex(captureIndex);
    header.set_artifactKind(OBC::PayloadArtifactKind::PREVIEW_JPEG);
    header.set_pixelFormat(OBC::PayloadPixelFormat::PIXEL_YUYV);
    header.set_reserved0(0U);
    header.set_bootCount(7U);
    header.set_captureTimeSec(1700000123U);
    header.set_captureTimeUsec(4567U);
    header.set_capturePolicy(OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC);
    header.set_resultCode(OBC::PayloadResultCode::PRESULT_OK);
    header.set_requestedMask(0U);
    header.set_appliedMask(0U);
    header.set_resolution(OBC::PayloadResolutionPreset::PRESET_HD_1280X720);
    header.set_jpegQualityApplied(90U);
    header.set_exposureUsec(12345U);
    header.set_gainX100(250U);
    header.set_actualExposureUsec(0U);
    header.set_actualGainX100(0U);
    header.set_actualAwbValid(false);
    header.set_actualAwbColorTemperatureK(0U);
    header.set_actualAwbRedGainX1000(0U);
    header.set_actualAwbBlueGainX1000(0U);
    header.set_imageWidth(1280U);
    header.set_imageHeight(720U);
    header.set_rawBytes(1280U * 720U * 2U);
    header.set_previewJpegBytes(static_cast<U32>(sizeof(STUB_JPEG_BYTES)));
    header.set_backendName(Fw::String("dual-artifact-test"));
    header.set_cameraModel(Fw::String("OV5647-dual-artifact-test"));
    header.set_rawRelativePath(Fw::String(rawRelativePath.c_str()));
    header.set_previewRelativePath(Fw::String(previewRelativePath.c_str()));
    header.set_previewDataProductRelativePath(Fw::String(previewDpRelativePath.c_str()));
    header.set_rawDataProductRelativePath(Fw::String(rawDpRelativePath.c_str()));
    header.set_previewDataProductPublished(false);
    header.set_rawDataProductPublished(false);

    std::array<U8, OBC::PayloadCaptureArtifactHeaderV2::SERIALIZED_SIZE> fullBytes = {};
    Fw::ExternalSerializeBuffer fullBuffer(fullBytes.data(), static_cast<FwSizeType>(fullBytes.size()));
    ASSERT_EQ(header.serializeTo(fullBuffer), Fw::SerializeStatus::FW_SERIALIZE_OK);

    std::array<U8, 128> prefixBytes = {};
    Fw::ExternalSerializeBuffer prefixBuffer(prefixBytes.data(), static_cast<FwSizeType>(prefixBytes.size()));
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U16>(2U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U32>(77U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(captureIndex), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(OBC::PayloadArtifactKind::PREVIEW_JPEG), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(OBC::PayloadPixelFormat::PIXEL_YUYV), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U8>(0U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U32>(7U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U32>(1700000123U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U32>(4567U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC),
              Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(OBC::PayloadResultCode::PRESULT_OK), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U32>(0U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U32>(0U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(OBC::PayloadResolutionPreset::PRESET_HD_1280X720), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U32>(90U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U32>(12345U)), Fw::SerializeStatus::FW_SERIALIZE_OK);
    ASSERT_EQ(prefixBuffer.serializeFrom(static_cast<U32>(250U)), Fw::SerializeStatus::FW_SERIALIZE_OK);

    constexpr FwSizeType legacySerializedSize =
        OBC::PayloadCaptureArtifactHeaderV2::SERIALIZED_SIZE - (sizeof(U32) * 5U + sizeof(bool));
    const FwSizeType legacyPrefixSize = prefixBuffer.getSize();
    const FwSizeType removedActualFieldsSize = sizeof(U32) * 5U + sizeof(bool);
    ASSERT_EQ(legacySerializedSize + removedActualFieldsSize, OBC::PayloadCaptureArtifactHeaderV2::SERIALIZED_SIZE);
    ASSERT_LT(legacyPrefixSize + removedActualFieldsSize, fullBuffer.getSize());

    std::array<U8, legacySerializedSize> bytes = {};
    std::copy(fullBytes.begin(), fullBytes.begin() + static_cast<std::ptrdiff_t>(legacyPrefixSize), bytes.begin());
    std::copy(fullBytes.begin() + static_cast<std::ptrdiff_t>(legacyPrefixSize + removedActualFieldsSize),
              fullBytes.begin() + static_cast<std::ptrdiff_t>(fullBuffer.getSize()),
              bytes.begin() + static_cast<std::ptrdiff_t>(legacyPrefixSize));
    ASSERT_TRUE(TestSupport::writeBinaryFile(this->manifestPathForIndex_(captureIndex),
                                             std::vector<U8>(bytes.begin(), bytes.end())));
    ASSERT_TRUE(TestSupport::writeBinaryFile(this->previewPathForIndex_(captureIndex),
                                             std::vector<U8>(std::begin(STUB_JPEG_BYTES), std::end(STUB_JPEG_BYTES))));

    const U32 sendBaseline = this->productSendHistory->size();
    this->sendCmd_PAYLOAD_PUBLISH_CAPTURE(TEST_INSTANCE_ID, 0, captureIndex, OBC::PayloadArtifactKind::PREVIEW_JPEG);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_PAYLOAD_PUBLISH_CAPTURE, 0, Fw::CmdResponse::OK);
    this->waitForProductSendCount_(sendBaseline + 1U);
    this->completePendingPublishes_();

    const OBC::PayloadCaptureMetadata metadata = this->getLastMetadata_();
    ASSERT_EQ(metadata.captureId, 77U);
    ASSERT_EQ(metadata.captureIndex, captureIndex);
    ASSERT_EQ(metadata.appliedSettings.resolution, OBC::PayloadResolutionPreset::PRESET_HD_1280X720);
    ASSERT_EQ(metadata.actualExposureUsec, 0U);
    ASSERT_EQ(metadata.actualGainX100, 0U);
    ASSERT_FALSE(metadata.actualAwbValid);
    ASSERT_TRUE(metadata.previewDataProductPublished);
    ASSERT_FALSE(metadata.rawDataProductPublished);
}

}  // namespace OBC
