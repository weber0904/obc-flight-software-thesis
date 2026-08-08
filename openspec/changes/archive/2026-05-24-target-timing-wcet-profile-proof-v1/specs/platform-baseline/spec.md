## ADDED Requirements

### Requirement: Active Service-Managed Target Timing Profile Is Explicit

The active Raspberry Pi service-managed target baseline SHALL document the
deployed base tick, rate-group divisors, and resulting nominal fast/slow/data
rates used by the installed `obc-comm-csp-stack.service` path.

#### Scenario: Service-managed timing truth is reviewable

- **WHEN** the active service-managed target baseline is documented or cited as
  current timing truth
- **THEN** the documentation and evidence SHALL identify the deployed base tick
- **AND** they SHALL identify the configured rate-group divisors
- **AND** they SHALL state the resulting nominal fast, slow, and data-group
  rates for that deployed baseline

### Requirement: Target Missed-Tick Policy Uses Upstream ActiveRateGroup Semantics

The active service-managed target timing contract SHALL derive missed-tick
policy from upstream `Svc::ActiveRateGroup` cycle-slip semantics rather than
from repo-local guessed scheduler language.

#### Scenario: Zero-slip service-managed window is explicit

- **WHEN** the repository claims a passing service-managed target timing window
- **THEN** the claim SHALL mean that no `RateGroupCycleSlip` event was observed
  for the active rate groups during the declared observation window
- **AND** it SHALL NOT be described as broader final flight real-time closure
  unless a later governed change proves that stronger claim
