## ADDED Requirements

### Requirement: Three-Host Split-Host Internal CSP Evidence Is Reviewable
The verification evidence tree SHALL record the host roles, SSH targets, workspace roots, CSP ports, node ids, observed `csp ping`, and observed `eps get` / `adcs get` output for the governed three-host split-host internal CSP path.

#### Scenario: Three-host internal CSP proof names each host role
- **WHEN** the governed three-host split-host probe completes
- **THEN** the evidence SHALL identify `macOS` as the hub host, `obc.local` as the OBC host, `subsystem.local` as the simulator host, and SHALL keep that proof separate from adjacent GDS or external-comm claims

### Requirement: Three-Host GDS-Driven Subsystem Command Evidence Is Reviewable
The verification evidence tree SHALL record the GDS launch command, `fprime-cli` commands, observed dispatch, and `obc.local` state readback for the bounded `macOS fprime-cli -> GDS -> obc.local -> subsystem.local` subsystem command path.

#### Scenario: Three-host ground-driven subsystem commands are auditable
- **WHEN** the main three-host split-host probe completes
- **THEN** reviewers SHALL be able to inspect the bounded EPS and ADCS commands together with the Pi-side observed state changes that confirm those commands traversed the governed GDS and internal CSP layers

### Requirement: Three-Host Comm Coexistence Evidence Is Reviewable
The verification evidence tree SHALL record the serial-device settings, host-side mock-radio launch, subsystem-host launch, observed remote CSP reachability, observed radio command exchange, and final verdict for the three-host coexistence path.

#### Scenario: Comm coexistence proof stays scoped
- **WHEN** the three-host coexistence probe passes
- **THEN** the evidence SHALL identify the host serial device, `obc.local` comm device, remote subsystem host, and observed `radio enable on` / `uart raw STATUS` results while keeping RF behavior, transparent/framed UART paths, and future physical carriers out of scope
