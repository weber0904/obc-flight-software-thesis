#ifndef OBC_COMPONENTS_UHFCCSDSVCIDADAPTER_HPP
#define OBC_COMPONENTS_UHFCCSDSVCIDADAPTER_HPP

#include "OBC/Components/UhfCcsdsVcidAdapter/UhfCcsdsVcidAdapterComponentAc.hpp"

namespace OBC {

class UhfCcsdsVcidAdapter final : public UhfCcsdsVcidAdapterComponentBase {
  public:
    explicit UhfCcsdsVcidAdapter(const char* compName);
    ~UhfCcsdsVcidAdapter() override;

  private:
    void dataIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) override;
    void dataReturnIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) override;
    void comStatusIn_handler(FwIndexType portNum, Fw::Success& condition) override;
};

}  // namespace OBC

#endif
