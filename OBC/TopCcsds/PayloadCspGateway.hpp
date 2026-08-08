#ifndef OBC_TOPCCSDS_PAYLOADCSPGATEWAY_HPP
#define OBC_TOPCCSDS_PAYLOADCSPGATEWAY_HPP

#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"
#include "OBC/TopCcsds/PayloadCspProtocol.hpp"

namespace OBCApp {

class PayloadCspGateway final {
  public:
    static void populateStatusReply(std::uint16_t seq,
                                    const OBC::PayloadStatusSnapshot& status,
                                    PayloadCSP::StatusReply& reply);

    static void populateCapabilitiesReply(std::uint16_t seq,
                                          const OBC::PayloadCapabilities& capabilities,
                                          PayloadCSP::CapabilitiesReply& reply);

    static void populateMetadataReply(std::uint16_t seq,
                                      const OBC::PayloadCaptureMetadata& metadata,
                                      PayloadCSP::MetadataReply& reply);
};

}  // namespace OBCApp

#endif
