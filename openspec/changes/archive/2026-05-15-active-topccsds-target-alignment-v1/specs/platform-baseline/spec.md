## MODIFIED Requirements

### Requirement: Raspberry Pi Installable Bundle
The governed `integ-rpi` profile SHALL provide a repo-local packaging flow that
emits a reviewable installable bundle for the Raspberry Pi target, and that
bundle SHALL include the Linux OBC runtime, the companion simulator executables
used by the integrated target flow, the deployment dictionary, and bundle
metadata describing the packaged framework and project versions.

#### Scenario: Target bundle is created from governed active artifacts
- **WHEN** the Raspberry Pi packaging flow runs after a successful target build
- **THEN** the repository SHALL produce a reviewable bundle artifact containing
  the installed-stack payload and bundle metadata without requiring the target
  to keep the full source workspace as the deployable unit
- **AND** the governed `bin/OBC` payload SHALL come from the active
  `build-artifacts/.../OBC` deployment rather than silently repackaging
  `OBC_ComFprimeLegacy` under an active name
- **AND** the packaged dictionary SHALL match the active `OBCApp` topology.

### Requirement: Default Hosted OBC Uses CCSDS Topology
The platform baseline SHALL make the default hosted `OBC` deployment the CCSDS
S-band topology while preserving the shared hosted runtime helper and all
currently available hosted OBC capability surfaces except retired HK fallback
surfaces.

#### Scenario: Target-facing helper defaults follow the active OBC deployment
- **WHEN** a governed source-workspace Raspberry Pi helper launches the current
  target-facing `OBC` runtime without an explicit override
- **THEN** that helper SHALL default to the active `OBC` binary rather than
  silently defaulting to `OBC_ComFprimeLegacy`
- **AND** legacy `ComFprime` helpers MAY remain available only when they are
  named or selected explicitly as legacy paths.
