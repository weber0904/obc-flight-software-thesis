## Context

`BootManager` currently owns the active boot/update runtime path, but staged image verification is limited to file size and SHA-256 digest. That leaves a gap between the newly authenticated command ingress foundation and the software image provenance needed before accepting an update.

The existing lifecycle and operator surface are valuable and should remain stable: `prepare -> verify -> activate -> pending-confirm -> confirm/rollback`. This design embeds the trust decision into that lifecycle instead of creating a parallel gate.

## Goals / Non-Goals

**Goals:**

- Require a signed manifest before staged image activation.
- Provide a repo-controlled trust-anchor/key-slot model that works in hosted and Raspberry Pi runtime profiles.
- Enforce a monotonic software-version floor so activation rejects downgrade or rollback images.
- Keep all trust rejection reasons observable through BootManager events, telemetry, metadata, runtime status, and evidence.
- Preserve pending-confirm, confirm, timeout rollback, manual rollback, and metadata reload behavior under the new trust decision.

**Non-Goals:**

- Do not implement asymmetric secure boot, hardware root of trust, TPM/secure element integration, bootloader partition handoff, or physical SD-card slot switching.
- Do not add command-payload image upload or file-transfer reliability.
- Do not claim full replay protection, persistent secure command/session storage, or target bootloader enforcement.
- Do not replace the existing `BOOT_*` operator flow.

## Decisions

### Decision: Use adjacent manifest files

`BOOT_VERIFY_STAGED_IMAGE(stagingPath)` will continue to receive the staged image path. The manifest path is resolved as `<stagingPath>.manifest-v1` under the same staging root.

This avoids changing the FPP command signature and keeps existing operator/probe flow stable. The trade-off is that manifest naming is fixed in v1.

Alternative considered: add an explicit manifest path argument. Rejected because it causes avoidable dictionary/API churn for v1.

### Decision: Use canonical key/value manifest v1

The manifest is a bounded text format with required fields:

- `schema=boot_manifest_v1`
- `image_path`
- `image_size`
- `image_digest_sha256`
- `target_slot`
- `image_id`
- `software_version`
- `signer_id`
- `key_slot`
- `signature_algorithm=hmac-sha256`
- `signature`

The signature covers the canonical field block in the fixed order above, excluding `signature`, with one `key=value\n` line per field. Unknown, duplicate, missing, malformed, oversized, or inconsistent fields reject the manifest.

This matches the existing metadata-store style and keeps parser behavior auditable without introducing JSON or third-party parsing dependencies.

### Decision: Use HMAC-SHA256 for v1 manifest authentication

Boot trust v1 uses `HMAC-SHA256` with a runtime/config-backed trust anchor:

- `signer_id=repo-dev-boot-signer`
- `key_slot=1`
- deterministic repo dev key material supplied by launch/runtime config

This reuses the project-owned SHA-256/HMAC primitive introduced for authenticated command envelopes and is directly testable in hosted and Raspberry Pi paths.

Alternative considered: Ed25519 or P-256. Rejected for v1 because it would require adding a new crypto dependency or implementing a larger primitive before the repo has a secure key-storage boundary.

### Decision: Keep trust anchor behind a narrow provider boundary

`BootManager` will receive a boot-trust config from the runtime entrypoint or tests. The config is repo-controlled local runtime state, not trusted manifest input. The implementation should keep lookup as `signer_id + key_slot + algorithm -> key material` so a later secure provider can replace the dev config without rewriting the lifecycle.

### Decision: Activation advances the monotonic version floor

`software_version` must be greater than persisted `last_accepted_version` during verification and is rechecked during activation. The floor advances at successful activation, not at verification and not at confirm. Rollback never lowers the floor.

This allows manifest verification retries before activation while preventing reactivation of an older accepted image even if rollback returns runtime to the previous last-known-good slot.

### Decision: Preserve old confirmed metadata but fail closed for old pending trust state

Existing confirmed metadata without trust fields loads as a confirmed baseline with software version `0` and `last_accepted_version=0`. Existing pending or stage-verified metadata without trust fields is not trusted; BootManager rolls back to the last-known-good slot and rewrites metadata in the supported schema.

This keeps deployed runtime bootable while avoiding ambiguous "verified" state after the trust model changes.

## Risks / Trade-offs

- HMAC is symmetric and not equivalent to asymmetric secure boot -> Evidence will call this repo-controlled dev trust-anchor boot trust v1, not hardware secure boot.
- Adjacent manifest naming is less flexible -> Keep the rule explicit and bounded in docs/probes.
- Activation-floor policy rejects reactivation of a rolled-back version -> This is intentional anti-downgrade behavior and must be visible in metadata/status.
- Metadata schema grows in place -> Parser must accept safe old confirmed metadata, reject ambiguous old pending trust state, and rewrite cleanly.

## Migration Plan

1. Create OpenSpec artifacts and delta specs before runtime work.
2. Add manifest/trust helper logic under the BootManager module and direct L1 tests.
3. Extend `BootManager` FPP, metadata, runtime config/status, launch scripts, and classic L2 harness.
4. Add a focused hosted boot trust probe and update the Raspberry Pi boot probe if target access is available.
5. Update evidence, path registry, README/current architecture, and roadmap docs to match the proven scope.
6. Run fresh local verification, focused probes, `openspec validate boot-trust-chain-v1`, and `openspec validate --specs`.
