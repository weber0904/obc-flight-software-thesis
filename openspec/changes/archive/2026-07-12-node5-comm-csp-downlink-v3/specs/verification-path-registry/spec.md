## ADDED Requirements

### Requirement: Node-5 Downlink V3 Records Hosted And Target Official Path Timing Separately
The verification path registry SHALL treat `node5-comm-csp-downlink-v3` as a
transport uplift below the existing official node-`5` `.fdp` path and SHALL
record both transport-local and end-to-end timing evidence.

#### Scenario: Hosted v3 transport proof records queue-backed acceptance
- **WHEN** hosted evidence is recorded for `node5-comm-csp-downlink-v3`
- **THEN** the focused hosted transport proof SHALL demonstrate byte-match on a
  direct `node1 backend -> node5 v3 -> external sink` path
- **AND** it SHALL record accepted-before-flushed queue separation
- **AND** it SHALL include at least one resend/no-progress observation

#### Scenario: Hosted official v3 proof records end-to-end timing
- **WHEN** the full hosted official `.fdp` path is rerun for
  `node5-comm-csp-downlink-v3`
- **THEN** the record SHALL distinguish
  `START_XMIT_CATALOG -> FileDownlink completion` from
  `START_XMIT_CATALOG -> final GDS arrival`
- **AND** it SHALL state whether hosted telemetry-queue overflow residuals
  remained, worsened, or disappeared

#### Scenario: Governed target v3 proof records comparison against the v2 branch-local reference
- **WHEN** governed target/lab evidence is recorded for
  `node5-comm-csp-downlink-v3`
- **THEN** the record SHALL compare the official payload `.fdp` completion time
  against the current `2026-07-07` node-`5` `v2` branch-local reference
- **AND** it SHALL state whether any restart, disconnect, or backpressure
  regressions were observed on the maintained target baseline
- **AND** it SHALL record that A established the scoped COMM CAN FD profile
  before C and that C did not restart or remove shared target services
- **AND** it SHALL distinguish CAN FD frame-format evidence from unproven BRS,
  data-phase-rate, RF, and OTA claims
