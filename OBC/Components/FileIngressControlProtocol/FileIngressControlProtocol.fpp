module OBC {

  struct FileIngressPolicyState {
    ingressPort: U32
    linkIdentity: U32
    linkRole: U32
    fileAllowed: bool
  }

  port FileIngressPolicyPort(policyArg: FileIngressPolicyState)

}
