#include "DpCatalogFileDownlinkGateTester.hpp"

namespace OBC {

DpCatalogFileDownlinkGateTester::DpCatalogFileDownlinkGateTester()
    : DpCatalogFileDownlinkGateGTestBase("DpCatalogFileDownlinkGateTester", MAX_HISTORY_SIZE),
      component("DpCatalogFileDownlinkGate"),
      m_nextSendResponse(Svc::SendFileStatus::STATUS_OK, 0U),
      m_sendRequests(),
      m_completions() {
    this->initComponents();
    this->connectPorts();
}

DpCatalogFileDownlinkGateTester::~DpCatalogFileDownlinkGateTester() = default;

void DpCatalogFileDownlinkGateTester::testMatchingCompletionForwards() {
    this->m_nextSendResponse = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 42U);
    this->clearHistory();

    const Svc::SendFileResponse response = this->requestFile_();
    ASSERT_EQ(response.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_EQ(response.get_context(), 42U);
    ASSERT_EQ(this->m_sendRequests.size(), 1U);

    this->invoke_to_fileCompleteIn(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 41U));
    ASSERT_EQ(this->m_completions.size(), 0U);

    this->invoke_to_fileCompleteIn(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 42U));
    ASSERT_EQ(this->m_completions.size(), 1U);
    ASSERT_EQ(this->m_completions[0].get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_EQ(this->m_completions[0].get_context(), 42U);
}

void DpCatalogFileDownlinkGateTester::testForeignCompletionIgnored() {
    this->clearHistory();

    this->invoke_to_fileCompleteIn(0, Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 7U));

    ASSERT_EQ(this->m_completions.size(), 0U);
    ASSERT_EQ(this->m_sendRequests.size(), 0U);
}

void DpCatalogFileDownlinkGateTester::testBusyWhilePending() {
    this->m_nextSendResponse = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 9U);
    this->clearHistory();

    const Svc::SendFileResponse first = this->requestFile_();
    const Svc::SendFileResponse second = this->requestFile_();

    ASSERT_EQ(first.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_EQ(second.get_status(), Svc::SendFileStatus::STATUS_BUSY);
    ASSERT_EQ(second.get_context(), 9U);
    ASSERT_EQ(this->m_sendRequests.size(), 1U);
    ASSERT_EQ(this->m_completions.size(), 0U);
}

void DpCatalogFileDownlinkGateTester::testSendFailureDoesNotLatchPending() {
    this->m_nextSendResponse = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_ERROR, 3U);
    this->clearHistory();

    const Svc::SendFileResponse failed = this->requestFile_();
    ASSERT_EQ(failed.get_status(), Svc::SendFileStatus::STATUS_ERROR);
    ASSERT_EQ(this->m_sendRequests.size(), 1U);

    this->m_nextSendResponse = Svc::SendFileResponse(Svc::SendFileStatus::STATUS_OK, 4U);
    const Svc::SendFileResponse retry = this->requestFile_();
    ASSERT_EQ(retry.get_status(), Svc::SendFileStatus::STATUS_OK);
    ASSERT_EQ(retry.get_context(), 4U);
    ASSERT_EQ(this->m_sendRequests.size(), 2U);
}

Svc::SendFileResponse DpCatalogFileDownlinkGateTester::requestFile_() {
    const Fw::String source("/runtime/data-products/Dp_1.fdp");
    const Fw::String dest("Dp_1.fdp");
    return this->invoke_to_sendFileIn(0, source, dest, 0U, 0U);
}

Svc::SendFileResponse DpCatalogFileDownlinkGateTester::from_sendFileOut_handler(
    FwIndexType portNum,
    const Fw::StringBase& sourceFileName,
    const Fw::StringBase& destFileName,
    U32 offset,
    U32 length) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_sendFileOut(sourceFileName, destFileName, offset, length);
    SendRequest request = {Fw::String(sourceFileName.toChar()), Fw::String(destFileName.toChar()), offset, length};
    this->m_sendRequests.push_back(request);
    return this->m_nextSendResponse;
}

void DpCatalogFileDownlinkGateTester::from_fileCompleteOut_handler(FwIndexType portNum,
                                                                   const Svc::SendFileResponse& resp) {
    static_cast<void>(portNum);
    this->pushFromPortEntry_fileCompleteOut(resp);
    this->m_completions.push_back(resp);
}

}  // namespace OBC
