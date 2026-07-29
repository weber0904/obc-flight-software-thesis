# historical-wrapper-hardening-v1 Evidence

Date:
- `2026-07-04`

OpenSpec change:
- `historical-wrapper-hardening-v1`

## Scope

This slice hardens the entrypoints of already-demoted legacy/timing wrappers so
developers do not accidentally use them as if they were current maintained
proof surfaces.

This slice proves:

- the hosted official sequencing wrapper now self-identifies as supplemental
  historical and refuses unless explicitly opt-in enabled
- the retired target timing wrapper entrypoints now fail closed immediately
- current registry/runbook/current-baseline wording matches the new script
  entrypoint behavior

This slice does **not** prove:

- requalification of any historical wrapper back into the maintained gate set
- migration of retired timing probes onto a new secure-auth timing harness
- deletion of archived historical evidence

## Verification

OpenSpec validation:

```bash
openspec validate historical-wrapper-hardening-v1
openspec validate --specs
```

- Result: `PASS`

Archive and reconciliation:

```bash
openspec archive historical-wrapper-hardening-v1 --yes
python3 scripts/generate_reconciliation_matrix_md.py
python3 scripts/check_repo_consistency.py
```

- Result: all `PASS`
- Archive location:
  `openspec/changes/archive/2026-07-04-historical-wrapper-hardening-v1/`

Wrapper hardening behavior:

```bash
bash scripts/run_official_sequencing_system_resources_v1_probe.sh
bash scripts/run_target_timing_empirical_ceiling_freeze_v1_probe.sh
bash scripts/run_target_timing_wcet_profile_proof_v1_probe.sh
python3 scripts/target_timing_empirical_ceiling_freeze_v1_probe.py
python3 scripts/target_timing_wcet_profile_proof_v1_probe.py
```

- Result: all returned bounded refusal messages
- Expected behavior:
  - official sequencing wrapper requests explicit
    `ALLOW_HISTORICAL_WRAPPER=1`
  - retired timing wrappers fail closed and redirect to current
    registry/current-baseline authority

Repo-local consistency:

```bash
python3 scripts/check_repo_consistency.py
```

- Result: `PASS`
