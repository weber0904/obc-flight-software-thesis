## 1. Governance And Contract

- [x] 1.1 Validate the OpenSpec proposal, design, and spec deltas for `boot-trust-chain-v1`
- [x] 1.2 Keep the implementation aligned with the selected HMAC-SHA256, adjacent-manifest, and activation-floor policies

## 2. BootManager Runtime

- [x] 2.1 Add BootManager-owned manifest parsing, canonicalization, trust-anchor config, and HMAC-SHA256 verification helpers
- [x] 2.2 Extend `BootMetadataStore` metadata load/save for trust status, reject reason, manifest identity, signer/key slot, software versions, and accepted-version floor
- [x] 2.3 Wire `BOOT_VERIFY_STAGED_IMAGE` so stage verification requires size, digest, manifest, signer/key, signature, target slot, and version policy success
- [x] 2.4 Wire activation, confirm, rollback, timeout, and metadata reload so pending-confirm state remains truthful under the trust decision
- [x] 2.5 Extend events, telemetry, runtime status, runtime config, and launch/probe scripts with boot trust state and trust-anchor defaults

## 3. Tests And Probes

- [x] 3.1 Expand the classic `BootManager` F' L2 harness for valid trust flow, all required rejection modes, activation/confirm/rollback, and metadata reload behavior
- [x] 3.2 Add direct L1 tests for manifest parser/canonicalization/signature verifier edge cases
- [x] 3.3 Add a bounded hosted boot trust probe and update the Raspberry Pi boot probe when target access can prove the same trust contract
- [x] 3.4 Update hosted runtime unit tests for boot command/status/config surface changes

## 4. Evidence, Docs, And Validation

- [x] 4.1 Add `docs/test-records/boot-trust-chain-v1/README.md` with bounded hosted/Raspberry Pi evidence claims
- [x] 4.2 Update `docs/verification-path-registry.md`, README/current architecture, and roadmap docs to match the proven boot trust baseline
- [x] 4.3 Run fresh local verification, focused probes, `openspec validate boot-trust-chain-v1`, and `openspec validate --specs`
- [x] 4.4 Archive/sync the OpenSpec change after implementation and evidence are complete
