## MODIFIED Requirements

### Requirement: Authenticated Command Envelope Evidence Is Reviewable

Verification evidence for the hosted secure-auth baseline SHALL now cover the
tracked keystore contract and the widened bounded uplink authority story in
addition to secure command proof.

#### Scenario: Hosted secure-auth evidence proves keystore-backed bootstrap
- **WHEN** `uplink-authority-and-key-hardening-v1` evidence is cited
- **THEN** it SHALL show that hosted S-band and UHF secure auth use the shared
  tracked keystore asset rather than runtime-injected command-auth keys
- **AND** it SHALL record the maintained helper or simulator commands used to
  read that asset.

#### Scenario: Unknown uplink reject evidence is reviewable
- **WHEN** the same evidence cites comm-managed unknown uplink closure
- **THEN** it SHALL include malformed or unsupported APID `0x00FE` rejection
  cases
- **AND** it SHALL show that those rejects do not create or mutate active auth
  state.

#### Scenario: Staged file-uplink authority evidence is reviewable
- **WHEN** the same evidence cites staged file-uplink authority
- **THEN** it SHALL include:
  - S-band secure auth plus `.sequence-staging/<leaf>` upload success
  - UHF backup secure auth plus staged upload denial
  - explicit UHF failover-primary re-auth plus staged upload success
  - mid-transfer revoke or timeout drop behavior
  - non-staging destination rejection

#### Scenario: Legacy compatibility remains separate and keystore-backed
- **WHEN** the same evidence cites retained comm-managed legacy v1
  compatibility
- **THEN** it SHALL state that legacy comm-managed auth also used the shared
  tracked keystore contract
- **AND** it SHALL keep that legacy coverage separate from the hosted secure
  auth and staged-file closure proof.
