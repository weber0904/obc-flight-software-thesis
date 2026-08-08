## ADDED Requirements

### Requirement: Current Node-5 S-band Live Content Uses Curated Summary Instead Of Broad Family Chatter

The current node-`5` auth-gated S-band live stream SHALL treat component-owned
scheduled summary as the baseline live content for the first selected family
set rather than forwarding every scheduled family detail as current operator
truth.

#### Scenario: Post-auth live visibility stays available but thinner
- **WHEN** node-`5` S-band live observability is open after accepted auth
- **THEN** the current baseline SHALL still provide live operator visibility
- **AND** it SHALL prefer curated scheduled summary and critical transition or
  fault events over broad scheduled family detail from `EPS`, `GPS`, `ADCS`,
  `RADIO`, and `STORAGE`.

### Requirement: Raw Radio Observation Remains Controller-Owned But Fresh Readback Is Explicit

The current comm baseline SHALL keep raw radio observation inside
`RadioController`, SHALL NOT promote raw radio detail into COMM policy truth,
and SHALL make `RADIO_GET_STATUS` an explicit fresh bounded readback command.

#### Scenario: Scheduled radio visibility is summary-only
- **WHEN** `RadioController` refreshes radio status on its scheduled path
- **THEN** it SHALL publish only summary observation state needed for current
  live review
- **AND** it SHALL keep raw radio detail out of the scheduled baseline live
  stream.

#### Scenario: Explicit radio status readback is fresh and bounded
- **WHEN** `RADIO_GET_STATUS` or a radio control command requests or returns a
  radio sample
- **THEN** `RadioController` SHALL interpret that sample through the same
  owner-controlled apply path used for cached observation state
- **AND** it SHALL make the detailed radio status reviewable as bounded
  readback without re-expanding broad scheduled live chatter.
