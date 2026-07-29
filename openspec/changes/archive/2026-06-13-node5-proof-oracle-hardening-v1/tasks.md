## 1. Artifact And Spec Updates

- [x] 1.1 Add the follow-up proof-hardening proposal, design, and tasks artifacts for `node5-proof-oracle-hardening-v1`.
- [x] 1.2 Update the delta specs for `verification-evidence` and `verification-path-registry` so the maintained node-`5` observability and target secure-auth paths explicitly require packet-path-or-source-aware review surfaces rather than observer-only quiet.
- [x] 1.3 Add a dedicated follow-up evidence record under `docs/test-records/node5-proof-oracle-hardening-v1/README.md` that cites the reused hosted and target path identities plus the new hardening verdicts.

## 2. Probe And Helper Implementation

- [x] 2.1 Harden the hosted node-`5` observability proof so representative bounded detailed readback and S-band close both require gateway/downlink capture quiet in addition to passive observer behavior.
- [x] 2.2 Harden the target node-`5` observability proof with the same packet-path quiet requirement.
- [x] 2.3 Replace the target secure-auth handshake watcher's single preferred-source count with source-aware progress tracking across wire capture and native packet logs.
- [x] 2.4 Keep the path identities, operator-governance buckets, and secure-auth product contract unchanged while implementing the stronger proof oracle.

## 3. Verification And Closeout

- [x] 3.1 Run the fresh local verification gate:
  `PATH="$PWD/fprime-venv/bin:$PATH" bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
- [x] 3.2 Rerun the maintained hosted and target node-`5` observability proofs on the fresh build.
  - `bash scripts/run_sband_observability_governance_hosted_probe.sh` -> `PASS`
    (`/tmp/sband-observability-governance-hosted.bk3mQ2`)
  - `bash scripts/run_target_sband_observability_governance_probe.sh` -> `PASS`
    (`/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.K0YDR3`)
- [x] 3.3 Rerun the maintained target secure-auth proof on the fresh build.
  - `bash scripts/run_target_secure_auth_proof.sh` -> `PASS`
    (`/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.uMu6hU/target-secure-auth-proof-v1`)
- [x] 3.4 Run `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate node5-proof-oracle-hardening-v1` and `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs`.
  - `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate node5-proof-oracle-hardening-v1` -> `Change 'node5-proof-oracle-hardening-v1' is valid`
  - `PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs` -> `30 passed, 0 failed`
- [x] Focused touched tests rerun on the maintained branch.
  - `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_GroundLinkHealthProvider_ut_exe` -> `PASS`
  - `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe` -> `PASS`
  - `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_WatchdogSupervisor_ut_exe` -> `PASS`
  - `./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test` -> `PASS`
- [x] 3.5 Archive `node5-proof-oracle-hardening-v1`, update `docs/baseline-reconciliation-matrix.json`, regenerate `docs/baseline-reconciliation-matrix.md`, rerun repo consistency/documentation governance checks, and leave the branch local-ready before push.
  - `PATH="$PWD/fprime-venv/bin:$PATH" openspec archive node5-proof-oracle-hardening-v1 --yes` -> archived as `2026-06-13-node5-proof-oracle-hardening-v1`
  - `python3 scripts/generate_reconciliation_matrix_md.py` -> `updated docs/baseline-reconciliation-matrix.md`
  - `python3 scripts/check_repo_consistency.py` -> `PASS`
  - `python3 scripts/check_documentation_governance.py` -> `PASS`
- [x] 3.6 Push the review-ready branch and open a non-draft PR once local-ready is complete.
  - `git push origin fix/node5-proof-oracle-hardening-v1` -> pushed commit `5f7c26ebc6829bdb5ffe6d2b53b67b14aefaf0b5`
  - non-draft PR opened: `https://github.com/weber0904/obc-flight-software/pull/141`
