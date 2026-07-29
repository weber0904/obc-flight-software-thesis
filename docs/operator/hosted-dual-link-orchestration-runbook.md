# Hosted Dual-Link Orchestration Runbook

Status: current hosted operator runbook.
Last reconciled against current non-quiet UHF-primary baseline wording on
2026-06-25.

This runbook covers the hosted-only layer-2 orchestration owner above the
maintained per-band stock ground/operator baseline.

The three layers are:

1. layer-1 maintained per-band stock stacks
2. layer-2 hosted thin lifecycle owner
3. separate current exact target-bearing dual-verdict proof family

This runbook covers only layer 2.

## Scope

This runbook covers:

- one hosted-only thin lifecycle owner for the combined dual-link hosted
  surface
- the owner manifest and status artifacts
- the owner lifecycle phases and failure summary
- the owner cleanup summary for declared hosted listeners
- the hosted-first repository-owned proof for this orchestration owner

This runbook does **not** cover:

- per-band TT&C semantics themselves
- command authority or session-policy ownership
- gateway relay ownership or one-gateway multiplexer behavior
- stock-GDS plugin behavior
- target-bearing simultaneous proof
- RF closure

## Preconditions

- repository built locally from `${REPO_ROOT}`
- `fprime-gds`, `fprime-cli`, and Python available from `fprime-venv/bin`
- current `OBC` dictionary present under the build artifacts
- current native hosted binaries present under `build-artifacts/Darwin/bin`

If build outputs are missing, run:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build
```

## 1. Start The Layer-2 Owner

Recommended entrypoint:

```bash
DUAL_LINK_ORCHESTRATION_ROOT=/tmp/dual-link-owner \
DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT=/tmp/dual-link-runtime \
PER_BAND_AUTO_PORTS=1 \
bash scripts/run_hosted_dual_link_orchestration.sh
```

Recommended knobs:

- `DUAL_LINK_ORCHESTRATION_ROOT=<path>`: owned layer-2 manifest, status, and
  delegated layer-1 artifacts
- `DUAL_LINK_ORCHESTRATION_RUNTIME_ROOT=<path>`: requested shared hosted
  runtime namespace root for the delegated layer-1 baseline
- `PER_BAND_AUTO_PORTS=1`: allocate fresh hosted ports for the delegated
  layer-1 surface
- `ORCHESTRATION_HOLD_SECS=<n>`: hold the owner open for a bounded number of
  seconds before automatic teardown

## 2. Interpret The Owner Boundary

The layer-2 owner writes:

- `<owner-root>/manifest.json`
- `<owner-root>/status.json`
- `<owner-root>/layer1-baseline/manifest.json`

Review these fields first:

- `ownerType`
- `ownedTransitions`
- `nonOwnedAdjacentState`
- `failureBehavior`
- `cleanupBehavior`
- `sharedRuntimeInteraction`
- `status.json:lifecyclePhase`
- `status.json:phaseHistory`
- `status.json:ownedProcessSet`
- `status.json:sharedRuntimeState`
- `status.json:cleanup.listenerChecks`

Current owner meaning:

- lifecycle owner for the hosted combined operator surface only
- explicit delegated dependence on `43B`
- no command authority ownership
- no gateway multiplexing
- no COMM runtime ownership inside OBC

## 3. Read The Three-Layer Truth Correctly

- Layer 1:
  - two stock GDS processes
  - two gateway processes
  - separate southbound paths
  - one shared hosted runtime
  - composition-only combined wrapper
- Layer 2:
  - hosted-only thin lifecycle owner
  - authoritative hosted lifecycle/failure/cleanup state for the combined
    operator surface
  - delegated per-band semantics
- Layer 3:
  - target-bearing dual-link proof is now a separate current proof family
  - the current exact family is documented in the target/lab runbook and
    verification registry, including maintained non-quiet UHF-primary and
    autonomous failover truth

## 4. Use The Hosted Proof

Repository-owned proof:

```bash
bash scripts/run_dual_link_orchestration_hosted_probe.sh
```

That proof verifies:

- happy-path lifecycle: `preflight -> starting -> ready -> stopping -> stopped`
- bounded startup-conflict classification as `startup_failed`
- cleanup summary comes from orchestration-owned artifacts
- orchestration acceptance does not depend on old COMM semantic oracles

Adjacent proof ownership remains separate:

- `bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh`
- `bash scripts/run_ccsds_sband_hosted_adoption_probe.sh`
  - retained historical hosted wrapper only; not a current maintained
    secure-auth gate
- `bash scripts/run_uhf_beacon_suppression_hosted_probe.sh`
  - retained historical hosted wrapper only; not a current maintained
    secure-auth gate
- `bash scripts/run_uhf_primary_packet_quiet_hosted_probe.sh`
  - retained historical hosted proof only; not a current maintained operator
    baseline gate

## 5. Truthful Limits

- This owner is hosted-only.
- It is a thin lifecycle owner, not a command/runtime owner.
- It is not a gateway multiplexer.
- It is not a stock-GDS plugin.
- It is not a target-bearing simultaneous proof surface.
- It does not itself serve as the node-`5` plus node-`6` dual-verdict
  target-bearing proof surface; that proof family is separate and already
  governed by its own exact target evidence.
