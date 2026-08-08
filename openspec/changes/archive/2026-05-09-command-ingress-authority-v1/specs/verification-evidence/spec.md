## ADDED Requirements

### Requirement: Command Ingress Authority Evidence Is Reviewable
Verification evidence for command ingress authority SHALL record the component tests, policy/catalog tests, hosted `sband-primary` and `uhf-backup` configured-profile proof, and explicit deferred boundaries.

#### Scenario: Component evidence covers response semantics
- **WHEN** command ingress authority component tests are reviewed
- **THEN** they SHALL show allowed forwarding, denied no-forward behavior, synthetic response exactly once, source/context preservation, restricted malformed and unknown fail-closed behavior, invalid config denial, throttled events, and unthrottled counters.

#### Scenario: Hosted evidence covers configured S-band and UHF roles
- **WHEN** hosted command ingress authority evidence is recorded
- **THEN** it SHALL include a default CCSDS S-band path result with `sband-primary` configured authority where an authorized command reaches subsystem execution
- **AND** it SHALL include a default CCSDS S-band routed path result with `uhf-backup` configured authority where one allowed status command succeeds and one high-authority command is denied before subsystem execution
- **AND** it SHALL state that this hosted profile proof does not establish physical UHF serial provenance or infer flight-side link identity from `Fw.Com.context`.

#### Scenario: Evidence does not over-claim link authority
- **WHEN** reviewers inspect command ingress authority evidence
- **THEN** the evidence SHALL state that file packet uplink, unknown packet routing, full link authority, full uplink authority, crypto authentication, sessions, sequence windows, replay protection, dynamic failover, and persistent authority config store remain out of scope.
