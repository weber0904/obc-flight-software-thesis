## Why

The active `OBC` / `TopCcsds` verification path repeatedly leaves behind
high-CPU Python helper processes after hosted or Raspberry Pi probes exit
abnormally. The recurring leftovers are not part of the flight runtime
contract, but they poison reruns, consume CPU, and make formal evidence less
trustworthy because successful replay often depends on manual cleanup.

The common failure pattern is narrow and actionable:

- repo-owned Python probes launch `fprime-gds` and related helpers as direct
  child processes without consistent process-group ownership
- shell launchers rely on `jobs -p` cleanup, which only works when the wrapper
  exits normally and still owns the children
- interrupted runs leave orphaned `fprime_gds.executables.comm`,
  `fprime_gds.executables.tcpserver`, or `CustomDataHandlers` helpers behind

This change hardens only the governed active verification path so reruns become
self-healing and do not require manual `kill` cleanup.

## What Changes

- Add a repo-internal Python helper for managed probe subprocess launch,
  process-group teardown, and narrowly-scoped stale helper reaping.
- Refactor the active Python probes that currently do direct-child
  `terminate()` cleanup so they launch local helpers in their own process
  groups and sweep owned stale helpers before startup and during shutdown.
- Harden the active shell stack launchers with targeted stale-process sweeps
  keyed by owned ports, runtime roots, log roots, and exact command fragments.
- Add repository-owned cleanup-hardening proof that interrupts representative
  hosted, shell, and Raspberry Pi probe flows, verifies rerun safety, and
  confirms no owned helper processes remain afterward.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `verification-evidence`: require reviewable proof that governed active probes
  and active stack launchers are rerun-safe after interruption and do not leave
  owned stale helper processes behind.

## Impact

- Affected code:
  - `scripts/_common.sh`
  - `scripts/run_dev_stack.sh`
  - `scripts/run_remote_csp_gds_stack.sh`
  - `scripts/run_rpi_stack.sh`
  - `scripts/run_rpi_installed_stack.sh`
  - active Python probe wrappers that launch `fprime-gds` or stack scripts
  - a new repo-internal Python subprocess helper under `scripts/`
- Public/operator impact:
  - none; this is verification-path and repo-workflow hardening only
- Non-goals:
  - no flight-runtime, topology, or command-wire-format changes
  - no legacy retirement or legacy-only probe cleanup sweep
  - no repo-wide historical process cleanup beyond the governed active path
