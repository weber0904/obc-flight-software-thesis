#ifndef OBC_DpCatalogFileDownlinkGateTester_HPP
#define OBC_DpCatalogFileDownlinkGateTester_HPP

#include <vector>

#include "OBC/Components/DpCatalogFileDownlinkGate/DpCatalogFileDownlinkGate.hpp"
#include "OBC/Components/DpCatalogFileDownlinkGate/DpCatalogFileDownlinkGateGTestBase.hpp"

namespace OBC {

class DpCatalogFileDownlinkGateTester final : public DpCatalogFileDownlinkGateGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    DpCatalogFileDownlinkGateTester();

    ~DpCatalogFileDownlinkGateTester() override;

    void testMatchingCompletionForwards();

    void testForeignCompletionIgnored();

    void testBusyWhilePending();

    void testSendFailureDoesNotLatchPending();

  private:
    struct SendRequest {
        Fw::String source;
        Fw::String dest;
        U32 offset;
        U32 length;
    };

    void connectPorts();

    void initComponents();

    Svc::SendFileResponse requestFile_();

    Svc::SendFileResponse from_sendFileOut_handler(FwIndexType portNum,
                                                   const Fw::StringBase& sourceFileName,
                                                   const Fw::StringBase& destFileName,
                                                   U32 offset,
                                                   U32 length) override;

    void from_fileCompleteOut_handler(FwIndexType portNum, const Svc::SendFileResponse& resp) override;

  private:
    OBC::DpCatalogFileDownlinkGate component;
    Svc::SendFileResponse m_nextSendResponse;
    std::vector<SendRequest> m_sendRequests;
    std::vector<Svc::SendFileResponse> m_completions;
};

}  // namespace OBC

#endif
