## ADDED Requirements

### Requirement: Non-Quiet UHF Primary Runtime Evidence Uses Stability Counts

The verification evidence tree SHALL record fresh target/lab evidence for the
current non-quiet UHF primary runtime using fixed repeated and interleaved
command stability counts.

#### Scenario: Evidence records repeated and interleaved command counts
- **WHEN** `uhf-primary-nonquiet-runtime-v1` evidence is recorded
- **THEN** it SHALL include one repeated single-command case and one interleaved
  dual-command case
- **AND** it SHALL record `10` attempts per subcase, the inter-command spacing,
  success counts, and the final per-subcase verdict.

#### Scenario: Evidence classifies failures instead of collapsing them
- **WHEN** any repetition fails or is degraded
- **THEN** the evidence SHALL classify the result as target runtime failure,
  ground observability failure, or environment/baseline failure
- **AND** it SHALL report those counts explicitly instead of only one overall
  summary line.

### Requirement: Autonomous UHF Failover Evidence Is Reviewable

The verification evidence tree SHALL record reviewable target/lab evidence for
detector-triggered autonomous UHF failover using a governed shared-service
unavailable-window helper.

#### Scenario: Evidence records the governed unavailable window and failover markers
- **WHEN** `target-autonomous-uhf-failover-v1` evidence is recorded
- **THEN** it SHALL identify how `subsystem-sband-csp.service` was stopped and
  restored, the detector and failover markers observed, the target artifact
  roots, and the final verdict.

#### Scenario: Evidence records post-failover re-auth and readback
- **WHEN** autonomous failover evidence records a PASS
- **THEN** it SHALL include UHF secure-auth re-bootstrap, bounded
  `GET_RESET_CAUSE`, bounded `GET_PERSISTENT_FAULT_HISTORY`, and the observed
  post-auth beacon suppress start
- **AND** it SHALL state whether live `event/tlm` and readback were visible on
  the UHF ground path.

### Requirement: Target Watchdog Evidence No Longer Depends On Quiet UHF Baseline

The verification evidence for the current target watchdog-reset proof SHALL use
the maintained secure-auth command-path family without requiring a probe-owned
quiet packet-egress overlay.

#### Scenario: Watchdog evidence records non-quiet post-reboot closure
- **WHEN** target watchdog-reset evidence is refreshed for this change
- **THEN** it SHALL record pre-trigger secure-auth readiness, the reboot edge,
  post-reboot secure-auth re-bootstrap, and final readback on the maintained
  non-quiet baseline
- **AND** it SHALL not require `DIAGNOSTIC_QUIET_PACKET_EGRESS=1` as a current
  acceptance dependency.
