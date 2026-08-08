## Why

Current governance documents already classify several legacy or timing probe
wrappers as supplemental historical or retired, but the script entrypoints
themselves still look runnable. That leaves a practical footgun: a developer can
skip the registry/runbook layer, execute a nearby script from `scripts/`, and
mistake a historical wrapper for a current maintained proof surface.

## What Changes

- Add explicit retired/historical hardening at script entrypoints for the known
  legacy command-envelope and retired timing wrapper family.
- Make retired timing wrappers fail closed immediately with a clear redirect to
  the governing registry/runbook surface.
- Make supplemental historical wrappers require an explicit
  `ALLOW_HISTORICAL_WRAPPER=1` override before they will run.
- Update registry and operator documentation so the script-level behavior and
  governance wording match.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `verification-path-registry`: historical or retired wrapper families now need
  self-identifying script entrypoints that default to fail-closed or explicit
  operator opt-in.

## Impact

- Affected code: selected legacy/timing scripts under `scripts/`
- Affected docs: `evidence/verification-path-registry.md`,
  `docs/operator/hosted-official-sequencing-system-resources-runbook.md`,
  roadmap/current-baseline references for wrapper status
- Affected workflow: developers get an immediate script-level warning instead of
  relying only on registry/runbook reading
