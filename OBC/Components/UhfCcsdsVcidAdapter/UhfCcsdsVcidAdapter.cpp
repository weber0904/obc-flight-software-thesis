#include "OBC/Components/UhfCcsdsVcidAdapter/UhfCcsdsVcidAdapter.hpp"

namespace OBC {

namespace {
constexpr U8 UHF_CCSDS_VCID = 2U;
}

UhfCcsdsVcidAdapter::UhfCcsdsVcidAdapter(const char* compName) : UhfCcsdsVcidAdapterComponentBase(compName) {}

UhfCcsdsVcidAdapter::~UhfCcsdsVcidAdapter() = default;

void UhfCcsdsVcidAdapter::dataIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) {
    static_cast<void>(portNum);
    ComCfg::FrameContext stamped = context;
    stamped.set_vcId(UHF_CCSDS_VCID);
    this->dataOut_out(0, data, stamped);
}

void UhfCcsdsVcidAdapter::dataReturnIn_handler(FwIndexType portNum,
                                               Fw::Buffer& data,
                                               const ComCfg::FrameContext& context) {
    static_cast<void>(portNum);
    this->dataReturnOut_out(0, data, context);
}

void UhfCcsdsVcidAdapter::comStatusIn_handler(FwIndexType portNum, Fw::Success& condition) {
    static_cast<void>(portNum);
    this->comStatusOut_out(0, condition);
}

}  // namespace OBC
