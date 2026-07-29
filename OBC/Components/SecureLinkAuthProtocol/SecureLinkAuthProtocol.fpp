module OBC {

  constant SECURE_SERVICE_SBAND = 1
  constant SECURE_SERVICE_UHF = 2

  enum SecureAuthStatusCode : U32 {
    AUTHENTICATED = 1
    NOT_AUTHENTICATED = 2
    MALFORMED = 3
    UNSUPPORTED_SERVICE = 4
    INTERNAL_ERROR = 5
  }

  enum SecureAuthRevocationReason : U32 {
    UNKNOWN = 0
    TIMEOUT = 1
    INVALIDATED = 2
    REPLACED = 3
    RESET = 4
    INTERNAL_ERROR = 5
  }

  array SecureSessionKey = [32] U8

  struct SecureAuthGrant {
    ingressPort: U32
    serviceId: U32
    sessionKey: SecureSessionKey
  }

  struct SecureAuthRevocation {
    ingressPort: U32
    serviceId: U32
    reason: U32
  }

  struct SecureAuthActivity {
    ingressPort: U32
    serviceId: U32
    sequenceNumber: U32
  }

  port SecureAuthGrantPort(grantArg: SecureAuthGrant)
  port SecureAuthRevocationPort(revocationArg: SecureAuthRevocation)
  port SecureAuthActivityPort(activityArg: SecureAuthActivity)

}
