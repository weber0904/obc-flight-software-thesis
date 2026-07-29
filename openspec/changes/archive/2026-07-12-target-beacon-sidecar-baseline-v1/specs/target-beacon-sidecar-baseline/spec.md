## ADDED Requirements

### Requirement: A SHALL Own The Target Beacon Sidecar Baseline

The A-layer target baseline manager SHALL own the remote UHF Beacon PTY bridge,
capture helper, required OBC/UHF service environment, and reviewable sidecar
metadata.

#### Scenario: A prepares a reusable sidecar
- **WHEN** A establishes a ready target COMM baseline
- **THEN** it SHALL verify or create one uniquely labelled remote Beacon bridge
  and capture helper
- **AND** it SHALL record source kind, source band, remote capture path, frame
  size, process identity, service identity, and readiness in its JSON result
- **AND** it SHALL apply a required service restart only as A-owned baseline
  setup or repair.

#### Scenario: A repairs only its own sidecar helpers
- **WHEN** A finds the recorded Beacon sidecar unhealthy
- **THEN** it SHALL reap or recreate only processes identified by its own
  instance label or recorded PID
- **AND** it SHALL NOT use a generic binary-path cleanup that can terminate an
  unrelated bridge or probe.

### Requirement: Manual Target Surfaces SHALL Consume But Not Own The Sidecar

The target manual ground surface SHALL consume A-published Beacon sidecar
metadata and may mirror its capture artifact locally, but SHALL NOT change
shared target service or remote sidecar lifecycle.

#### Scenario: Manual surface reads ready baseline metadata
- **WHEN** the current target baseline manifest advertises a ready Beacon
  sidecar
- **THEN** the manual surface SHALL publish target remote-sidecar capability
  metadata and mirror the advertised capture into its local surface root
- **AND** it SHALL not restart, override, or stop OBC/UHF/shared target
  services.

#### Scenario: Sidecar metadata is absent or unhealthy
- **WHEN** a target manual surface starts without ready A-published sidecar
  metadata
- **THEN** it SHALL safely omit Beacon capability
- **AND** Mission Console SHALL return its explicit unsupported Beacon state
  without attempting target repair.
