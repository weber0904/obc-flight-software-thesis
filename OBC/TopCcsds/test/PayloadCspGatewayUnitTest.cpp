#include <gtest/gtest.h>

#include "OBC/TopCcsds/PayloadCspGateway.hpp"

namespace {

TEST(PayloadCspGateway, PopulatesStatusReplyFlagsAndFields) {
    OBC::PayloadStatusSnapshot status = {};
    status.runtimeConfigured = true;
    status.busy = true;
    status.logicalPowerEnabled = true;
    status.prepared = true;
    status.proxyAsserted = true;
    status.state = OBC::PayloadState::PSTATE_READY;
    status.preparedReadyKind = OBC::PayloadReadyKind::READY_RAW_SENSOR;
    status.lastResult = OBC::PayloadResultCode::PRESULT_OK;
    status.lastDetail = 17U;
    status.lastCaptureId = 3U;
    status.lastRequestedMask = 0x12U;
    status.lastAppliedMask = 0x10U;
    status.abortTotal = 2U;

    OBCApp::PayloadCSP::StatusReply reply = {};
    OBCApp::PayloadCspGateway::populateStatusReply(44U, status, reply);

    EXPECT_EQ(OBCApp::PayloadCSP::VERSION, reply.header.version);
    EXPECT_EQ(static_cast<std::uint8_t>(OBCApp::PayloadCSP::ServicePort::STATUS), reply.header.service);
    EXPECT_EQ(44U, reply.header.seq);
    EXPECT_EQ(static_cast<std::uint8_t>(OBCApp::PayloadCSP::ResultCode::OK), reply.header.result);
    EXPECT_TRUE((reply.header.flags & OBCApp::PayloadCSP::FLAG_BUSY) != 0U);
    EXPECT_TRUE((reply.header.flags & OBCApp::PayloadCSP::FLAG_LOGICAL_POWER_ENABLED) != 0U);
    EXPECT_TRUE((reply.header.flags & OBCApp::PayloadCSP::FLAG_PREPARED) != 0U);
    EXPECT_TRUE((reply.header.flags & OBCApp::PayloadCSP::FLAG_PROXY_ASSERTED) != 0U);
    EXPECT_EQ(static_cast<std::uint8_t>(OBC::PayloadState::PSTATE_READY), reply.state);
    EXPECT_EQ(static_cast<std::uint8_t>(OBC::PayloadReadyKind::READY_RAW_SENSOR), reply.preparedReadyKind);
    EXPECT_EQ(17U, reply.lastDetail);
    EXPECT_EQ(3U, reply.lastCaptureId);
    EXPECT_EQ(0x12U, reply.lastRequestedMask);
    EXPECT_EQ(0x10U, reply.lastAppliedMask);
    EXPECT_EQ(2U, reply.abortTotal);
}

TEST(PayloadCspGateway, PopulatesCapabilitiesReply) {
    OBC::PayloadCapabilities capabilities = {};
    capabilities.backendName = "stub";
    capabilities.cameraModel = "OV5647-stub";
    capabilities.supportedMask = 0x11U;
    capabilities.offOnlyMask = 0x01U;
    capabilities.autoMutableMask = 0x02U;
    capabilities.deterministicMutableMask = 0x10U;
    capabilities.rawRegisterSupported = true;
    capabilities.realSensorPath = false;

    OBCApp::PayloadCSP::CapabilitiesReply reply = {};
    OBCApp::PayloadCspGateway::populateCapabilitiesReply(9U, capabilities, reply);

    EXPECT_EQ(0x11U, reply.supportedMask);
    EXPECT_EQ(0x01U, reply.offOnlyMask);
    EXPECT_EQ(0x02U, reply.autoMutableMask);
    EXPECT_EQ(0x10U, reply.deterministicMutableMask);
    EXPECT_STREQ("stub", reply.backendName);
    EXPECT_STREQ("OV5647-stub", reply.cameraModel);
    EXPECT_TRUE((reply.header.flags & OBCApp::PayloadCSP::FLAG_RAW_REGISTER_SUPPORTED) != 0U);
    EXPECT_FALSE((reply.header.flags & OBCApp::PayloadCSP::FLAG_REAL_SENSOR_PATH) != 0U);
}

TEST(PayloadCspGateway, PopulatesMetadataReply) {
    OBC::PayloadCaptureMetadata metadata = {};
    metadata.capturePolicy = OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC;
    metadata.captureId = 7U;
    metadata.captureIndex = 0x2AU;
    metadata.bootCount = 3U;
    metadata.captureTimeSec = 1717482000U;
    metadata.captureTimeUsec = 123456U;
    metadata.requestedMask = 0x30U;
    metadata.appliedMask = 0x10U;
    metadata.pixelFormat = OBC::PayloadPixelFormat::PIXEL_YUYV;
    metadata.rawRelativePath = "persistent-data/payload/camera/PIC2A.bin";
    metadata.previewRelativePath = "persistent-data/payload/camera/PIC2A.jpg";
    metadata.previewDataProductRelativePath = "data-products/Dp_00000001_00000003_00000007.fdp";
    metadata.rawDataProductRelativePath = "data-products/Dp_00000001_00000003_00000008.fdp";
    metadata.appliedSettings.resolution = OBC::PayloadResolutionPreset::PRESET_HD_1280X720;
    metadata.appliedSettings.jpegQuality = 85U;
    metadata.appliedSettings.exposureUsec = 12000U;
    metadata.appliedSettings.gainX100 = 200U;
    metadata.imageWidth = 1280U;
    metadata.imageHeight = 720U;
    metadata.rawBytes = 1843200U;
    metadata.previewJpegBytes = 103038U;
    metadata.previewDataProductBytes = 104512U;
    metadata.rawDataProductBytes = 1847296U;
    metadata.previewDataProductPublished = true;
    metadata.rawDataProductPublished = true;
    metadata.resultCode = OBC::PayloadResultCode::PRESULT_OK;

    OBCApp::PayloadCSP::MetadataReply reply = {};
    OBCApp::PayloadCspGateway::populateMetadataReply(10U, metadata, reply);

    EXPECT_EQ(7U, reply.captureId);
    EXPECT_EQ(0x2AU, reply.captureIndex);
    EXPECT_EQ(static_cast<std::uint8_t>(OBC::PayloadCapturePolicy::CAPTURE_DETERMINISTIC), reply.capturePolicy);
    EXPECT_EQ(3U, reply.bootCount);
    EXPECT_EQ(1717482000U, reply.captureTimeSec);
    EXPECT_EQ(123456U, reply.captureTimeUsec);
    EXPECT_EQ(12000U, reply.exposureUsec);
    EXPECT_EQ(200U, reply.gainX100);
    EXPECT_EQ(1280U, reply.imageWidth);
    EXPECT_EQ(720U, reply.imageHeight);
    EXPECT_EQ(static_cast<std::uint8_t>(OBC::PayloadPixelFormat::PIXEL_YUYV), reply.pixelFormat);
    EXPECT_EQ(1843200U, reply.rawBytes);
    EXPECT_EQ(103038U, reply.previewJpegBytes);
    EXPECT_EQ(104512U, reply.previewDataProductBytes);
    EXPECT_EQ(1847296U, reply.rawDataProductBytes);
    EXPECT_EQ(1U, reply.previewDataProductPublished);
    EXPECT_EQ(1U, reply.rawDataProductPublished);
    EXPECT_STREQ("persistent-data/payload/camera/PIC2A.bin", reply.rawRelativePath);
    EXPECT_STREQ("persistent-data/payload/camera/PIC2A.jpg", reply.previewRelativePath);
    EXPECT_STREQ("data-products/Dp_00000001_00000003_00000007.fdp", reply.previewDataProductPath);
    EXPECT_STREQ("data-products/Dp_00000001_00000003_00000008.fdp", reply.rawDataProductPath);
}

TEST(PayloadCspGateway, PreservesLongMetadataPaths) {
    OBC::PayloadCaptureMetadata metadata = {};
    metadata.rawRelativePath = std::string(OBCApp::PayloadCSP::RELATIVE_PATH_LENGTH - 1U, 'r');
    metadata.previewRelativePath = std::string(OBCApp::PayloadCSP::RELATIVE_PATH_LENGTH - 1U, 'p');
    metadata.previewDataProductRelativePath = std::string(OBCApp::PayloadCSP::RELATIVE_PATH_LENGTH - 1U, 'd');
    metadata.rawDataProductRelativePath = std::string(OBCApp::PayloadCSP::RELATIVE_PATH_LENGTH - 1U, 'w');

    ASSERT_EQ(OBCApp::PayloadCSP::RELATIVE_PATH_LENGTH - 1U, metadata.rawRelativePath.size());
    ASSERT_EQ(OBCApp::PayloadCSP::RELATIVE_PATH_LENGTH - 1U, metadata.previewRelativePath.size());
    ASSERT_EQ(OBCApp::PayloadCSP::RELATIVE_PATH_LENGTH - 1U, metadata.previewDataProductRelativePath.size());
    ASSERT_EQ(OBCApp::PayloadCSP::RELATIVE_PATH_LENGTH - 1U, metadata.rawDataProductRelativePath.size());

    OBCApp::PayloadCSP::MetadataReply reply = {};
    OBCApp::PayloadCspGateway::populateMetadataReply(11U, metadata, reply);

    EXPECT_STREQ(metadata.rawRelativePath.c_str(), reply.rawRelativePath);
    EXPECT_STREQ(metadata.previewRelativePath.c_str(), reply.previewRelativePath);
    EXPECT_STREQ(metadata.previewDataProductRelativePath.c_str(), reply.previewDataProductPath);
    EXPECT_STREQ(metadata.rawDataProductRelativePath.c_str(), reply.rawDataProductPath);
}

}  // namespace
