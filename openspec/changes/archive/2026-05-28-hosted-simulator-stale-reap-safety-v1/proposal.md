## Why

The maintained hosted per-band stock ground/operator baseline merged with a
real launcher safety gap: EPS and ADCS simulator stale-process matching relied
only on shared `--node-id` fragments, so starting or stopping one launcher
could reap an unrelated active hosted run that happened to use different CSP
hub ports behind the same simulator binaries.

The follow-up review also exposed a smaller documentation problem. Most
current-facing per-band stock-stack wording was synced in the original change,
but a few current/progress documents still lag the merged baseline and the
current target-timing truth.

## What Changes

- Harden the maintained hosted per-band launcher cleanup model so shared EPS
  and ADCS simulator identities are reaped only as orphaned leftovers unless a
  unique ownership marker exists.
- Add a repository-owned hosted proof that the maintained per-band launchers do
  not terminate another active simulator-backed hosted stack running on
  different CSP hub ports.
- Sync the current-facing operator and progress documentation that still lags
  the merged per-band baseline or the current target-timing truth.

## Capabilities

### Modified Capabilities

- `ground-ttc-gateway`: refine the maintained hosted per-band stock-stack
  launcher cleanup boundary and runbook truth
- `verification-evidence`: require bounded hosted proof that launcher cleanup
  does not terminate unrelated active EPS/ADCS simulator runs
- `verification-path-registry`: record the bounded non-interference proof as
  part of the maintained hosted per-band operator-baseline entry

## Impact

- `scripts/probe_process_utils.py`
- `scripts/per_band_stock_ground_stacks.py`
- `scripts/run_per_band_stock_ground_stacks_hosted_probe.sh`
- `evidence/records/hosted-simulator-stale-reap-safety-v1/README.md`
- `docs/operator/hosted-per-band-stock-ground-stacks-runbook.md`
- `docs/roadmap/current-baseline.md`
- `docs/architecture/current-development-architecture.md`
- `evidence/verification-path-registry.md`
- `docs/architecture/comm-followup-directions.md`
- `docs/interfaces.md`
- `docs/architecture/target-flight-design.md`
- `scripts/README.md`
- `README.md`
- `openspec/specs/ground-ttc-gateway/spec.md`
- `openspec/specs/verification-evidence/spec.md`
- `openspec/specs/verification-path-registry/spec.md`
