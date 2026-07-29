# legacy-command-envelope-retirement-v1 Evidence

Date:
- `2026-06-05`

OpenSpec change:
- `legacy-command-envelope-retirement-v1`

## Scope

This record closes the current-baseline retirement slice for legacy command
envelope v1 as a preferred operator contract. The slice does not attempt to
remove every historical helper or migrate every adjacent historical proof. It
does prove that the maintained secure baseline no longer depends on the
tracked legacy keystore tuple contract and that the current canonical secure
proof wrappers still pass after retirement.

This record proves:

- current docs/specs/registry now treat secure-auth success, not wire
  `SESSION_OPEN`, as the preferred operator-facing command-session boundary
- the tracked current command-auth keystore contract now contains only
  `module_serial` plus per-service `key_hex` material
- active secure runtime provisioning no longer depends on tracked
  `source_id` / `key_slot` tuple fields
- the canonical hosted secure-auth and hosted uplink-authority proof wrappers
  still pass on the retired keystore/helper contract
- the canonical target secure-auth preflight and full proof still pass after a
  governed installed-release refresh to pick up the new tracked keystore asset
- repo-internal `Authenticated -> opened-session` synthesis remains intact as
  runtime implementation detail

This record does **not** prove:

- removal of repo-internal opened-session/activity/revoke callbacks used by
  current runtime owners
- migration of hosted beacon suppress/runtime, hosted official sequencing with
  `SystemResources`, or target timing/WCET proof families
- hosted payload dual-artifact revalidation
- encryption, RF closure, hardware-backed key storage, or secure-auth redesign

## Dependency Audit Outcome

Retirement outcome for the tracked current baseline:

- `config/security/command-auth.ini` is now a secure-baseline contract with
  top-level `module_serial` and `[sband]` / `[uhf] key_hex` only
- `OBC/Main.cpp` and `OBC/TopCcsds/OBCAppTopology.cpp` now provision
  `SecureLinkAuthorizer` directly with `serviceId + moduleSerial + keyBytes`
  instead of routing active secure provisioning through legacy tuple material
- `OBC/Components/CommandIngressAuthority/CommandAuthKeystore.cpp` rejects
  legacy tuple fields in the tracked current asset path
- `scripts/secure_link_auth_lib.py` loads only the secure-baseline tracked
  keystore contract and fails explicit legacy tuple requests instead of
  presenting them as current behavior

Retired or demoted current-baseline surfaces in this slice:

- legacy maintained command-envelope/session-lifecycle/freshness wrapper
  scripts under `scripts/`
- current wording that treated wire `SESSION_OPEN`, wire `session_id`,
  `source_id`, or `key_slot` as preferred baseline semantics
- legacy non-regression expectations inside the current hosted secure-auth and
  hosted uplink-authority proof wrappers

Preserved by design:

- repo-internal secure-session synthesis after `Authenticated`
- archived legacy evidence under `docs/test-records/`
- non-core historical proof families that still need a future secure-baseline
  replacement if they are ever re-promoted
- archived hosted official sequencing and matrix sequence-subsystem evidence as
  supplemental historical records rather than current secure-baseline gates

## Canonical Proof Wrappers Used

- Hosted secure-auth:
  [scripts/run_challenge_handshake_secure_command_hosted_probe.sh]($REPO_ROOT/scripts/run_challenge_handshake_secure_command_hosted_probe.sh)
- Hosted uplink authority:
  [scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh]($REPO_ROOT/scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh)
- Target secure-auth preflight:
  [scripts/run_target_secure_auth_command_path_probe.sh]($REPO_ROOT/scripts/run_target_secure_auth_command_path_probe.sh)
- Target secure-auth proof:
  [scripts/run_target_secure_auth_proof.sh]($REPO_ROOT/scripts/run_target_secure_auth_proof.sh)

Wrapper/workflow hardening applied during this retirement closeout:

- `scripts/per_band_stock_ground_stacks.py` now uses larger capture frame
  buffers, waits for event/channel listeners, and gates hosted proof traffic
  on gateway readiness markers before probe injection
- hosted secure-auth and uplink-authority probes now log raw injected packets
  and use longer bounded auth waits suitable for the current dual-GDS baseline
- `scripts/run_target_secure_auth_proof.sh` now captures probe output through a
  temp artifact instead of relying on a brittle pipe-only wrapper

## Verification

Focused verification used for this change:

| Step | Command | Result |
|---|---|---|
| Python syntax | `./fprime-venv/bin/python -m py_compile scripts/challenge_handshake_secure_command_hosted_probe.py scripts/uplink_authority_and_key_hardening_hosted_probe.py scripts/per_band_stock_ground_stacks.py scripts/secure_link_auth_lib.py` | PASS |
| native rebuild | `./fprime-venv/bin/cmake --build build-fprime-automatic-native -j4 --target OBC` | PASS |
| focused UT rebuild | `./fprime-venv/bin/cmake --build build-fprime-automatic-native-ut -j4 --target OBC_Components_CommandIngressAuthority_ut_exe OBC_Components_SecureLinkAuthorizer_ut_exe OBC_Components_FileIngressAuthority_ut_exe OBC_Components_CommController_ut_exe` | PASS |
| CommandIngressAuthority UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe` | PASS |
| SecureLinkAuthorizer UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_SecureLinkAuthorizer_ut_exe` | PASS |
| FileIngressAuthority UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_FileIngressAuthority_ut_exe` | PASS |
| CommController UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe` | PASS |
| local verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |

## Hosted Secure-Auth Rerun

Repository-owned proof entrypoint:

```bash
bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| date | `2026-06-05` |
| formal verdict | `challenge-handshake-secure-command-hosted` |
| proof root | `/tmp/challenge-handshake-secure-command-hosted.9d95Wf` |
| security-server log | `/tmp/challenge-handshake-secure-command-hosted.9d95Wf/security-server.log` |
| stack root | `/tmp/challenge-handshake-secure-command-hosted.9d95Wf/combined-stack` |
| runtime root | `/tmp/challenge-handshake-secure-command-hosted.9d95Wf/runtime-root/combined` |
| raw command log | `/tmp/challenge-handshake-secure-command-hosted.9d95Wf/raw-command.log` |
| OBC log | `/tmp/challenge-handshake-secure-command-hosted.9d95Wf/combined-stack/logs/obc.log` |

Observed PASS markers:

```text
challenge-handshake-secure-command-hosted-probe: PASS
formal-verdict=challenge-handshake-secure-command-hosted
secure-auth-service-1=sband
secure-auth-service-2=uhf
handshake-apid=0x00FE
secure-command-apid=0x0000
case-sband-secure-auth-eps-status=PASS
case-uhf-backup-secure-auth-read-continuity-and-high-authority-deny=PASS
case-uhf-primary-reauth-required-and-high-authority-after-reauth=PASS
case-secure-auth-inactivity-timeout-clears-session=PASS
```

This rerun confirms the canonical hosted secure-auth wrapper still produces
actual handshake and secure-command traffic on the current dual-GDS baseline
after the tracked keystore/helper retirement.

## Hosted Uplink-Authority Rerun

Repository-owned proof entrypoint:

```bash
bash scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| date | `2026-06-05` |
| formal verdict | `uplink-authority-and-key-hardening-hosted` |
| proof root | `/tmp/uplink-authority-and-key-hardening-hosted.X0SFzV` |
| security-server log | `/tmp/uplink-authority-and-key-hardening-hosted.X0SFzV/security-server.log` |
| stack root | `/tmp/uplink-authority-and-key-hardening-hosted.X0SFzV/combined-stack` |
| runtime root | `/tmp/uplink-authority-and-key-hardening-hosted.X0SFzV/runtime-root/combined` |
| OBC log | `/tmp/uplink-authority-and-key-hardening-hosted.X0SFzV/combined-stack/logs/obc.log` |

Observed PASS markers:

```text
uplink-authority-and-key-hardening-hosted-probe: PASS
formal-verdict=uplink-authority-and-key-hardening-hosted
case-malformed-handshake-fail-closed=PASS
case-sband-secure-auth-staged-upload-and-sequence-validate=PASS
case-uhf-backup-staged-upload-denied=PASS
case-uhf-failover-primary-reauth-staged-upload-and-sequence-validate=PASS
```

This rerun confirms the current hosted staged-upload authority proof still
passes after removing `source_id` / `key_slot` from the tracked current
keystore contract.

## Target Secure-Auth Refresh And Reruns

The first target preflight on this branch failed the provenance gate because
the installed release still carried the older bundled keystore asset. Per the
current A/B/C target workflow, target proof was not accepted until the tracked
bundle was refreshed.

Governed refresh sequence used:

```bash
bash scripts/bootstrap_rpi_workspace.sh
bash scripts/package_rpi_bundle.sh
bash scripts/install_rpi_bundle.sh build-artifacts/packages/rpi/v0.1.0-195-gaced1249d-dirty/obc-rpi-v0.1.0-195-gaced1249d-dirty.tar.gz
```

### Target Secure-Auth Preflight

Repository-owned proof entrypoint:

```bash
bash scripts/run_target_secure_auth_command_path_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| date | `2026-06-05` |
| verdict | `target-secure-auth-command-path=PASS` |
| proof root | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.kFnne7` |

Observed PASS markers:

```text
case-installed-release-keystore-provenance=PASS
case-sband-apid-00fe-secure-auth=PASS
case-sband-secure-command-get-reset-cause-sequence=41
target-secure-auth-command-path=PASS
```

### Target Secure-Auth Full Proof

Repository-owned proof entrypoint:

```bash
bash scripts/run_target_secure_auth_proof.sh
```

Final passing run:

| Field | Value |
|---|---|
| date | `2026-06-05` |
| formal verdict | `target-secure-auth-proof=PASS` |
| wrapper verdict | `target-secure-auth-proof-v1: PASS` |
| proof root | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.lfOGZp/target-secure-auth-proof-v1` |
| summary log | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.lfOGZp/summary.log` |
| checkpoint stream | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.lfOGZp/target-secure-auth-proof-v1/diagnostics/checkpoints.jsonl` |
| provenance JSON | `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.lfOGZp/target-secure-auth-proof-v1/diagnostics/secure-auth-keystore-provenance.json` |

Observed PASS markers:

```text
target-can-matrix-probe: PASS
mode=secure-auth-proof
profile=uhf-primary
case-installed-release-keystore-provenance=PASS
case-sband-malformed-handshake-fail-closed=PASS
case-sband-apid-00fe-secure-auth=PASS
case-sband-secure-command-first-sequence=41
case-sband-secure-command-strict-next-sequence=PASS
case-sband-secure-auth-staged-upload=PASS
case-uhf-backup-serviceid-2-secure-auth=PASS
case-uhf-backup-read-status-and-high-authority-deny=PASS
case-uhf-backup-secure-auth-staged-upload-denied=PASS
case-uhf-role-switch-invalidates-backup-auth=PASS
case-old-uhf-backup-auth-state-rejected-after-role-switch=PASS
case-uhf-primary-after-failover-reauth-secure-command=PASS
case-restore-sband-primary=PASS
target-secure-auth-proof=PASS
target-secure-auth-proof-v1: PASS
```

The wrapper output still emits the known `openpyxl` / `lxml` interpreter
teardown noise after the PASS markers. That noise is not used as a product
oracle and does not change the proof verdict.

## OpenSpec Validation

| Step | Command | Result |
|---|---|---|
| change validate | `openspec validate legacy-command-envelope-retirement-v1` | PASS |
| specs validate | `openspec validate --specs` | PASS |

## Residual Non-Goals

This slice intentionally leaves these surfaces outside closeout scope:

- repo-internal `Authenticated -> opened-session` synthesis stays in place
- hosted beacon suppress/runtime secure-baseline replacement
- hosted official sequencing + `SystemResources` secure-baseline replacement
- target timing/WCET secure-baseline replacement
- hosted payload dual-artifact rerun or migration

Those surfaces no longer define the preferred current command-session model,
but they are not re-proved or redesigned here.
