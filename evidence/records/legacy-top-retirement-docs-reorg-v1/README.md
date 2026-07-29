# legacy-top-retirement-docs-reorg-v1 Evidence

Status: local verification complete for hosted/build gates; target operator
probe attempted with a lab-link limitation noted below.
Change: `legacy-top-retirement-docs-reorg-v1`.
Archived change: `2026-05-16-legacy-top-retirement-docs-reorg-v1`.
Branch: `feature/legacy-top-retirement-docs-reorg-v1`.
Base commit: `75a85677`.

## Scope

This record covers hard retirement of the legacy Top / ComFprime OBC deployment
from the maintained build, script, policy, current documentation, and
verification inventory surface. It also covers the repository documentation
information-architecture cleanup and repo-tracked thesis refresh.

Historical evidence records may still mention the retired names. Those records
are preserved as point-in-time evidence and are not rerun expectations for the
current baseline.

## Implementation Summary

- Removed legacy OBC topology source and deployment registration.
- Moved `OnboardStateSnapshotSource` unit-test ownership to `TopCcsds`.
- Removed retired ComFprime-only launch/probe wrappers.
- Ported current target COMM CSP lab scripts to the active `OBC` dictionary and
  `OBCApp.*` namespace.
- Removed retired command-policy entries for legacy command names.
- Collapsed roadmap into current baseline, next-work, and historical archive
  files.
- Moved repo-tracked thesis content to `docs/thesis/` and updated R2 restart
  wording.
- Registered this cleanup boundary in the verification path registry.

## Verification

```bash
rg -n "OBC_ComFprimeLegacy|OBCAppComFprimeLegacy|OBC/Top\\b|run_comfprime_legacy" \
  CMakeLists.txt OBC scripts README.md AGENTS.md docs openspec/specs obc-dev-spec \
  --glob '!evidence/records/**' \
  --glob '!docs/architecture-review/archive/**' \
  --glob '!docs/reporting/archive/**' \
  --glob '!openspec/changes/archive/**'
python3 scripts/check_repo_consistency.py
python3 scripts/check_component_test_baseline.py
python3 scripts/check_legacy_zmq_retired.py
openspec validate legacy-top-retirement-docs-reorg-v1
openspec validate --specs
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut
bash scripts/run_verification_ci.sh build-artifacts/legacy-top-retirement-docs-reorg-v1-local
bash scripts/run_ccsds_sband_hosted_adoption_probe.sh
bash scripts/run_uhf_ccsds_hosted_adoption_probe.sh
```

## Results

- `python3 scripts/check_repo_consistency.py`: PASS.
- `python3 scripts/check_component_test_baseline.py`: PASS.
- `python3 scripts/check_legacy_zmq_retired.py`: PASS.
- `bash -n` on the updated target and hosted probe wrappers: PASS.
- `openspec validate legacy-top-retirement-docs-reorg-v1`: PASS.
- `openspec validate --specs`: PASS.
- `openspec archive legacy-top-retirement-docs-reorg-v1 -y`: PASS;
  archived as `2026-05-16-legacy-top-retirement-docs-reorg-v1`.
- Post-archive `python3 scripts/check_repo_consistency.py`: PASS.
- Post-archive `openspec validate --specs`: PASS.
- `bash scripts/run_verification_ci.sh build-artifacts/legacy-top-retirement-docs-reorg-v1-local`: PASS.
  The final run passed generate, build, UT generation, UT build, `check_all`,
  repo consistency, component-test baseline, legacy-ZMQ retirement, and
  OpenSpec specs validation.
- `bash scripts/run_ccsds_sband_hosted_adoption_probe.sh`: PASS.
  Latest log directory: `/tmp/obc-ccsds-sband-adoption.qkKvyK`.
- `bash scripts/run_uhf_ccsds_hosted_adoption_probe.sh`: PASS.
  Latest log directory: `/tmp/obc-ccsds-uhf-adoption.JMo8I9`.

The forbidden-reference sweep was used as an active-reference check. Remaining
matches outside archived evidence are intentional retired-path warnings in
current docs/specs, not maintained runtime, build, script, or command-policy
entrypoints.

## Target Operator Probe Attempt

`bash scripts/run_target_comm_csp_lab_operational_probe.sh` was ported to the
active `OBC` dictionary, `OBCApp.*` command namespace, CCSDS S-band framing, and
authenticated command envelopes for restricted active commands.

The final target attempt reached the active installed OBC service and verified
that the current startup journal fragment is `OBC CCSDS S-band runtime started`.
It opened an authenticated command session and observed active command dispatch
for EPS/ADCS command paths. The run did not finish with PASS because
`OBCApp.gpsBridge.GPS_ACCEPTED_SENTENCES` did not advance within 90 seconds
near the end of the probe:

```text
RuntimeError: OBCApp.gpsBridge.GPS_ACCEPTED_SENTENCES did not advance within 90s
```

Evidence directory: `/tmp/target-obc-comm-csp-lab-operational.Ei6mO8`.
The same run recorded repeated UART checksum/sequence warnings and
`ComCcsds.comQueue.QueueOverflow` events, so this result is recorded as a lab
link/GPS-source limitation rather than as evidence that legacy Top is still
required. No legacy dictionary or `OBCAppComFprimeLegacy.*` command path was
used by the updated script.
