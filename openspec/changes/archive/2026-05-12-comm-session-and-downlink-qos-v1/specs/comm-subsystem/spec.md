## ADDED Requirements

### Requirement: COMM-Owned Operational Link Roles

The comm subsystem SHALL own explicit runtime roles for primary command, primary telemetry, and primary file-transfer links, together with per-link availability and reviewable transition state for the active baseline.

#### Scenario: Default startup roles are explicit

- **WHEN** the active hosted `TopCcsds` baseline starts
- **THEN** COMM SHALL report `S-band` as the primary command, telemetry, and file-transfer link
- **AND** it SHALL report UHF as the bounded backup command path when UHF ingress is configured and available

#### Scenario: Operator switches the primary link

- **WHEN** the operator executes the governed COMM primary-link switch command
- **THEN** COMM SHALL update primary command, telemetry, and file-transfer roles together
- **AND** it SHALL emit reviewable events or telemetry reflecting the new primary-link state

### Requirement: COMM Drives Authenticated Ingress Policy By Link Role

The comm subsystem SHALL drive runtime authenticated command policy for each active ingress path according to the current COMM link role while leaving authenticated envelope verification, session-open lifecycle, and strict-monotonic sequence ownership inside `CommandIngressAuthority`.

#### Scenario: S-band primary allows full authenticated command set

- **WHEN** an authenticated command arrives through the S-band ingress while COMM marks S-band as primary
- **THEN** the command SHALL be evaluated against the full catalog policy rather than the restricted UHF-backup policy

#### Scenario: UHF backup stays low-risk

- **WHEN** an authenticated command arrives through the UHF ingress while COMM marks UHF as backup
- **THEN** COMM-driven policy SHALL allow only bounded read/status or explicitly allowlisted low-risk commands
- **AND** it SHALL reject higher-risk mode, config, reset, power, ADCS, COMM-control, file-transfer, and data-product control commands

#### Scenario: UHF primary gains full authenticated authority

- **WHEN** COMM marks UHF as primary
- **THEN** authenticated commands arriving through the UHF ingress SHALL be evaluated against the full catalog policy, including governed data-product and file-transfer commands

### Requirement: Observe-Only Pass State

The comm subsystem SHALL continue to publish pass-active and pass-end state for operator visibility, but pass state SHALL NOT by itself admit or deny authenticated commands or new file-transfer ownership on the active baseline.

#### Scenario: Pass transition is observable only

- **WHEN** a governed COMM pass starts or stops
- **THEN** COMM SHALL update reviewable pass events, telemetry, or counters
- **AND** the authenticated command and downlink admission policy SHALL continue to be driven by link availability and primary-link role rather than pass state alone

### Requirement: Shared File-Downlink Ownership Is COMM-Owned

The comm subsystem SHALL own the shared file/downlink policy surface used by official data products and bounded housekeeping fallback on the active baseline.

#### Scenario: DP catalog is the active owner

- **WHEN** a `DpCatalog` file/downlink request is accepted
- **THEN** COMM SHALL record `DpCatalog` as the active shared downlink owner until completion, cancellation, or owner drop

#### Scenario: HK fallback is busy-rejected during active DP transfer

- **WHEN** a `HousekeepingArchive` downlink request arrives while `DpCatalog` owns the active shared downlink surface
- **THEN** COMM SHALL reject the HK request as busy instead of bypassing or preempting the active DP transfer

#### Scenario: DP waits behind an active HK fallback transfer

- **WHEN** a `DpCatalog` request arrives while `HousekeepingArchive` owns the active shared downlink surface
- **THEN** COMM SHALL retain at most one pending DP request
- **AND** it SHALL launch that DP request immediately after the active HK transfer completes if the primary file-transfer link is still available

### Requirement: COMM Converges State On Link Loss Or Primary Switch

The comm subsystem SHALL converge authenticated-session policy and shared downlink ownership state when the current primary link becomes unavailable or the primary-link role changes.

#### Scenario: Active owner link becomes unavailable

- **WHEN** the current active shared downlink owner's link becomes unavailable
- **THEN** COMM SHALL clear the active owner state
- **AND** it SHALL drop any pending shared downlink request that no longer matches the current primary file-transfer link policy
- **AND** it SHALL emit reviewable reasoned observability for the drop

#### Scenario: Primary switch revokes the old command session profile

- **WHEN** COMM changes the primary command link
- **THEN** any authenticated session that no longer matches the COMM-driven ingress role policy SHALL be revoked through `CommandIngressAuthority`
- **AND** new commands SHALL be evaluated against the updated ingress role immediately
