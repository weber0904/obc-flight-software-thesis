## ADDED Requirements

### Requirement: Authenticated Command Envelope Evidence Is Reviewable

Verification evidence SHALL prove authenticated envelope acceptance and fail-closed rejection without claiming full replay protection.

#### Scenario: Component coverage includes auth acceptance and rejection boundaries
- **WHEN** `CommandIngressAuthority` component and helper tests are reviewed
- **THEN** evidence SHALL list coverage for valid authenticated `SESSION_OPEN`, valid authenticated post-open increasing sequence traffic, invalid MAC rejection, malformed or unknown-key rejection, authority-denied no-consume behavior, lifecycle-denied no-sequence-consume behavior, reboot/recreate reopen behavior, and `uhf-backup` continuity without authority expansion.

#### Scenario: Hosted probe proves authenticated order on current hosted path
- **WHEN** hosted authenticated command envelope evidence is recorded
- **THEN** it SHALL reuse the current hosted CCSDS S-band routed command path and current authority ingress port `0`
- **AND** it SHALL include observations proving `parse -> auth -> authority -> lifecycle -> sequence -> dispatch`
- **AND** it SHALL include valid authenticated `SESSION_OPEN(seq0)` acceptance, increasing post-open acceptance, invalid MAC rejection, malformed or unknown-key rejection, auth-pass authority-denied no-consume behavior, stale or mismatched session rejection, reboot-reopen behavior, and `uhf-backup` authenticated read/status continuity.

#### Scenario: Evidence records bounded rejection surfaces
- **WHEN** authenticated command envelope evidence is recorded
- **THEN** it SHALL include command responses, dedicated auth-failure event or telemetry excerpts, lifecycle/sequence evidence where applicable, and confirmation that rejected commands did not reach downstream dispatch.

#### Scenario: Evidence scope remains bounded
- **WHEN** evidence cites `command-auth-envelope-v1`
- **THEN** it SHALL state that the proof covers the current hosted authority ingress port `0` only
- **AND** it SHALL NOT claim full replay protection, persistent anti-replay state, persistent secure key storage, hosted ingress port `1`, simultaneous S-band/UHF routed ingress, trusted boot chain, file/unknown uplink authority, RF behavior, target hardware, or Pi deployment.
