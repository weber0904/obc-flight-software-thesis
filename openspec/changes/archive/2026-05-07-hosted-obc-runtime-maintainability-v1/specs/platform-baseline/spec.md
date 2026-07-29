## ADDED Requirements

### Requirement: Hosted Runtime Dispatch Is Shared Across Protocol Variants
The platform baseline SHALL keep common hosted runtime command parsing, command dispatch, runtime state formatting, and launch argument handling in shared helper logic when multiple hosted deployment variants expose the same operator shell surface.

#### Scenario: Default and CCSDS hosted runtimes share command dispatch
- **WHEN** the default hosted `OBC` runtime and `OBC_CcsdsGroundLinkSpike` expose the same operator commands
- **THEN** their common command dispatch SHALL be implemented through shared runtime helper logic rather than maintained as independent duplicated command loops
- **AND** deployment-specific topology access SHALL remain adapter-owned for `OBCApp` and `OBCAppCcsds`

#### Scenario: Protocol variants preserve topology boundaries
- **WHEN** the shared hosted runtime helper is used by multiple deployment variants
- **THEN** the default `OBC` runtime SHALL remain on its existing `ComFprime` topology
- **AND** `OBC_CcsdsGroundLinkSpike` SHALL remain a separate spike-only `ComCcsds` executable
- **AND** shared helper reuse SHALL NOT imply that direct GDS, S-band-through-COMM, UHF serial backup, or CCSDS spike evidence boundaries are interchangeable

### Requirement: Hosted Runtime Refactors Preserve Operator Surface
Hosted runtime maintainability refactors SHALL preserve existing operator command names, accepted valid arguments, visible probe markers, and public F' command, event, telemetry, and file/downlink behavior unless a formal change explicitly scopes a behavior change.

#### Scenario: Existing operator commands remain stable
- **WHEN** a hosted runtime command-dispatch refactor is implemented
- **THEN** existing operator commands such as `status`, `mode`, `csp ping`, `eps`, `adcs`, `gps`, `storage`, `comm`, `radio`, `uart`, and `boot` SHALL remain available with their existing valid argument forms
- **AND** existing startup and status output markers used by repository-owned probes SHALL remain stable

#### Scenario: Refactor does not change public component contracts
- **WHEN** hosted runtime dispatch logic is refactored
- **THEN** the change SHALL NOT modify public F' component command opcodes, telemetry channels, event definitions, housekeeping archive file fields, COMM CSP node IDs, COMM service ports, or COMM request/reply wire layouts
