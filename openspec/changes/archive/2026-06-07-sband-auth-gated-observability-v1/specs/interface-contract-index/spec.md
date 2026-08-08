## ADDED Requirements

### Requirement: Interface Index Records The Current Observability Tier Boundary

`docs/interfaces.md` SHALL summarize the current observability tier boundary so
reviewers can distinguish always-on critical surfaces, auth-gated S-band live
packet visibility, and bounded `GET_*` summary readback.

#### Scenario: Reviewers can audit the live observability tiers in one place
- **WHEN** a reviewer inspects the COMM observability section of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that:
  - UHF beacon remains a no-ACK always-on critical surface
  - packetized S-band live `event/tlm` is quiet until accepted S-band secure
    auth opens the current live packet session
  - bounded `GET_*` or read/status commands remain the on-demand summary tier
- **AND** they SHALL be able to see that these tiers are current operator truth,
  not future generic telemetry redesign.

#### Scenario: Interface index keeps live packet visibility distinct from summary readback
- **WHEN** the same section describes readback behavior
- **THEN** it SHALL distinguish packetized live observability from bounded
  component-owned summary events/telemetry emitted by `GET_*` or status
  commands
- **AND** it SHALL NOT present those summary readbacks as proof that broad live
  packet chatter remains always-on by default.
