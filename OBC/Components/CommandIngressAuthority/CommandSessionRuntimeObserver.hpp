#ifndef OBC_COMMANDSESSIONRUNTIMEOBSERVER_HPP
#define OBC_COMMANDSESSIONRUNTIMEOBSERVER_HPP

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"

namespace OBC {

class ICommandSessionRuntimeObserver {
  public:
    virtual ~ICommandSessionRuntimeObserver() = default;

    virtual void onCommandSessionOpenedForRuntime(FwIndexType ingressPort,
                                                  const AuthorityConfig& config,
                                                  U32 sessionId,
                                                  U32 sequenceNumber,
                                                  bool secureAuthenticated,
                                                  bool replaced) = 0;

    virtual void onCommandSessionActivityForRuntime(FwIndexType ingressPort,
                                                    const AuthorityConfig& config,
                                                    U32 sessionId,
                                                    U32 sequenceNumber) = 0;

    virtual void onCommandSessionRevokedForRuntime(FwIndexType ingressPort,
                                                   const AuthorityConfig& config,
                                                   U32 sessionId,
                                                   U32 lastAcceptedSequence,
                                                   U32 reason) = 0;
};

}  // namespace OBC

#endif
