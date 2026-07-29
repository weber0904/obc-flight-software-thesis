## ADDED Requirements

### Requirement: Mission Console SHALL Provide A Repo-Owned Ground Operator Surface

The repository SHALL provide a repo-owned `Mission Console` Phase 1 surface
that layers a local Mission Gateway and server-rendered UI above the maintained
manual dual-GDS operator baseline without replacing stock `fprime-gds`.

#### Scenario: Hosted operator can use the Mission Console without replacing stock GDS
- **WHEN** the maintained hosted manual dual-GDS surface is active
- **THEN** operators SHALL be able to open the Mission Console locally and use
  dashboard, operator actions, detailed readback, and surface-status pages
- **AND** stock `fprime-gds` SHALL remain available as a separate engineering
  and fallback observation surface.

### Requirement: Mission Console SHALL Reuse The Existing Operator Authority

The Mission Console SHALL execute secure auth, secure-v2 command, governed
upload, and governed `SEQ_*` actions only through the repo-owned manual secure
operator surface and SHALL NOT introduce a second command authority.

#### Scenario: Mission Console uses the maintained secure helper path
- **WHEN** an operator triggers auth, command, upload, or sequence actions from
  the Mission Console
- **THEN** the Gateway SHALL route those actions through the maintained
  `manual_secure_ops` plus `secure_link_auth_lib.py` path
- **AND** it SHALL NOT claim stock GDS UI interception, direct raw command
  authority, or bypass of `SequenceAdmissionController`.

### Requirement: Mission Console SHALL Own Its Own Listener And Snapshot Layer

The Mission Console SHALL own gateway-managed `events` and `channels` listeners
 for each active operator surface so that dashboard and readback behavior do not
 depend on whether the maintained manual surface already started passive
 listeners.

#### Scenario: Gateway restarts listeners when surface identity drifts
- **WHEN** the active manifest root, `ownerPid`, or `gdsTtsPort` changes for an
  active context/band
- **THEN** the Mission Gateway SHALL restart its owned listener pair for that
  surface
- **AND** it SHALL mark any cached secure session or snapshot state derived
  from the old surface identity as stale.

### Requirement: Mission Console SHALL Distinguish Summary, Transition, And Detailed Readback

The Mission Console SHALL expose four distinct data tiers: surface/lifecycle
truth, keep-live summary, operator-facing transition events, and explicit
detailed readback.

#### Scenario: Dashboard keeps summary and detailed readback distinct
- **WHEN** operators inspect the Mission Console dashboard and readback pages
- **THEN** the dashboard SHALL present keep-live summary and transition state
  without expanding into a raw full channel/event flood
- **AND** detailed `GET_*`, `PAYLOAD_*`, `SEQ_LOG_STATUS`, boot, recovery, and
  persistent-fault observations SHALL remain explicit bounded readback actions.

### Requirement: Mission Console SHALL Support Structured Operator Jobs

The Mission Console SHALL model operator actions as structured requests and
results with persisted job and history records so later planner work can reuse
the same action contract.

#### Scenario: Operator actions are recorded as structured history
- **WHEN** an operator runs an auth, command, upload, sequence, readback, or
  packet-lab action
- **THEN** the Mission Gateway SHALL record a structured result with action
  kind, context, band, timestamps, success/failure, and bounded artifacts or
  readback payloads
- **AND** that history SHALL be available without scraping raw console output.

### Requirement: Mission Console SHALL Provide A Bounded Negative Packet Demo Surface

The Mission Console SHALL provide a lab-only negative packet demo surface that
can inject selected replay, stale-session, sequence, and MAC-error cases and
explain why those packets are wrong using bounded parsed field summaries.

#### Scenario: Demo surface explains why a malformed or replayed packet is wrong
- **WHEN** an operator runs a negative packet demo case from the Mission
  Console
- **THEN** the UI SHALL show the packet source, bounded key fields, the
  intentionally wrong field or condition, the expected failure class, and the
  observed evidence
- **AND** it SHALL NOT require a full raw packet dump to understand the demo.
