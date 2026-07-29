## ADDED Requirements

### Requirement: Hosted Command Ingress Authority Path Includes Persistent Freshness Boundary

The verification-path registry SHALL extend the hosted command ingress authority
profile path to distinguish reboot-safe persistent freshness from earlier auth,
lifecycle, and sequence-only claims.

#### Scenario: Registry identifies persistent freshness proof boundary
- **WHEN** the hosted command ingress authority profile path is updated by
  `persistent-command-freshness-v1`
- **THEN** the entry SHALL state that the path proves persisted source-epoch
  reopen floors, replayed or lower/equal `SESSION_OPEN` rejection after
  same-runtime-root restart, stale old-session fail-closed behavior, and fresh
  higher reopen success on the active hosted ingress path
- **AND** it SHALL keep legacy non-envelope traffic outside that persistence
  claim.

#### Scenario: Registry keeps persistent freshness scope bounded
- **WHEN** reviewers inspect the hosted command ingress authority registry entry
  after this change
- **THEN** the entry SHALL state that the path still does not prove nonce-based
  replay protection, persistent secure key storage, hosted ingress port `1`,
  simultaneous S-band/UHF routed ingress, target hardware power-loss behavior,
  or legacy retirement.

### Requirement: Raspberry Pi Active OBC Freshness Persistence Path Is Registered Separately

The verification-path registry SHALL include a distinct Raspberry Pi active-path
entry for bounded command-freshness and boot-trust persistence proof on the
governed active `OBC` package path.

#### Scenario: Registry identifies active target persistence boundary
- **WHEN** a Raspberry Pi persistence probe passes for
  `persistent-command-freshness-v1`
- **THEN** the registry SHALL identify the path as active-package
  `TopCcsds` target restart or reboot persistence for boot-trust metadata plus
  command-ingress freshness state
- **AND** it SHALL state exactly whether the proof covered installed current
  release, autostart relaunch, or interactive target runtime only.

#### Scenario: Registry keeps broader target claims out of scope
- **WHEN** reviewers inspect the Raspberry Pi persistence entry after this
  change
- **THEN** the entry SHALL state that it does not prove bootloader handoff,
  hardware secure boot, power-loss robustness, full COMM lab-operational
  closure, RF behavior, or final flight deployment readiness.
