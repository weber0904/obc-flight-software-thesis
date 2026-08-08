## ADDED Requirements

### Requirement: Ground TT&C Gateway Capability Exists
The repository SHALL define a dedicated `ground-ttc-gateway` capability for the omitted-RF ground test chain, and that capability SHALL remain distinct from both the stock direct `fprime-gds -> TCP -> OBC` development path and the spacecraft-side `comm` subsystem.

#### Scenario: Gateway capability does not replace the direct GDS baseline
- **WHEN** the project adds the first omitted-RF TT&C architecture
- **THEN** it SHALL keep the existing direct `GDS -> TCP -> OBC` path available as a separate governed development baseline

### Requirement: First Gateway Architecture Is Bidirectional
The first governed omitted-RF TT&C gateway architecture SHALL support both uplink and downlink traffic between ground-side tooling and the spacecraft-side comm path.

#### Scenario: Gateway supports bounded uplink and downlink
- **WHEN** the first omitted-RF TT&C path is validated
- **THEN** the repository SHALL capture evidence for a bounded uplink path and a bounded downlink path instead of validating only one direction

### Requirement: First Gateway Uses Lab-Side Serial Ingress
The first omitted-RF TT&C gateway SHALL be allowed to use a lab-side serial ingress that represents the RF-omitted boundary, and that ingress SHALL remain explicitly described as a lab transport rather than as a claim about flight RF behavior.

#### Scenario: Lab ingress does not over-claim RF equivalence
- **WHEN** the first gateway-backed omitted-RF TT&C path uses `macOS` to `subsystem.local` serial wiring
- **THEN** the repository SHALL describe that wiring as a lab ingress path and SHALL NOT describe it as RF validation

### Requirement: Gateway-First Integration Precedes Custom GDS Plugin Work
The repository SHALL allow the first omitted-RF TT&C implementation to use a governed gateway process or adapter layer before any custom `fprime-gds` communication plugin becomes required.

#### Scenario: First TT&C slice avoids immediate GDS plugin coupling
- **WHEN** the project implements the first omitted-RF TT&C path
- **THEN** it MAY use a repository-owned ground gateway without first shipping a custom GDS communication plugin
