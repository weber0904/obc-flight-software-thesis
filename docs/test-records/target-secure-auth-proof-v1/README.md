# target-secure-auth-proof-v1 Evidence

Current-note:

- this record remains current for the governed target secure-auth bootstrap and
  bounded uplink-authority path on node `5` plus bounded node `6`
  compatibility
- any explicit switched `uhf-primary-after-failover` wording here is not the
  maintained current autonomous-failover proof family by itself
- do not cite this record alone as current Route 2/3 target closure or current
  UHF-primary operator truth; use the archived 2026-06-24 non-quiet
  autonomous-failover and Chapter 5 records for that

Date:
- `2026-06-01` local lab time; final proof includes the review follow-up
  provenance gate for forbidden `--command-auth*` CLI injection.

OpenSpec change:
- `target-secure-auth-proof-v1`

## Scope

This record moves the already-hosted secure auth, secure command v2, and
bounded uplink-authority behavior onto the governed target/lab paths. It does
not treat hosted proof as target proof; hosted entries `43E` and `43F` are
ancestry only.

Target paths under test:

- S-band: registry entry `59`, macOS `fprime-gds` plus
  `ground_ttc_gateway(raw relay)` to `subsystem.local` node `5`, SocketCAN,
  and `obc.local`.
- UHF: registry entry `69`, physical node-`6` `uhf-backup` adjunct plus
  explicit switch to `uhf-primary-after-failover`.

This record proves:

- installed-release keystore provenance for
  `$OBC_HOME/obc-deploy/current/config/security/command-auth.ini`
- target S-band APID `0x00FE` challenge auth and secure command v2 on the
  command APID
- first accepted target S-band secure command may use non-`1` sequence `41`
  and the next accepted command must be strictly sequence `42`
- duplicate target S-band secure command sequence `42` is rejected
- malformed S-band handshake fails closed without challenge/status or session
  mutation
- S-band `.sequence-staging/<leaf>` staged upload is admitted after secure
  auth
- target UHF physical node-`6` secure auth uses `ServiceID = 2`
- `uhf-backup` accepts read/status secure command, denies high-authority
  `MODE_SET`, and does not consume the denied sequence
- `uhf-backup` denies staged upload even after secure auth
- switching to `uhf-primary-after-failover` invalidates the old UHF
  auth/session state
- `uhf-primary-after-failover` requires re-auth before secure-command
  acceptance

This record does **not** prove:

- UHF primary staged-upload success
- encryption
- RF closure
- boot-trust expansion
- hardware-backed or persistent secure key storage
- generic file authority beyond `.sequence-staging/<leaf>`
- one-GDS aggregation
- one-gateway simultaneous S-band/UHF multiplexing
- legacy command envelope v1 retirement

## Implemented Entry Points

- [scripts/run_target_secure_auth_proof.sh]($REPO_ROOT/scripts/run_target_secure_auth_proof.sh)
- [scripts/comm_verification/lib/run_target_can_matrix_probe.py]($REPO_ROOT/scripts/comm_verification/lib/run_target_can_matrix_probe.py)
- [scripts/secure_link_auth_lib.py]($REPO_ROOT/scripts/secure_link_auth_lib.py)
- [scripts/security_server_sim.py]($REPO_ROOT/scripts/security_server_sim.py)
- [config/security/command-auth.ini]($REPO_ROOT/config/security/command-auth.ini)

The target proof loads the tracked keystore from the installed release and
does not introduce `COMMAND_AUTH_*` or `--command-auth-*` runtime injection.
`run_target_can_matrix_probe.py` is a shared implementation helper here; it is
not, by itself, the governing authority for current secure-auth semantics.
Current authority comes from this dedicated wrapper, registry entry `70`, and
the current target/lab runbook.

## Target Verdict

Repository-owned proof entrypoint:

```bash
bash scripts/run_target_secure_auth_proof.sh
```

Final passing run:

| Field | Value |
|---|---|
| date | `2026-06-01` |
| formal verdict | `target-secure-auth-proof=PASS` |
| wrapper verdict | `target-secure-auth-proof-v1: PASS` |
| proof root | `/private/tmp/target-secure-auth-proof-v1.f71Pvv/target-secure-auth-proof-v1` |
| summary JSON | `/private/tmp/target-secure-auth-proof-v1.f71Pvv/target-secure-auth-proof-v1/diagnostics/secure-auth-proof-summary.json` |
| provenance JSON | `/private/tmp/target-secure-auth-proof-v1.f71Pvv/target-secure-auth-proof-v1/diagnostics/secure-auth-keystore-provenance.json` |
| checkpoint stream | `/private/tmp/target-secure-auth-proof-v1.f71Pvv/target-secure-auth-proof-v1/diagnostics/checkpoints.jsonl` |
| cleanup status | `/private/tmp/target-secure-auth-proof-v1.f71Pvv/target-secure-auth-proof-v1/diagnostics/cleanup-status.json` |
| OBC journal snapshot | `/private/tmp/target-secure-auth-proof-v1.f71Pvv/target-secure-auth-proof-v1/diagnostics/journal-snapshots/secure-auth-proof-obc.log` |
| S-band service journal snapshot | `/private/tmp/target-secure-auth-proof-v1.f71Pvv/target-secure-auth-proof-v1/diagnostics/journal-snapshots/secure-auth-proof-sband-service.log` |
| UHF service journal snapshot | `/private/tmp/target-secure-auth-proof-v1.f71Pvv/target-secure-auth-proof-v1/diagnostics/journal-snapshots/secure-auth-proof-uhf-service.log` |

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

The final wrapper exited `0`, `cleanup-status.json` reported `PASS`, and the
post-run process scan found no proof-owned local residue. The wrapper output
also contains F' GDS/openpyxl `Exception ignored` messages during interpreter
teardown after the proof PASS markers; those messages are local file-uplink log
cleanup noise and are not used as a target behavior oracle.

Gateway capture artifacts:

| Path | Size | SHA-256 |
|---|---:|---|
| `captures/sband/gds-to-southbound.bin` | `716` | `cec9f2c0ff6b7cd0c4de30db1a8c1c4868c046db098cf83160053afd31a0c922` |
| `captures/sband/southbound-to-gds.bin` | `329088` | `50a5e05274cab0216b99c5f6fe113442b19495f438ba2428b824b45f535cfcee` |
| `captures/uhf/gds-to-southbound.bin` | `856` | `d83bdb04b39e9fa289bf7051e472cdf72efe3be42c0d7964d2ca84617aa39168` |
| `captures/uhf/southbound-to-gds.bin` | `4096` | `623decd6b1c657ac98da53820c2e8642740d83361b5d3cfcb5b8ac868dcd3eef` |

## Provenance Gate

The passing proof accepted the installed release only after the provenance
gate passed:

- installed release root: `$OBC_HOME/obc-deploy`
- `current` symlink: present
- resolved release:
  `$OBC_HOME/obc-deploy/releases/v0.1.0-189-g53a3cd4c7-dirty`
- service `WorkingDirectory`: `$OBC_HOME/obc-deploy/current`
- target service: `obc-comm-csp-stack.service`
- expected target service environment:
  `TARGET_COMM_PROFILE=sband`, `COMM_CSP_NODE=5`,
  `COMMAND_AUTHORITY_PROFILE=sband-primary`
- forbidden `COMMAND_AUTH_*` service environment and `--command-auth*` CLI
  injection: absent
- repo keystore SHA:
  `2be51e353d2206b41da96bea4e4f0dd9dac21add5efde9b315a8da6530ef9e89`
- bundled installed keystore SHA:
  `2be51e353d2206b41da96bea4e4f0dd9dac21add5efde9b315a8da6530ef9e89`
- manifest recorded keystore SHA:
  `2be51e353d2206b41da96bea4e4f0dd9dac21add5efde9b315a8da6530ef9e89`
- manifest SHA:
  `021e8a820406c96adfe9b14127772d3ae88d3264b193f0596e3aca9e501331ee`

Before the final proof, an earlier preflight exposed stale installed release
and subsystem service provenance, including obsolete runtime command-auth
injection flags. That was classified as environment/provenance staleness, then
closed by rerunning the governed sync/bootstrap/package/install/service
workflow before accepting any secure-auth result.

## Verification

Focused verification used for this change:

| Step | Command | Result |
|---|---|---|
| shell syntax | `bash -n scripts/run_target_secure_auth_proof.sh` | PASS |
| Python syntax | `python -m py_compile scripts/comm_verification/lib/run_target_can_matrix_probe.py scripts/secure_link_auth_lib.py scripts/security_server_sim.py` | PASS |
| product unit tests | focused component UTs | Not run; no product code changed |
| fresh local verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| target preflight | `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh` | PASS |
| target proof | `bash scripts/run_target_secure_auth_proof.sh` | PASS |
| cleanup check | `cleanup-status.json` and local process scan | PASS |
| OpenSpec change validate | `openspec validate target-secure-auth-proof-v1` | PASS |
| OpenSpec specs validate | `openspec validate --specs` | PASS |

## Path Registration Impact

This record creates a new target proof boundary for:

- installed-release keystore provenance on the active target service root
- target S-band APID `0x00FE` secure auth and secure command v2
- target S-band staged upload admission after secure auth
- bounded physical node-`6` UHF secure-auth behavior on `ServiceID = 2`
- `uhf-backup` denial for high-authority command and staged upload
- role-switch invalidation and UHF re-auth before
  `uhf-primary-after-failover` secure-command acceptance

It does not widen:

- hosted proof entries `43E` or `43F`
- generic arbitrary file-uplink governance
- UHF primary staged-upload behavior
- legacy v1 retirement
- encryption, RF, boot trust, or key-storage claims
