## 1. OpenSpec And Governance Alignment

- [x] 1.1 Add the verification-path-registry delta spec for historical-wrapper hardening.
- [x] 1.2 Update current registry/runbook wording so the affected wrappers are described with the same script-entrypoint behavior.

## 2. Wrapper Hardening

- [x] 2.1 Harden `run_official_sequencing_system_resources_v1_probe.sh` so it requires explicit `ALLOW_HISTORICAL_WRAPPER=1`.
- [x] 2.2 Harden the retired timing wrapper entrypoints so they fail closed with clear redirect messaging.

## 3. Verification And Evidence

- [x] 3.1 Run repo-local validation for the new change and verify the hardened wrappers now emit the expected refusal behavior.
- [x] 3.2 Record the branch-head evidence in a dedicated test record.
