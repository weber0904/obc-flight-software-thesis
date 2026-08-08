#ifndef OBC_Components_DpCatalogFileDownlinkGate_HPP
#define OBC_Components_DpCatalogFileDownlinkGate_HPP

#include "OBC/Components/DpCatalogFileDownlinkGate/DpCatalogFileDownlinkGateComponentAc.hpp"

namespace OBC {

class DpCatalogFileDownlinkGate final : public DpCatalogFileDownlinkGateComponentBase {
  public:
    explicit DpCatalogFileDownlinkGate(const char* const compName);

    ~DpCatalogFileDownlinkGate() override;

  private:
    Svc::SendFileResponse sendFileIn_handler(FwIndexType portNum,
                                             const Fw::StringBase& sourceFileName,
                                             const Fw::StringBase& destFileName,
                                             U32 offset,
                                             U32 length) override;

    void fileCompleteIn_handler(FwIndexType portNum, const Svc::SendFileResponse& resp) override;

    void clearPending_();

  private:
    static constexpr U32 NO_CONTEXT = 0xFFFFFFFFU;

    bool m_havePending;
    U32 m_pendingContext;
};

}  // namespace OBC

#endif
