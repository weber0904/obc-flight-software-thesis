## 1. OpenSpec Artifacts

- [x] 1.1 Create the `target-secure-auth-proof-v1` OpenSpec change.
- [x] 1.2 Write proposal, design, tasks, and scoped delta specs.

## 2. Target Proof Harness

- [x] 2.1 Add `scripts/run_target_secure_auth_proof.sh` as the repository-owned entrypoint.
- [x] 2.2 Extend `scripts/comm_verification/lib/run_target_can_matrix_probe.py` with `secure-auth-proof` mode.
- [x] 2.3 Reuse `scripts/secure_link_auth_lib.py` and `scripts/security_server_sim.py` with tracked `config/security/command-auth.ini`.
- [x] 2.4 Add installed-release provenance gates for `current`, systemd working directory, bundled keystore SHA, manifest SHA, expected service identities, forbidden `COMMAND_AUTH_*` env absence, and forbidden `--command-auth*` CLI absence.
- [x] 2.5 Capture S-band/UHF gateway bytes, target journals, checkpoints, summary JSON, and cleanup status under the proof root.

## 3. Target Proof Cases

- [x] 3.1 Prove target S-band APID `0x00FE` challenge auth and secure command v2 on the command APID.
- [x] 3.2 Prove first accepted S-band secure command may use a non-`1` sequence and later commands require strict next-sequence behavior.
- [x] 3.3 Prove malformed S-band handshake fails closed without auth/session mutation.
- [x] 3.4 Prove target S-band `.sequence-staging/<leaf>` staged upload admission after secure auth.
- [x] 3.5 Prove target UHF `ServiceID = 2` auth on physical node `6`.
- [x] 3.6 Prove `uhf-backup` read/status acceptance, high-authority denial, and staged-upload denial.
- [x] 3.7 Prove switch to `uhf-primary-after-failover` invalidates old UHF auth/session state and requires re-auth before secure-command acceptance.

## 4. Evidence And Docs

- [x] 4.1 Add `docs/test-records/target-secure-auth-proof-v1/README.md`.
- [x] 4.2 Update `docs/verification-path-registry.md` with the target secure-auth proof entry and adjacent-path boundaries.
- [x] 4.3 Update `docs/interfaces.md` secure-auth and uplink-authority target status.
- [x] 4.4 Update `docs/operator/target-obc-comm-csp-lab-runbook.md` with the proof entrypoint and evidence contract.
- [x] 4.5 Clarify `docs/operator/formal-comm-verification-matrix-v1-runbook.md` if touched so older matrix quiet-UHF cells remain distinct from registry entry `69`.

## 5. Verification

- [x] 5.1 Run `bash -n scripts/run_target_secure_auth_proof.sh`.
- [x] 5.2 Run `python -m py_compile scripts/comm_verification/lib/run_target_can_matrix_probe.py scripts/secure_link_auth_lib.py scripts/security_server_sim.py`.
- [x] 5.3 Run focused product unit tests if product code changes. Not required; no product code changed in this verification-first slice.
- [x] 5.4 Run `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`.
- [x] 5.5 Run target preflight `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh`.
- [x] 5.6 Run `bash scripts/run_target_secure_auth_proof.sh`.
- [x] 5.7 Run `openspec validate target-secure-auth-proof-v1` and `openspec validate --specs`.
