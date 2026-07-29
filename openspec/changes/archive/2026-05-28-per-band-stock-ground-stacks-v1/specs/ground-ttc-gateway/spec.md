## MODIFIED Requirements

### Requirement: Gateway-First Integration Precedes Custom GDS Plugin Work

The repository SHALL continue to allow the active omitted-RF TT&C baseline to
use stock `fprime-gds` plus repo-owned gateway processes before any custom
multi-band GDS communication plugin becomes required.

#### Scenario: First TT&C slice avoids immediate GDS plugin coupling
- **WHEN** the project implements the first omitted-RF TT&C path
- **THEN** it MAY use a repository-owned ground gateway without first shipping
  a custom `fprime-gds` communication plugin

#### Scenario: First gateway reuses stock F' framing
- **WHEN** the repository implements the first governed ground gateway
- **THEN** that gateway SHALL reuse stock F' framing across the northbound GDS
  connection instead of requiring a custom GDS communication plugin for the
  first slice

#### Scenario: Near-term simultaneous multi-band operations use maintained separate stock stacks
- **WHEN** the current baseline needs simultaneous operator access to S-band
  and UHF before a custom orchestrated ground surface exists
- **THEN** the maintained hosted-first baseline SHALL use separate stock
  `fprime-gds` plus `ground_ttc_gateway` stacks per band
- **AND** the repository SHALL treat that as a smaller baseline step than a new
  custom GDS communication plugin or a new orchestration owner in the same
  change

#### Scenario: One gateway instance remains one-southbound
- **WHEN** reviewers inspect the current `ground_ttc_gateway` boundary
- **THEN** they SHALL see it described as one northbound GDS relay bound to one
  current southbound path
- **AND** they SHALL NOT treat one gateway instance as the current simultaneous
  S-band/UHF multiplexer baseline

## ADDED Requirements

### Requirement: Hosted Per-Band Stock Ground Stacks Are Maintained

The ground TT&C gateway capability SHALL provide maintained hosted-first
launcher surfaces for the current near-term simultaneous multi-band operator
baseline using one shared hosted runtime plus distinct stock S-band and UHF
ground stacks.

#### Scenario: Per-band launchers expose distinct operator surfaces
- **WHEN** an operator starts the maintained hosted S-band or UHF stock stack
- **THEN** the launcher SHALL report the stack-specific GDS port, TTS port,
  southbound endpoint, file-storage directory, and process-log locations
- **AND** it SHALL keep the S-band stock surface distinct from the UHF stock
  surface rather than presenting one stock GDS or one gateway process as the
  simultaneous baseline

#### Scenario: Combined wrapper remains composition-only
- **WHEN** an operator starts the maintained combined hosted wrapper
- **THEN** it SHALL compose one shared hosted runtime with the maintained
  S-band and UHF stock stacks
- **AND** it SHALL report startup order and coordinated shutdown or cleanup for
  the owned processes
- **AND** it SHALL NOT claim that the wrapper is a new runtime policy owner,
  multiplexer, or higher-level orchestration surface

### Requirement: Hosted Per-Band Operator Runbook Is Reviewable

The ground TT&C gateway capability SHALL include a dedicated hosted operator
runbook for the maintained per-band stock-stack baseline.

#### Scenario: Runbook records current operator truth
- **WHEN** an operator or reviewer follows the hosted per-band runbook
- **THEN** it SHALL identify which stack is the nominal S-band high-authority
  path, which stack exposes the bounded UHF node-`6` surface, the shared hosted
  runtime root, per-stack logs and artifacts, startup order, and shutdown or
  cleanup steps

#### Scenario: Runbook preserves current non-claims
- **WHEN** reviewers inspect the runbook boundary
- **THEN** it SHALL state that the maintained baseline uses two stock GDS
  processes and two gateway processes with separate southbound paths
- **AND** it SHALL keep explicit non-claims for one-GDS heterogeneous upstream
  handling, one-gateway multiplexer behavior, target-bearing simultaneous
  dual-link closure, and future orchestration completion
