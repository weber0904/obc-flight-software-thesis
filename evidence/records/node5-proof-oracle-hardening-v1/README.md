# node5-proof-oracle-hardening-v1 Evidence

Status: fresh follow-up closeout record for the maintained proof-oracle
hardening layered on top of archived `node5-observability-residual-cleanup-v1`.

## Reused Path Identities

This change hardens already-registered proof families. It does not introduce a
new operator path, command plane, or observability bucket.

- Hosted node-`5` observability governance path:
  registry entry `43G`
- Service-managed target node-`5` observability governance path:
  registry entry `70A`
- Service-managed target secure-auth proof path:
  registry entry `70`

## What Was Hardened

The follow-up closes two proof-integrity bugs on maintained paths:

- hosted and target node-`5` observability proofs now keep bounded detailed
  readback and switch-close tied to packet-path evidence, not only passive
  listener quiet
- target secure-auth now tracks handshake progress across maintained sources
  and tolerates one bounded late-challenge arrival on UHF primary re-auth
  instead of sending duplicate `REQ_AUTH` retries

The implemented target secure-auth fix is narrower than any product redesign:

- S-band secure-auth waits prefer the maintained native packet log when it is
  the source that actually advanced
- when UHF primary re-auth shows
  `SECURE_AUTH_CHALLENGE_ISSUED ingress 1 service 2` in the target journal but
  the packet-path challenge artifact arrives late, the probe waits one bounded
  grace window before retrying
- this avoids proof-created APID `0x00FE` sequence churn on the maintained UHF
  uplink path and preserves the existing secure-auth/runtime contract

## Fresh Verification

Fresh local gate:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

- Result: `PASS`
- Notable covered steps:
  `01_generate`, `02_build`, `03_generate_ut`, `04_build_ut`,
  `05_check_all`, `06_check_repo_consistency`,
  `07_check_documentation_governance`,
  `08_check_transport_mtu_apid_contract`,
  `09_check_component_test_baseline`,
  `10_check_legacy_zmq_retired`,
  `11_openspec_validate_specs`

Fresh hosted node-`5` observability proof:

```bash
bash scripts/run_sband_observability_governance_hosted_probe.sh
```

- Result: `PASS`
- Proof root:
  `/tmp/sband-observability-governance-hosted.bk3mQ2`
- Governed verdicts:
  `case-pre-auth-sband-live-quiet=PASS`
  `case-post-auth-sband-live-open=PASS`
  `case-post-auth-summary-live-curated=PASS`
  `case-post-auth-resource-reviewable-surfaces=PASS`
  `case-eps-get-bounded-detailed-readback=PASS`
  `case-authenticated-get-reset-cause-summary-readback=PASS`
  `case-primary-switch-closes-sband-live-observability=PASS`

Fresh target node-`5` observability proof:

```bash
bash scripts/run_target_sband_observability_governance_probe.sh
```

- Result: `PASS`
- Proof root:
  `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-sband-observability-governance.K0YDR3`
- Governed verdicts:
  `case-pre-auth-sband-live-quiet=PASS`
  `case-post-auth-sband-live-open=PASS`
  `case-post-auth-summary-live-curated=PASS`
  `case-post-auth-resource-reviewable-surfaces=PASS`
  `case-eps-get-bounded-detailed-readback=PASS`
  `case-authenticated-get-reset-cause-summary-readback=PASS`
  `case-primary-switch-closes-sband-live-observability=PASS`
  `case-restore-sband-primary=PASS`

Fresh target secure-auth proof:

```bash
bash scripts/run_target_secure_auth_proof.sh
```

- Result: `PASS`
- Proof root:
  `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.uMu6hU/target-secure-auth-proof-v1`
- Governed verdicts include:
  `case-installed-release-keystore-provenance=PASS`
  `case-sband-malformed-handshake-fail-closed=PASS`
  `case-sband-apid-00fe-secure-auth=PASS`
  `case-sband-secure-command-strict-next-sequence=PASS`
  `case-sband-secure-auth-staged-upload=PASS`
  `case-uhf-backup-serviceid-2-secure-auth=PASS`
  `case-uhf-role-switch-invalidates-backup-auth=PASS`
  `case-old-uhf-backup-auth-state-rejected-after-role-switch=PASS`
  `case-uhf-primary-after-failover-reauth-secure-command=PASS`
  `case-restore-sband-primary=PASS`
  `target-secure-auth-proof=PASS`

Focused touched tests rerun on the maintained branch:

```bash
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_GroundLinkHealthProvider_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_WatchdogSupervisor_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test
```

- Result: all `PASS`

## Key Evidence Notes

- The maintained hosted and target observability claims still use the same
  residual-governance boundary:
  curated post-auth live summary plus one representative bounded detailed
  readback `EPS_GET_STATUS -> EPS_IBAT`
- The maintained target secure-auth proof now records which surface confirmed
  each handshake step. Fresh passing evidence used native packet-log
  confirmation on the S-band handshake and bounded late-challenge grace on the
  UHF primary re-auth step
- The failing intermediate target diagnosis showed UHF uplink APID `0x00FE`
  sequence gaps during duplicate proof-side `REQ_AUTH` retries; that was a
  proof/oracle bug, not evidence of broken auth math or a generic packet
  format regression
- The recurring `openpyxl` teardown warnings after the target secure-auth
  proof remain non-oracle cleanup noise; they do not alter the governed PASS
  verdicts above
