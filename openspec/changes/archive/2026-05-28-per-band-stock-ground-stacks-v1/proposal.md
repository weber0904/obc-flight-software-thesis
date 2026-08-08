## Why

The current baseline already freezes the near-term answer for simultaneous
operator access before any higher-level orchestration exists: use separate
stock `fprime-gds` plus `ground_ttc_gateway` stacks per band. That truth is
present in current specs and recent evidence, but it is still not a maintained
repository workflow or a clear current operator entrypoint.

This change is needed now because the repository should stop treating that
dual-surface hosted operator model as an implied or probe-only convention.
Reviewers and later changes need a maintained hosted-first baseline they can
run, cite, and build on without over-claiming one-GDS aggregation, one-gateway
multiplexing, or target-bearing simultaneous dual-link closure.

## What Changes

- Add a maintained hosted-first operator baseline with one shared hosted
  `TopCcsds` runtime and two distinct stock ground/operator surfaces:
  - S-band stock `fprime-gds` + `ground_ttc_gateway` + node `5`
  - UHF stock `fprime-gds` + `ground_ttc_gateway` + node `6`
- Add maintained launcher entrypoints for:
  - S-band-only hosted stock stack
  - UHF-only hosted stock stack
  - combined hosted wrapper that composes both maintained stock stacks without
    becoming a policy/orchestration owner
- Add a dedicated hosted operator runbook that records ports, runtime roots,
  logs, startup order, shutdown/cleanup, stack roles, and explicit non-claims.
- Add a hosted-first repository-owned proof and registry entry for the new
  maintained operator baseline while reusing existing S-band and UHF transport
  and policy evidence for the already proven per-band paths.
- Sync current-facing docs so the current baseline clearly states:
  - 2026-05-28 UHF egress narrowing and packet-quiet versus beacon-suppress
    split are already current truth
  - near-term simultaneous multi-band operator truth is separate per-band stock
    stacks
  - `comm-dual-link-orchestration-v1` remains future work layered on top of
    this maintained baseline

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `ground-ttc-gateway`: add hosted maintained per-band stock-stack launcher and
  runbook requirements, and make the maintained operator boundary explicit.
- `verification-evidence`: require hosted-first maintained operator-baseline
  evidence with explicit reused prerequisites and explicit simultaneous
  non-claims.
- `verification-path-registry`: add a dedicated hosted per-band stock-stack
  operator-baseline registry path instead of relying only on adjacent per-band
  path entries.

## Impact

- Affected code:
  - new maintained launcher wrappers and one shared hosted process manager
  - extracted dual-surface hosted startup logic currently embedded in probe-only
    topology code
  - one hosted-first proof wrapper for the maintained operator baseline
- Affected docs/specs:
  - OpenSpec change artifacts and the three modified main specs above
  - current baseline, roadmap, interface/follow-up wording, scripts inventory,
    README, verification registry, and a new hosted operator runbook
- Affected operations:
  - hosted near-term simultaneous multi-band access becomes a maintained
    repo-owned workflow with two distinct stock stacks
  - this change does not claim one-GDS heterogeneous upstream handling, one
    gateway multiplexer behavior, target/lab simultaneous closure, RF closure,
    or future orchestration completion
