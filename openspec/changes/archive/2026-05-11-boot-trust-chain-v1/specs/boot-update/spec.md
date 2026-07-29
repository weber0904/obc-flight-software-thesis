## ADDED Requirements

### Requirement: Signed Boot Manifest Verification

The boot/update subsystem SHALL require a signed boot manifest or equivalent signed boot metadata before a staged image can be treated as verified for activation.

#### Scenario: Staged image verification requires manifest trust decision
- **WHEN** `BOOT_VERIFY_STAGED_IMAGE` evaluates a staged image
- **THEN** `BootManager` SHALL verify the staged file size, SHA-256 digest, target slot, signer identity, key slot, signature, and software version from the manifest before setting the staged image as verified
- **AND** digest or size match alone SHALL NOT be sufficient to set the staged image as verified.

#### Scenario: Adjacent manifest is the v1 runtime contract
- **WHEN** `BootManager` receives a staged image path for verification
- **THEN** it SHALL resolve the v1 manifest as an adjacent `<stagingPath>.manifest-v1` file under the configured staging root.

### Requirement: Boot Trust Anchor Model

The boot/update subsystem SHALL verify boot manifests against a repo-controlled runtime/config-backed trust anchor model that binds signer identity, key slot, signature algorithm, and verification key material.

#### Scenario: Trusted signer and key slot are configured locally
- **WHEN** a boot manifest declares a signer and key slot
- **THEN** `BootManager` SHALL accept that identity only if runtime configuration provides a matching trusted signer, key slot, algorithm, and key material
- **AND** it SHALL reject manifest-provided signer or key claims that are not present in the configured trust anchor model.

#### Scenario: HMAC-SHA256 is the v1 manifest algorithm
- **WHEN** boot trust v1 verifies a manifest signature
- **THEN** it SHALL use `HMAC-SHA256` over the canonical manifest fields
- **AND** it SHALL keep the trust-anchor lookup boundary narrow enough for a later provider replacement.

### Requirement: Monotonic Boot Version Policy

The boot/update subsystem SHALL enforce a persisted monotonic software-version floor before staged image activation.

#### Scenario: Verification rejects downgrade candidates
- **WHEN** a manifest software version is less than or equal to the persisted last accepted version
- **THEN** `BootManager` SHALL reject the staged image as a downgrade and SHALL NOT mark it trusted or stage-verified.

#### Scenario: Activation advances accepted version floor
- **WHEN** a trusted staged image is activated into the pending-confirm window
- **THEN** `BootManager` SHALL persist that image version as the new last accepted version
- **AND** later rollback SHALL NOT lower the accepted version floor.

### Requirement: Trust Rejection Is Observable

The boot/update subsystem SHALL make each boot trust rejection reason explicit through runtime state, persisted metadata, events, telemetry, and operator status.

#### Scenario: Trust failure cases are distinguishable
- **WHEN** signature verification fails, signer/key slot is unknown, the manifest is malformed, the manifest version is a downgrade, or the staged image digest mismatches the manifest
- **THEN** `BootManager` SHALL reject activation progress
- **AND** it SHALL expose a distinct trust rejection reason instead of reporting a generic or ambiguous success state.

### Requirement: Trust Decision Is Compatible With Confirm And Rollback

The boot/update subsystem SHALL preserve the existing activation, pending-confirm, confirm, timeout rollback, manual rollback, and metadata reload lifecycle while making those states truthful under the boot trust decision.

#### Scenario: Invalid trust state cannot activate
- **WHEN** the current staged image has not passed the complete boot trust decision
- **THEN** `BOOT_ACTIVATE_STAGED_IMAGE` SHALL reject the command and SHALL NOT enter a pending slot.

#### Scenario: Restart preserves truthful pending trust state
- **WHEN** `BootManager` reloads metadata during an active pending-confirm window
- **THEN** it SHALL preserve the pending slot, trusted manifest identity, staged software version, and remaining confirm behavior needed for `BOOT_CONFIRM` or `BOOT_ROLLBACK`.

#### Scenario: Old ambiguous pending metadata fails closed
- **WHEN** existing metadata predates boot trust fields and claims a pending or stage-verified image
- **THEN** `BootManager` SHALL treat that state as untrusted, roll back to the last-known-good slot, and rewrite metadata in the supported schema.
