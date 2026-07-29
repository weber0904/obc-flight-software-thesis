## Context

`PayloadOpsController` currently accepts capture requests asynchronously at the
driver layer, but it keeps the public capture command pending until capture
finishes and canonical payload `.fdp` publication either succeeds or fails.
That made the hosted payload closure proof easy to script, but it conflated
request acceptance with final delivery completion. The user has now chosen to
make the public capture command behave like a real asynchronous operation:
acceptance should return immediately, while later status/readback surfaces
report progress and final result. This follow-up change also closes the missing
governed target node-`5` payload `.fdp` proof instead of leaving it deferred.

## Goals / Non-Goals

**Goals**

- Keep the existing `PAYLOAD_CAPTURE_*` public command family but change it to
  async-ack semantics.
- Make payload state/readback surfaces the completion oracle for capture and
  canonical `.fdp` publication.
- Add explicit observability for post-capture `.fdp` publication.
- Close the governed target node-`5` payload `.fdp` path with one repo-owned
  wrapper and evidence record.

**Non-Goals**

- No new payload browse/list/select/download command family.
- No rollback to local-files-only payload delivery.
- No payload-family reliable-transfer widening.
- No new scheduler or mission planner surface.
- No reopening of generic security redesign scope.

## Decisions

### Change existing capture commands instead of adding parallel async commands

The user chose to update the existing `PAYLOAD_CAPTURE_*` contract directly.
This avoids splitting the payload surface into two parallel capture models.
The tradeoff is that sequence/probe logic that assumed synchronous capture
completion must now poll payload state or metadata before issuing later steps
such as `PAYLOAD_SHUTDOWN`.

### Separate request acceptance from final completion

Accepted capture requests return `Fw::CmdResponse::OK` immediately after
`PiCameraManager.beginCapture(...)` succeeds and the controller records the
active capture state. Final capture success or failure becomes:

- `PAYLOAD_STATE`
- `PAYLOAD_BUSY`
- `PAYLOAD_STATUS`
- `PAYLOAD_CAPTURE_METADATA`
- `PAYLOAD_LAST_RESULT`
- `PAYLOAD_LAST_DETAIL`
- `PAYLOAD_LAST_DATA_PRODUCT_PUBLISHED`
- `PAYLOAD_LAST_DATA_PRODUCT_BYTES`

### Add explicit publishing state

The current state machine reuses `PSTATE_CAPTURING` while canonical `.fdp`
publication is in progress. This change adds a distinct `PSTATE_PUBLISHING`
state so operators and probes can distinguish:

- request accepted / capture in progress
- JPEG finished / `.fdp` publishing in progress
- final success or failure

### Keep final payload success gated on canonical `.fdp` publication

This change only moves the public command acknowledgement boundary. It does not
change the formal mission-delivery rule that a payload capture is finally
successful only after canonical `.fdp` publication succeeds.

### Replace deferred target boundary with a repo-owned target wrapper

Target proof will reuse the current governed target baseline:

- `ensure_target_comm_lab_baseline.sh`
- default node-`5` S-band target path
- stock `run_target_comm_csp_ground_stack.sh`
- authenticated payload commands on the governed target path
- stock `BUILD_CATALOG` plus `START_XMIT_CATALOG(NO_WAIT)`
- repo-owned `.fdp` decode/JPEG parity tooling

It will not relabel the old Pi-local direct payload target probe as target
official downlink closure.

## Risks / Trade-offs

- [Existing official sequence wrappers assumed synchronous capture completion]
  Update proofs/docs to poll payload state and metadata explicitly before later
  shutdown or verification steps.
- [Async ack can hide later failure unless observability is clear] Add explicit
  `PSTATE_PUBLISHING` and preserve final failure events/tlm/readback.
- [Target proof may expose target timing or path-flake edges] Keep the wrapper
  bounded to node-`5` S-band official delivery only and bind the oracle to the
  exact payload `.fdp` selected for downlink.

## Migration Plan

1. Add the follow-up OpenSpec change and delta specs.
2. Update `PayloadOpsController` state and command-completion logic so accepted
   capture commands respond immediately.
3. Add `PSTATE_PUBLISHING` and update events/tlm/tests/readback semantics.
4. Update hosted probe logic to wait on payload state/readback instead of
   command completion as the final success oracle.
5. Add the governed target node-`5` payload `.fdp` proof wrapper and evidence.
6. Reconcile registry/docs/specs and rerun focused hosted + target proof.

## Open Questions

- None. The command model and target-proof requirement are fixed by user
  direction for this follow-up change.
