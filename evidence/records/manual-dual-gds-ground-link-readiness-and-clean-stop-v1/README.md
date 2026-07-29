# Manual Dual-GDS Ground-Link Readiness And Clean Stop V1

Status: branch-scoped evidence for manual dual-GDS readiness/cleanup semantics.
Date: 2026-06-12.

## Scope

This record captures the bounded fix that separates:

- target baseline readiness
- manual ground-surface link attachment
- manual surface stop cleanup

It does not claim any COMM auth-path redesign.

## Findings

- `GROUND_LINK_UP` on target node `5` is not an auth event.
- `GROUND_LINK_UP` appears only after a ground-side TCP client actually attaches
  to the subsystem S-band listener.
- `scripts/ensure_target_comm_lab_baseline.sh` should not gate readiness on
  `GROUND_LINK_UP`, because layer `A` does not start that client.
- `scripts/manual_ops/target/stop_target_manual_ground_surface.sh` previously
  leaked child helpers after the owner pid exited.

## Direct Evidence

Using the formal target manual dual-GDS order:

1. `bash scripts/manual_ops/target/start_target_manual_baseline.sh`
2. `GDS_UI_MODE=headless bash scripts/manual_ops/target/start_target_manual_ground_surface.sh`
3. no auth

Observed during step 2:

- subsystem node `5` TCP listener gained an established client session on
  `:18520`
- target OBC journal emitted fresh
  `groundLinkDriver) GROUND_LINK_UP : Ground link connected mode 2`

This confirms that `GROUND_LINK_UP` belongs to the ground-surface layer, not
the baseline layer.

Observed during stop before this fix:

- owner pid could already be gone
- `status.json` still said `running`
- helper residue remained:
  - `fprime-gds`
  - `fprime_gds.executables.comm`
  - `fprime_gds.executables.tcpserver`
  - `CustomDataHandlers`
  - `ground_ttc_gateway`

## Resulting Contract

- `A = ensure_target_comm_lab_baseline.sh`
  now uses target-local OBC markers such as node-`5` CSP ping continuity; it
  no longer treats
  `GROUND_LINK_UP` as a blocker.
- target manual baseline preflight waits for target-local readiness markers,
  not `GROUND_LINK_UP`.
- target manual ground-surface startup waits for fresh target
  `groundLinkDriver) GROUND_LINK_UP` after it starts the local dual-GDS client
  path.
- hosted manual surface readiness likewise waits for fresh hosted
  `GROUND_LINK_UP`.
- hosted/target stop wrappers now terminate the owner and then reap manifest-
  keyed helper residue.
