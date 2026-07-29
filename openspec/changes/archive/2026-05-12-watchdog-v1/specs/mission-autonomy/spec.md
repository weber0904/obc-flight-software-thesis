## ADDED Requirements

### Requirement: WatchdogSupervisor Owner Is Separate From Mode Safety And EPS Timeout FDIR

The mission-autonomy capability SHALL provide a focused `WatchdogSupervisor` owner for runtime liveness supervision, and the active runtime SHALL keep that owner separate from the SoC-only `ModeSafetyController` and the EPS-timeout-only `EpsFdirController`.

#### Scenario: Watchdog-v1 does not broaden existing owners

- **WHEN** `watchdog-v1` is implemented
- **THEN** the active runtime SHALL instantiate, configure, and schedule a focused watchdog owner
- **AND** `ModeSafetyController` SHALL remain limited to cached-EPS SoC policy
- **AND** `EpsFdirController` SHALL remain limited to EPS timeout retry, fault latch, and recovery behavior

### Requirement: Watchdog-v1 Uses Explicit Heartbeats For A Bounded Source Set

The mission-autonomy capability SHALL define watchdog-v1 from explicit per-source heartbeats emitted by a bounded active-baseline source set.

#### Scenario: Supervised set is explicit

- **WHEN** `watchdog-v1` is implemented
- **THEN** the supervised source set SHALL include `EpsBridge`, `EpsFdirController`, `ModeSafetyController`, and `CommController`
- **AND** it SHALL NOT implicitly claim `ModeManager`, slow-group owners, or data-group owners are supervised in this change

#### Scenario: V1 heartbeat is explicit rather than inferred

- **WHEN** `WatchdogSupervisor` evaluates liveness in this change
- **THEN** it SHALL use explicit heartbeat submission from the supervised owners
- **AND** it SHALL NOT infer source freshness only from expected schedule cadence or upstream active-component ping alone

### Requirement: Watchdog-v1 Uses Deterministic Tick-Based Freshness

The mission-autonomy capability SHALL define watchdog-v1 freshness from deterministic tick-based age thresholds per supervised source.

#### Scenario: Threshold crossings are ordered

- **WHEN** a supervised source ages past its configured warning, safe, and suppress thresholds
- **THEN** the watchdog owner SHALL evaluate those thresholds in that order
- **AND** it SHALL treat warning as less severe than latched fault
- **AND** it SHALL treat feed suppression as more severe than latched fault

#### Scenario: V1 does not add persistent watchdog config

- **WHEN** `SET_WATCHDOG_CONFIG` changes watchdog-v1 thresholds or enable flags
- **THEN** the updated config SHALL apply to the current runtime only
- **AND** it SHALL NOT by itself claim reboot-persistent watchdog config storage in this change

### Requirement: Watchdog Escalation Uses The Normal Mode Surface

The mission-autonomy capability SHALL make watchdog mode escalation use the existing internal mode-control path instead of inventing a parallel watchdog-owned mode store.

#### Scenario: First watchdog fault crossing requests SAFE once

- **WHEN** a supervised source first crosses the configured watchdog fault threshold
- **AND** the current mode is `IDLE`, `PAYLOAD`, or `TTC`
- **THEN** `WatchdogSupervisor` SHALL request `SAFE` through the normal internal mode-control runtime path
- **AND** it SHALL tag that request with a distinct watchdog-fault internal apply source

#### Scenario: SAFE and HELL are fault-only watchdog cases

- **WHEN** a supervised source first crosses the configured watchdog fault threshold
- **AND** the current mode is `SAFE` or `HELL`
- **THEN** `WatchdogSupervisor` SHALL record the fault condition
- **AND** it SHALL NOT request another mode change while the current mode remains `SAFE` or `HELL`

#### Scenario: Latched watchdog fault can defer SAFE until the mode is requestable

- **WHEN** a supervised source remains watchdog-fault-latched without a prior watchdog `SAFE` request
- **AND** the current mode later becomes `IDLE`, `PAYLOAD`, or `TTC`
- **THEN** `WatchdogSupervisor` SHALL request `SAFE` once through the normal internal mode-control runtime path
- **AND** it SHALL keep that request tagged with the distinct watchdog-fault internal apply source

#### Scenario: Repeated stale evaluation does not re-escalate while latched

- **WHEN** a supervised source remains stale after watchdog fault has already latched
- **AND** `WatchdogSupervisor` has already issued a watchdog `SAFE` request for that fault epoch
- **THEN** `WatchdogSupervisor` SHALL NOT repeatedly re-request `SAFE`
- **AND** it SHALL keep later escalation bounded to watchdog recovery-level progression only

### Requirement: Watchdog Feed Suppression Remains A Supervisor-Side Claim

The mission-autonomy capability SHALL let watchdog-v1 suppress watchdog feed eligibility without over-claiming target reset closure.

#### Scenario: Continued stale source suppresses feed eligibility

- **WHEN** a supervised source continues stale long enough to cross the configured feed-suppression threshold
- **THEN** `WatchdogSupervisor` SHALL mark watchdog feed as ineligible
- **AND** it SHALL emit reviewable feed-suppressed or restart-intent evidence

#### Scenario: V1 does not claim target watchdog reset

- **WHEN** watchdog-v1 records feed suppression in this change
- **THEN** it SHALL NOT by itself claim Raspberry Pi hardware watchdog stroking, watchdog-caused reset, boot-safe-image recovery, or reset-cause persistence

### Requirement: Watchdog Recovery Clears On First Restored Beat

The mission-autonomy capability SHALL clear watchdog-v1 stale state on the first restored heartbeat according to explicit aggregate rules.

#### Scenario: Source recovery clears its stale state

- **WHEN** a supervised source has crossed watchdog warning, fault, or suppress thresholds
- **AND** that source later emits a valid heartbeat
- **THEN** `WatchdogSupervisor` SHALL clear that source's stale state immediately

#### Scenario: Recovery does not auto-restore prior mode

- **WHEN** watchdog-v1 clears a latched source fault after heartbeat recovery
- **THEN** it SHALL NOT automatically restore the pre-fault mode
- **AND** it SHALL leave later mode-recovery policy to future governed changes

#### Scenario: Aggregate recovery waits for all enabled sources

- **WHEN** one stale source recovers while another enabled supervised source remains faulted or suppressed
- **THEN** aggregate watchdog fault or feed-suppressed state SHALL remain active
- **AND** the aggregate state SHALL clear only after no enabled source remains faulted or suppressed
