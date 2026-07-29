# Boot Trust Chain v1 Evidence

## Scope

This record captures implementation and verification evidence for the `boot-trust-chain-v1` OpenSpec change.

## Environment

- Date: `2026-05-12`
- Workspace: `$REPO_ROOT`
- Branch: `feature/boot-trust-chain-v1`
- Framework baseline: `lib/fprime` pinned to `v4.1.0`
- Virtual environment: `$REPO_ROOT/fprime-venv`
- Host validation mode: Darwin hosted build plus file-backed boot metadata under isolated runtime roots

## Implemented Artifacts

- `BootManager` now requires a trust decision before staged image activation.
- `BootManifestVerifier` parses adjacent `*.manifest-v1` files, canonicalizes manifest fields, and verifies `HMAC-SHA256` signatures using the existing command-auth crypto helper.
- Runtime-config-backed boot trust anchor fields are available through hosted runtime args and launch scripts:
  - `--boot-trust hmac-sha256`
  - `--boot-trust-signer-id`
  - `--boot-trust-key-slot`
  - `--boot-trust-key-hex`
- `BootMetadataStore` persists trust status, reject reason, signer/key identity, staged/active/pending software versions, and `last_accepted_version`.
- `BootManager` events, telemetry, runtime status, and metadata now expose truthful trust state.
- `scripts/run_boot_trust_chain_probe.sh` exercises the hosted trust-chain path.
- `scripts/run_rpi_boot_probe.sh` was updated to generate a signed manifest for the existing target-side boot restart probe.

## Boot Trust Contract

- Manifest path: adjacent to the staged image as `<staged-image>.manifest-v1`.
- Required manifest fields:
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
- Trusted signer model: one runtime-configured signer identity, key slot, and hex key in v1.
- Anti-downgrade policy: `BOOT_VERIFY_STAGED_IMAGE` accepts only manifests whose `software_version` is greater than persisted `last_accepted_version`; activation rechecks the floor and advances it before pending confirm. Confirm and rollback do not lower the floor.
- Reject reasons are explicit for missing/malformed manifest, unknown signer/key slot, invalid signature, version downgrade, target-slot mismatch, digest mismatch, size mismatch, image-path mismatch, and invalid config.

## Commands Run

1. `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f`
2. `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build`
3. `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f`
4. `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut`
5. `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_BootManager_ut_exe`
6. `./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test`
7. `bash scripts/run_boot_trust_chain_probe.sh`
8. `openspec validate boot-trust-chain-v1`
9. `openspec validate --specs`
10. `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-boot-trust-chain-v1`

## Results

- Native hosted build completed successfully.
- UT build completed successfully.
- `OBC_Components_BootManager_ut_exe`: passed `23/23`.
  - `7` direct `BootManifestVerifier` L1 tests.
  - `16` classic `BootManager` F' L2 harness tests.
  - Post-review regression coverage includes manifest digest mismatch rejection, uppercase trust-key hex acceptance, activation-time staged-byte revalidation after post-verify tamper, nested relative manifest image path activation, invalid pending trust metadata rollback, legacy pending metadata rollback recovery, and prepare rejection during an active pending-confirm window.
- `hosted_runtime_unit_test`: `PASS`.
- `run_boot_trust_chain_probe.sh`: `PASS`.
- `openspec validate boot-trust-chain-v1`: `PASS`.
- `openspec validate --specs`: `PASS`.
- `run_verification_ci.sh`: `PASS`.
  - `01_generate`: `PASS`
  - `02_build`: `PASS`
  - `03_generate_ut`: `PASS`
  - `04_build_ut`: `PASS`
  - `05_check_all`: `PASS`
  - `06_check_repo_consistency`: `PASS`
  - `07_check_component_test_baseline`: `PASS`
  - `08_check_legacy_zmq_retired`: `PASS`
  - `09_openspec_validate_specs`: `PASS`

## Hosted Probe Evidence

The hosted probe used `/tmp/boot-trust-chain-v1-probe` and produced:

- `/tmp/boot-trust-chain-v1-probe/logs/valid-activation.log`
- `/tmp/boot-trust-chain-v1-probe/logs/confirm-reload.log`
- `/tmp/boot-trust-chain-v1-probe/logs/invalid-signature.log`
- `/tmp/boot-trust-chain-v1-probe/logs/downgrade.log`

The probe proves:

- valid signed manifest plus matching staged image is accepted
- activation enters pending confirm with `trustStatus=3` and `lastAcceptedVersion=1`
- restart/status/confirm preserves and completes the pending trusted state with `trustStatus=4`
- invalid signature is rejected with `trustRejectReason=14`
- version rollback/downgrade is rejected with `trustRejectReason=15`
- rejected trust decisions do not advance `lastAcceptedVersion`

## Target Path Notes

- `scripts/run_rpi_boot_probe.sh` now generates and passes a signed manifest using the same v1 contract.
- This evidence record does not claim a fresh Raspberry Pi target run for `boot-trust-chain-v1`.
- Target claims remain limited to previously archived boot/update restart evidence until a fresh RPi probe run is recorded.

## Remaining Gaps

- No hardware-backed key storage or secure element is part of v1.
- No Raspberry Pi bootloader, partition handoff, SD-card power-loss recovery, or full flight secure boot chain is proven.
- `HMAC-SHA256` is a repo-controlled software trust closure suitable for the current runtime baseline, not an asymmetric production signing scheme.
