## MODIFIED Requirements

### Requirement: Ground TT&C Gateway Capability Exists
The repository SHALL define a dedicated `ground-ttc-gateway` capability for the omitted-RF ground test chain, and that capability SHALL remain distinct from both the stock direct `fprime-gds -> TCP -> OBC` development path and the spacecraft-side `comm` subsystem.

#### Scenario: Gateway capability does not replace the direct GDS baseline
- **WHEN** the project adds the first omitted-RF TT&C architecture
- **THEN** it SHALL keep the existing direct `GDS -> TCP -> OBC` path available as a separate governed development baseline

#### Scenario: First gateway stays GDS-facing instead of bypassing GDS
- **WHEN** the project implements the first gateway-backed omitted-RF path
- **THEN** the repository SHALL keep stock `fprime-gds` as the ground-facing operator surface
- **AND** the governed gateway SHALL act as a repository-owned adapter between that GDS-side TCP path and the lab-side comm ingress

### Requirement: First Gateway Architecture Is Bidirectional
The first governed omitted-RF TT&C gateway architecture SHALL support both uplink and downlink traffic between ground-side tooling and the spacecraft-side comm path, and the first formal proof SHALL be bounded to `command`, `event`, and `telemetry` traffic.

#### Scenario: Gateway supports bounded uplink and downlink
- **WHEN** the first omitted-RF TT&C path is validated
- **THEN** the repository SHALL capture evidence for a bounded uplink path and a bounded downlink path instead of validating only one direction

#### Scenario: First gateway proof stays out of file/downlink scope
- **WHEN** the first gateway-backed omitted-RF path is recorded as formal evidence
- **THEN** that evidence SHALL identify the validated scope as bounded `command`, `event`, and `telemetry`
- **AND** it SHALL keep file/downlink behavior explicit as future scope

### Requirement: Gateway-First Integration Precedes Custom GDS Plugin Work
The repository SHALL allow the first omitted-RF TT&C implementation to use a governed gateway process or adapter layer before any custom `fprime-gds` communication plugin becomes required.

#### Scenario: First TT&C slice avoids immediate GDS plugin coupling
- **WHEN** the project implements the first omitted-RF TT&C path
- **THEN** it MAY use a repository-owned ground gateway without first shipping a custom `fprime-gds` communication plugin

#### Scenario: First gateway reuses stock F' framing
- **WHEN** the repository implements the first governed ground gateway
- **THEN** that gateway SHALL reuse stock F' framing across the northbound GDS connection instead of requiring a custom GDS communication plugin for the first slice
