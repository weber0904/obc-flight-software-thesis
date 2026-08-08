## Why

The source debug branch already isolated a real target-side node-`5`
secure-auth RG3 blocker: observability work in `rateGroup3` could stall behind
shared internal CSP runtime traffic. Before the larger CSP runtime-owner
refactor starts, the bounded product fix for that proven RG3 blocker should be
salvaged onto a clean branch and carried through the formal workflow.

This salvage change is intentionally narrow. It preserves only the mainline
product fixes and the minimum branch-verifiable evidence needed to support the
claim that the reproduced RG3 blocker was closed. Investigation-only
diagnostics, temporary probe surfaces, cadence knobs, and dirty submodule
patches stay out of the mainline change.

## What Changes

- Change `LibCspRuntime::metrics()` so observability-only runtime metrics reads
  can reuse a cached snapshot instead of waiting behind blocking shared-runtime
  traffic.
- Change `CommController` runtime-state publishing so runtime observers read a
  cached `CommRuntimeState` snapshot instead of rebuilding it on every read.
- Add a narrowed evidence record and OpenSpec artifacts for this bounded RG3
  closure, while explicitly keeping remaining RG1 contention as separate
  follow-up work.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: the maintained internal CSP runtime baseline now keeps
  observability-only runtime metrics reads bounded even when a blocking CSP
  operation currently owns the shared runtime mutex.
- `verification-evidence`: the repository records the salvaged RG3 blocker
  classification, bounded fix, and clean-branch verification record under
  `evidence/records/target-node5-rg3-csp-runtime-contention-fix-v1/`.

## Impact

- Affected code:
  - `simulators/csp/CspRuntime.*`
  - `OBC/Components/CommController/*`
- Affected docs:
  - `evidence/records/target-node5-rg3-csp-runtime-contention-fix-v1/README.md`
  - `evidence/verification-path-registry.md`
- Intended non-claims:
  - no scheduler redesign
  - no migration to the future `CspRuntimeOwner` architecture in this change
  - no claim that RG1 contention is solved here
  - no carry-forward of debug-only diagnostics or dirty `lib/fprime` changes
