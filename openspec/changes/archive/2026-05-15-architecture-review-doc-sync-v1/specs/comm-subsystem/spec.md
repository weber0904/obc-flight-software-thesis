## MODIFIED Requirements

### Requirement: Lab Operational COMM SocketCAN Path Reuses Existing Contract
The lab operational COMM SocketCAN baseline SHALL preserve the existing COMM
node identity, service ports, stock F' framing, and bounded proof scope while
moving runtime management from probes into services. Current file/downlink
validation SHALL be described through official data-product file/downlink
surfaces rather than retired HK fallback files.

#### Scenario: COMM contract remains unchanged under services
- **WHEN** command/event/channel or official data-product file/downlink validation runs over the service-managed lab path
- **THEN** COMM SHALL retain node `4`
- **AND** it SHALL keep services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add COMM service ports, wire layouts, custom GDS plugins, RF behavior, reliable retransmission, or arbitrary onboard file downlink

### Requirement: COMM Runtime State Exposes Per-Band Activity Age And Reasons
The comm subsystem SHALL expose per-band ground-link activity age and
availability reason through reviewable COMM runtime state on the active
baseline. Radio signal-quality metrics such as RSSI and SNR are intentionally
deferred to a future `radio-metrics` change rather than being placeholders in
the v1 provider-owned link-health view.

#### Scenario: Hosted status shows new health fields
- **WHEN** operators inspect hosted COMM runtime status
- **THEN** they SHALL be able to see S-band and UHF activity-age values
- **AND** they SHALL be able to distinguish healthy activity, connected-only fallback, explicit disconnect, and stale-activity reasons

#### Scenario: Radio signal metrics remain deferred
- **WHEN** reviewers inspect the v1 `GroundLinkHealthProvider` surface
- **THEN** RSSI and SNR SHALL NOT be required fields in the current `CommLinkHealthView`
- **AND** any future RSSI/SNR integration SHALL be specified by a later governed radio-metrics change that defines source ownership, units, freshness, and unavailable-value semantics
