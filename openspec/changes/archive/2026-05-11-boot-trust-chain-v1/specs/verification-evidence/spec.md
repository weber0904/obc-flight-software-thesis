## ADDED Requirements

### Requirement: Boot Trust Chain Evidence

The verification evidence tree SHALL record the commands, artifacts, trust model, manifest schema, rejection cases, and final verdict for the first boot trust-chain implementation.

#### Scenario: Boot trust evidence is reviewable
- **WHEN** `boot-trust-chain-v1` completes
- **THEN** reviewers SHALL be able to inspect the selected manifest schema, signer/trust-anchor model, version policy, runtime files changed, tests run, probes run, and observed pass/fail outcomes from `docs/test-records/boot-trust-chain-v1/`.

#### Scenario: Evidence covers trust rejection behavior
- **WHEN** boot trust-chain evidence is recorded
- **THEN** it SHALL include explicit results for valid signed manifest acceptance, invalid signature rejection, unknown signer/key rejection, malformed manifest rejection, downgrade rejection, and staged image digest mismatch rejection.

#### Scenario: Evidence bounds hosted and Raspberry Pi claims
- **WHEN** hosted or Raspberry Pi boot trust probes are cited
- **THEN** the evidence SHALL identify which runtime path was actually proven
- **AND** it SHALL NOT claim hardware secure boot, bootloader partition switching, power-loss resilience, asymmetric signing, or physical media robustness unless those were separately validated.

### Requirement: Boot Component And Helper Coverage

The boot trust-chain change SHALL include both classic F' `BootManager` component coverage and direct helper coverage for parser/verifier behavior.

#### Scenario: BootManager L2 coverage remains required
- **WHEN** `BootManager` component behavior changes for boot trust-chain enforcement
- **THEN** its classic F' L2 harness SHALL cover success, rejection, activation, confirm, rollback, and metadata reload behavior through the component command/event/telemetry surface.

#### Scenario: Helper tests do not replace component coverage
- **WHEN** manifest parsing or signature verification helpers are added
- **THEN** direct L1 helper tests SHALL cover parser/verifier edge cases
- **AND** those helper tests SHALL NOT replace the required `BootManager` component harness coverage.
