#include "simulators/comm/CommSimModel.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>

namespace {

OBC::COMM::CSP::ResultCode resultCode(const OBC::COMM::CSP::ReplyHeader& header) {
    return static_cast<OBC::COMM::CSP::ResultCode>(header.result);
}

OBC::COMM::CSP::DownlinkWriteRequest makeDownlinkRequest(std::uint16_t seq, const char* payload) {
    OBC::COMM::CSP::DownlinkWriteRequest request = OBC::COMM::CSP::makeDownlinkWriteRequest(seq);
    request.byteCount = static_cast<std::uint16_t>(std::strlen(payload));
    std::memcpy(request.data, payload, request.byteCount);
    return request;
}

void testDefaultResetAndLinkFlags() {
    OBC::COMM::CommSimModel model;
    OBC::COMM::CommSimStatus status = model.status();
    assert(status.uplinkQueueCapacity == 4096U);
    assert(status.uplinkQueueDepth == 0U);
    assert(!status.effectiveLinkConnected);
    assert(model.linkFlags() == 0U);

    model.setPhysicalLinkConnected(true);
    assert(model.status().effectiveLinkConnected);
    assert(model.linkFlags() == OBC::COMM::CSP::FLAG_LINK_CONNECTED);

    model.setForcedDisconnected(true);
    status = model.status();
    assert(status.physicalLinkConnected);
    assert(status.forcedDisconnected);
    assert(!status.effectiveLinkConnected);
    assert(model.linkFlags() == 0U);

    model.reset();
    status = model.status();
    assert(!status.physicalLinkConnected);
    assert(!status.forcedDisconnected);
    assert(status.rxChunks == 0U);
    assert(status.txChunks == 0U);
}

void testUplinkPollOrderingAndNoChunk() {
    OBC::COMM::CommSimModel model;
    model.setPhysicalLinkConnected(true);

    const std::uint8_t ingress[] = {'A', 'B', 'C'};
    assert(model.ingestUplinkBytes(ingress, sizeof(ingress)));
    assert(model.status().rxChunks == 1U);
    assert(model.status().uplinkQueueDepth == 3U);

    OBC::COMM::CSP::UplinkPollRequest poll = OBC::COMM::CSP::makeUplinkPollRequest(1U, 2U);
    OBC::COMM::CSP::ChunkReply reply = model.handleUplinkPoll(poll);
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::OK);
    assert(reply.header.byteCount == 2U);
    assert(reply.data[0] == 'A');
    assert(reply.data[1] == 'B');
    assert(model.status().uplinkQueueDepth == 1U);

    poll = OBC::COMM::CSP::makeUplinkPollRequest(2U);
    reply = model.handleUplinkPoll(poll);
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::OK);
    assert(reply.header.byteCount == 1U);
    assert(reply.data[0] == 'C');

    poll = OBC::COMM::CSP::makeUplinkPollRequest(3U);
    reply = model.handleUplinkPoll(poll);
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::NO_CHUNK);
    assert(reply.header.byteCount == 0U);
}

void testUplinkOverflowDropsNewChunk() {
    OBC::COMM::CommSimConfig config = {};
    config.uplinkQueueCapacity = 4U;
    OBC::COMM::CommSimModel model(config);
    model.setPhysicalLinkConnected(true);

    const std::uint8_t first[] = {'A', 'B', 'C', 'D'};
    const std::uint8_t second[] = {'E', 'F'};
    assert(model.ingestUplinkBytes(first, sizeof(first)));
    assert(!model.ingestUplinkBytes(second, sizeof(second)));

    OBC::COMM::CommSimStatus status = model.status();
    assert(status.uplinkQueueDepth == 4U);
    assert(status.uplinkDroppedChunks == 1U);
    assert(status.uplinkDroppedBytes == 2U);
    assert(status.rxChunks == 1U);
    assert(status.rxErrors == 1U);

    OBC::COMM::CSP::LinkStatusReply statusReply =
        model.handleLinkStatus(OBC::COMM::CSP::makeLinkStatusRequest(9U));
    assert(statusReply.rxErrors == 1U);

    OBC::COMM::CSP::UplinkPollRequest poll = OBC::COMM::CSP::makeUplinkPollRequest(4U, 4U);
    OBC::COMM::CSP::ChunkReply reply = model.handleUplinkPoll(poll);
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::OK);
    assert(reply.header.byteCount == 4U);
    assert(reply.data[0] == 'A');
    assert(reply.data[1] == 'B');
    assert(reply.data[2] == 'C');
    assert(reply.data[3] == 'D');
}

void testInvalidRequests() {
    OBC::COMM::CommSimModel model;
    model.setPhysicalLinkConnected(true);

    OBC::COMM::CSP::UplinkPollRequest poll = OBC::COMM::CSP::makeUplinkPollRequest(1U);
    poll.header.version = 99U;
    OBC::COMM::CSP::ChunkReply chunkReply = model.handleUplinkPoll(poll);
    assert(resultCode(chunkReply.header) == OBC::COMM::CSP::ResultCode::INVALID_REQUEST);

    OBC::COMM::CSP::DownlinkWriteRequest downlink = OBC::COMM::CSP::makeDownlinkWriteRequest(2U);
    bool shouldWrite = true;
    chunkReply = model.validateDownlinkWriteRequest(downlink);
    assert(resultCode(chunkReply.header) == OBC::COMM::CSP::ResultCode::INVALID_REQUEST);
    assert(model.status().txErrors == 0U);

    chunkReply = model.beginDownlinkWrite(downlink, shouldWrite);
    assert(!shouldWrite);
    assert(resultCode(chunkReply.header) == OBC::COMM::CSP::ResultCode::INVALID_REQUEST);
    assert(model.status().txErrors == 0U);

    OBC::COMM::CSP::LinkStatusRequest linkStatus = OBC::COMM::CSP::makeLinkStatusRequest(3U);
    linkStatus.header.service = 99U;
    OBC::COMM::CSP::LinkStatusReply statusReply = model.handleLinkStatus(linkStatus);
    assert(resultCode(statusReply.header) == OBC::COMM::CSP::ResultCode::INVALID_REQUEST);
}

void testDownlinkAcceptanceBackpressureAndErrors() {
    OBC::COMM::CommSimModel model;
    model.setPhysicalLinkConnected(true);

    OBC::COMM::CSP::DownlinkWriteRequest downlink = makeDownlinkRequest(1U, "abc");
    bool shouldWrite = false;
    OBC::COMM::CSP::ChunkReply reply = model.beginDownlinkWrite(downlink, shouldWrite);
    assert(shouldWrite);
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::OK);
    assert(reply.header.flags == OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    model.completeDownlinkWrite(true);
    assert(model.status().txChunks == 1U);

    model.setDownlinkBackpressure(true);
    assert(!model.shouldAttemptSerialDownlink());
    reply = model.beginDownlinkWrite(makeDownlinkRequest(2U, "def"), shouldWrite);
    assert(!shouldWrite);
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::IO_ERROR);
    assert(model.status().txErrors == 1U);
    model.setDownlinkBackpressure(false);

    model.setForcedDisconnected(true);
    assert(!model.shouldAttemptSerialDownlink());
    reply = model.beginDownlinkWrite(makeDownlinkRequest(3U, "ghi"), shouldWrite);
    assert(!shouldWrite);
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::IO_ERROR);
    assert(reply.header.flags == 0U);
    assert(model.status().txErrors == 2U);
    model.setForcedDisconnected(false);

    model.setInjectedIoError(true);
    assert(!model.shouldAttemptSerialDownlink());
    reply = model.beginDownlinkWrite(makeDownlinkRequest(4U, "jkl"), shouldWrite);
    assert(!shouldWrite);
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::IO_ERROR);
    assert(reply.header.flags == OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(model.status().txErrors == 3U);
    model.setInjectedIoError(false);

    reply = model.beginDownlinkWrite(makeDownlinkRequest(5U, "mno"), shouldWrite);
    assert(shouldWrite);
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::OK);
    assert(model.shouldAttemptSerialDownlink());
    model.completeDownlinkWrite(false);
    assert(model.status().txErrors == 4U);
}

void testLinkStatusCounters() {
    OBC::COMM::CommSimModel model;
    model.setPhysicalLinkConnected(true);
    const std::uint8_t ingress[] = {'A'};
    assert(model.ingestUplinkBytes(ingress, sizeof(ingress)));
    model.recordRxError();
    model.completeDownlinkWrite(true);
    model.completeDownlinkWrite(false);

    OBC::COMM::CSP::LinkStatusReply reply =
        model.handleLinkStatus(OBC::COMM::CSP::makeLinkStatusRequest(10U));
    assert(resultCode(reply.header) == OBC::COMM::CSP::ResultCode::OK);
    assert(reply.header.flags == OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(reply.rxChunks == 1U);
    assert(reply.txChunks == 1U);
    assert(reply.rxErrors == 1U);
    assert(reply.txErrors == 1U);
}

}  // namespace

int main() {
    testDefaultResetAndLinkFlags();
    testUplinkPollOrderingAndNoChunk();
    testUplinkOverflowDropsNewChunk();
    testInvalidRequests();
    testDownlinkAcceptanceBackpressureAndErrors();
    testLinkStatusCounters();
    return 0;
}
