# legacy-command-envelope-retirement-v2 Evidence

Date:
- `2026-07-04`

OpenSpec change:
- `legacy-command-envelope-retirement-v2`

## Scope

This record closes the current-baseline retirement slice for legacy external
command-envelope v1 and public `SESSION_OPEN`.

This slice proves:

- current branch-head runtime no longer exposes public
  `OBCApp.commandIngressAuthority.SESSION_OPEN` as a maintained command surface
- legacy command-envelope v1 lifecycle traffic now fails closed before
  dispatch, session mutation, sequence mutation, or legacy persistence
  mutation
- secure-auth still synthesizes the current runtime session evidence used by
  maintained operator surfaces:
  `COMMAND_SESSION_OPENED`, `COMMAND_SESSION_REVOKED`, `SESSION_*`,
  `SECURE_COMMAND_REJECT_*`
- current maintained secure-baseline proofs still pass after the retirement:
  hosted challenge-handshake, hosted uplink-authority, hosted S-band
  observability, target secure-auth command-path, and target secure-auth proof
- current docs/specs/registry now treat legacy external `SESSION_OPEN` and the
  old timing wrappers as historical or retired rather than maintained
  closeout authority

This slice does **not** prove:

- renaming of retained secure-session runtime evidence families
- migration of retired timing wrappers onto a new secure-auth timing harness
- deletion of archived historical evidence
- re-promotion of historical `SESSION_OPEN` wrappers into current baseline

## Qualification Outcome

Re-qualified current maintained secure-baseline gates used for this slice:

- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
- `bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh`
- `bash scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh`
- `bash scripts/run_sband_observability_governance_hosted_probe.sh`
- `bash scripts/run_target_secure_auth_command_path_probe.sh`
- `bash scripts/run_target_secure_auth_proof.sh`

Explicitly not used as current local-ready gates:

- hosted official sequencing / `SystemResources` wrappers
- legacy QoS and older TT&C file/downlink wrappers
- payload compatibility wrappers that still send public `SESSION_OPEN`
- historical beacon/packet-quiet wrappers that still depend on retained legacy
  lifecycle wording
- retired timing wrappers:
  `target_timing_empirical_ceiling_freeze_v1_probe` and
  `target_timing_wcet_profile_proof_v1_probe`

Mission Console remains an adjacent maintained operator surface, but it is not
the governing closeout gate for this retirement slice.

## Runtime Retirement Outcome

Branch-head runtime/code behavior after this slice:

- public `OBCApp.commandIngressAuthority.SESSION_OPEN` has been removed from:
  - current component command surface
  - command authority catalog
  - authority policy JSON
  - current dictionary-facing contract
- `authGranted` still synthesizes the current secure runtime session and emits
  `COMMAND_SESSION_OPENED`
- `authRevoked` still clears that runtime session and emits
  `COMMAND_SESSION_REVOKED`
- legacy command-envelope v1 traffic now rejects with
  `LEGACY_UNSUPPORTED` before runtime mutation
- the old legacy reopen-floor / persistent freshness store is no longer part
  of the maintained runtime contract

## Verification

Fresh local gate:

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

- Result: `PASS`
- Covered shared checks include:
  `01_generate`, `02_build`, `03_generate_ut`, `04_build_ut`,
  `05_check_all`, `06_check_repo_consistency`,
  `07_check_documentation_governance`,
  `08_check_transport_mtu_apid_contract`,
  `09_check_component_test_baseline`,
  `10_check_legacy_zmq_retired`,
  `11_openspec_validate_specs`

Focused touched-path verification:

```bash
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut --output-on-failure -R 'OBC_Components_CommandIngressAuthority_ut_exe|command_authority_catalog_check'
python3 -m py_compile scripts/comm_verification/lib/run_target_can_matrix_probe.py scripts/sband_observability_governance_hosted_probe.py
```

- Result: all `PASS`

## Hosted Maintained Gates

Hosted challenge-handshake secure-command proof:

```bash
bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh
```

- Result: `PASS`
- Proof root:
  `/tmp/challenge-handshake-secure-command-hosted.YIBOcX`
- PASS markers include:
  - `case-sband-secure-auth-eps-status=PASS`
  - `case-uhf-backup-secure-auth-read-continuity-and-high-authority-deny=PASS`
  - `case-uhf-primary-reauth-required-and-high-authority-after-reauth=PASS`
  - `case-secure-auth-inactivity-timeout-clears-session=PASS`

Hosted uplink-authority and key-hardening proof:

```bash
bash scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh
```

- Result: `PASS`
- Proof root:
  `/tmp/uplink-authority-and-key-hardening-hosted.A5lO9H`
- PASS markers include:
  - `case-malformed-handshake-fail-closed=PASS`
  - `case-sband-secure-auth-staged-upload-and-sequence-validate=PASS`
  - `case-uhf-backup-staged-upload-denied=PASS`
  - `case-uhf-failover-primary-reauth-staged-upload-and-sequence-validate=PASS`

Hosted node-`5` S-band observability governance proof:

```bash
bash scripts/run_sband_observability_governance_hosted_probe.sh
```

- Result: `PASS`
- Proof root:
  `/tmp/sband-observability-governance-hosted.qkVGKZ`
- PASS markers:
  - `case-pre-auth-sband-live-quiet=PASS`
  - `case-post-auth-sband-live-open=PASS`
  - `case-post-auth-summary-live-curated=PASS`
  - `case-post-auth-resource-reviewable-surfaces=PASS`
  - `case-eps-get-bounded-detailed-readback=PASS`
  - `case-authenticated-get-reset-cause-summary-readback=PASS`
  - `case-primary-switch-closes-sband-live-observability=PASS`
- Representative bounded detailed readback on the maintained packet path:
  `EPS_GET_STATUS -> EPS_POWER_OUT`

## Target Maintained Gates

Target secure-auth command-path proof:

```bash
bash scripts/run_target_secure_auth_command_path_probe.sh
```

- Result: `PASS`
- Proof root:
  `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.riX335`
- Provenance marker:
  - `case-installed-release-keystore-provenance=PASS`
- PASS markers include:
  - `case-sband-apid-00fe-secure-auth=PASS`
  - `case-sband-secure-command-get-reset-cause-sequence=42`
  - `target-secure-auth-command-path=PASS`
- This rerun also confirmed that baseline manager `A` detected and removed the
  stale `58-sband-ingress-diagnostics.conf` override before the proof was
  accepted.

Target secure-auth proof:

```bash
bash scripts/run_target_secure_auth_proof.sh
```

- Result: `PASS`
- Proof root:
  `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.rUjilp/target-secure-auth-proof-v1`
- Provenance marker:
  - `case-installed-release-keystore-provenance=PASS`
- PASS markers include:
  - `case-sband-malformed-handshake-fail-closed=PASS`
  - `case-sband-apid-00fe-secure-auth=PASS`
  - `case-sband-secure-command-strict-next-sequence=PASS`
  - `case-sband-secure-auth-staged-upload=PASS`
  - `case-uhf-backup-serviceid-2-secure-auth=PASS`
  - `case-uhf-backup-read-status-and-high-authority-deny=PASS`
  - `case-uhf-backup-secure-auth-staged-upload-denied=PASS`
  - `case-uhf-role-switch-invalidates-backup-auth=PASS`
  - `case-old-uhf-backup-auth-state-rejected-after-role-switch=PASS`
  - `case-uhf-primary-after-failover-reauth-secure-command=PASS`
  - `case-restore-sband-primary=PASS`
  - `target-secure-auth-proof=PASS`

## Notes

- During branch-head rerun, hosted S-band observability initially exposed a
  probe bug, not a product regression:
  `scripts/sband_observability_governance_hosted_probe.py` returned an
  undefined `capture_path` variable after the proof logic had already passed.
  That bug was fixed and the governed rerun above passed cleanly.
- The target secure-auth proof emitted post-PASS ignored `openpyxl`/`lxml`
  worksheet-writer exceptions while the wrapper was exiting, but the proof
  itself returned exit code `0` and all governed case markers were already
  recorded as `PASS`. No current failure claim in this slice depends on those
  teardown warnings.
