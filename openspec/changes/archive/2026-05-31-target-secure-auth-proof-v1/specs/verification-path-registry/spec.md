## ADDED Requirements

### Requirement: Registry Adds Target Secure Auth Proof Path Only After Fresh Target Evidence

The verification-path registry SHALL add a target secure-auth proof entry only
after fresh target evidence exercises the installed S-band and bounded UHF
paths.

#### Scenario: Registry names target S-band and UHF ancestry separately
- **WHEN** the target secure-auth path is registered
- **THEN** the entry SHALL cite hosted entries `43E/43F` as ancestry only
- **AND** it SHALL cite target entry `59` for S-band path reuse
- **AND** it SHALL cite target entry `69` for bounded UHF physical node-`6`
  path reuse.

#### Scenario: Registry records exact target secure-auth scope
- **WHEN** the target secure-auth path is registered
- **THEN** the entry SHALL state the S-band proof cases, the bounded UHF proof
  cases, the installed-release keystore provenance, and the retained gateway
  capture plus target-journal evidence surfaces.

#### Scenario: Registry keeps adjacent non-claims explicit
- **WHEN** reviewers inspect the target secure-auth entry
- **THEN** the entry SHALL keep encryption, RF, boot-trust expansion,
  hardware-backed key storage, generic file authority, UHF primary staged-file
  success, one-GDS aggregation, one-gateway multiplexing, and legacy v1
  retirement out of scope.
