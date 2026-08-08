## Why

The current boot/update baseline verifies staged images with size and SHA-256 digest only, while command ingress now has an authenticated envelope foundation. This change closes the next active-baseline trust gap by making staged image activation depend on signed software provenance, a repo-controlled trust anchor, and monotonic anti-downgrade policy.

## What Changes

- Extend `BootManager` from digest-only staged-image verification to a first boot trust chain over a signed manifest.
- Add a repo-controlled runtime/config-backed trust anchor model for boot manifest signer/key-slot verification.
- Add monotonic software-version policy so activation cannot accept a staged image at or below the persisted accepted version floor.
- Make signature invalid, signer unknown, malformed manifest, version rollback/downgrade, and digest/size mismatch fail closed with explicit runtime state and evidence.
- Preserve the existing `prepare -> verify -> activate -> pending-confirm -> confirm/rollback` lifecycle while making `stageVerified` mean the complete trust decision passed.
- Extend metadata, telemetry/events, runtime status, component tests, helper tests, probes, and evidence so claims stay aligned with what hosted and Raspberry Pi paths actually prove.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `boot-update`: staged image activation now requires signed manifest verification, trusted signer/key-slot configuration, version anti-downgrade policy, and truthful trust-state metadata.
- `verification-evidence`: add boot trust-chain evidence requirements and bounded hosted/Raspberry Pi proof claims.
- `verification-path-registry`: register the reusable boot trust-chain validation path only for the environments actually proven by this change.

## Impact

- Affected runtime: `BootManager`, `BootMetadataStore`, hosted runtime boot status/config, Raspberry Pi boot/update probes, and launch scripts that provide runtime trust-anchor config.
- Affected public surface: boot events/telemetry/status gain trust decision fields; existing `BOOT_*` commands and operator flow remain compatible.
- Affected tests: BootManager classic F' L2 harness is expanded, and manifest/parser/verifier helper behavior gets direct L1 coverage.
- Affected docs: boot/update spec, evidence record, verification-path registry, README/current architecture/roadmap layers where active baseline truth changes.
