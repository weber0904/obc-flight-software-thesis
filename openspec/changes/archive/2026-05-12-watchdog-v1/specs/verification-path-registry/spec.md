## ADDED Requirements

### Requirement: Registry Tracks Hosted Watchdog-v1 Path

The verification-path registry SHALL add a distinct hosted watchdog-v1 path that identifies active-baseline software-watchdog supervision without collapsing it into EPS timeout FDIR or target watchdog reset proof.

#### Scenario: Hosted watchdog path is registered

- **WHEN** hosted evidence proves bounded watchdog supervision over the active `TopCcsds` baseline
- **THEN** the registry SHALL describe the supervised source set, heartbeat freshness boundary, escalation behavior, and explicit scope limits that remain outside the proof

#### Scenario: Feed-suppression proof stays separate from target reset proof

- **WHEN** hosted evidence proves supervisor-side watchdog feed suppression
- **THEN** the registry SHALL identify that proof as feed-eligibility or feed-suppression behavior only
- **AND** it SHALL keep Raspberry Pi hardware watchdog reset, boot recovery, and reset-cause persistence as separate unproven or future paths unless later evidence proves them
