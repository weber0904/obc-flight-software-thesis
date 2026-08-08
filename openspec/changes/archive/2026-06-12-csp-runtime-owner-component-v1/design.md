## Context

The current deployed topology keeps one process-wide libcsp runtime but does not give it one clear owner. `CspBridge`, `GroundLinkDriver`, `CommReliableTransfer`, `EpsBridge`, and `AdcsBridge` can all reach the runtime through direct `defaultRuntime()` binding or constructor-time shared references. That preserves functionality, but it also means steady-state scheduler work and opportunistic service traffic all converge on one runtime without a topology-owned arbitration surface.

The first implementation step should therefore separate ownership from deeper scheduler behavior changes:

1. Make the deployed topology instantiate a formal `CspRuntimeOwner`.
2. Inject that owner-managed runtime into all deployed CSP clients.
3. Preserve existing blocking semantics for command paths while the topology stops using direct global runtime ownership.
4. Only then convert scheduled polling paths to fully async/coalesced owner submissions.

## Goals

- Give the deployed topology one explicit CSP runtime owner.
- Remove deployed direct `defaultRuntime()` binding from COMM, EPS, and ADCS steady-state clients.
- Keep current public component contracts intact.
- Expose owner telemetry/events that keep queueing, timeout, and last-result behavior reviewable.

## Non-Goals

- This change does not fully redesign `PayloadCspService`.
- This first slice does not yet complete the RG1 async polling conversion for `EpsBridge`, `AdcsBridge`, and COMM subsystem health probing.
- This change does not alter command authority, file authority, or ground packet routing policy.

## Architecture

### Owner Boundary

`CspRuntimeOwner` becomes the only deployed object intended to own runtime lifecycle and runtime request serialization. It implements `ICspRuntime` so existing client-side transport helpers can be retargeted with minimal contract churn. Internally it also exposes owner-only queueing and completion helpers for the later async bridge refactor.

### Topology Injection

Deployed components stop assuming `defaultRuntime()` is their flight runtime. Instead:

- `CspBridge` stores a runtime pointer and accepts runtime injection during topology setup.
- `GroundLinkDriver.configureCommCsp(...)` receives `cspRuntimeOwner`.
- `CommReliableTransfer` stores a runtime pointer and is rebound from `CommController`.
- `EpsBridge` and `AdcsBridge` rebuild their owned CSP transports against `cspRuntimeOwner`.

This keeps the current F' component public surfaces stable while moving the deployed runtime boundary under topology control.

### Incremental Bridge Refactor

The second code slice will use the owner's async request path to move scheduled `schedIn` polling off blocking round-trips. This design is intentionally staged so the repository can first prove that runtime ownership has been centralized before changing subsystem poll semantics and freshness policy.

### Target Restart-Recovery Gap

During target secure-auth reruns on node `5`, the owner-refactor branch exposed a separate platform gap that is not the same issue as RG1 blocking CSP polls: an `R2` `PROCESS_RESTART` of `obc-comm-csp-stack.service` could leave `obc.local:can0` wedged even though `subsystem.local` services and CAN interfaces remained healthy. In that failure mode:

- `systemd` correctly restarted `obc-comm-csp-stack.service`
- the OBC child processes came back up
- `obc-lab-can.service` stayed `active (exited)` from the earlier boot
- but the inherited `SocketCAN` path on `obc.local:can0` no longer carried traffic until `obc-lab-can.service` was manually restarted

The deployed target service start path therefore also needs one bounded platform correction: each `obc-comm-csp-stack.service` start or restart shall explicitly re-arm the governed `obc-lab-can.service` before launching the user-space OBC stack. That keeps the target `R2` process-restart path reviewable and prevents the owner-refactor proof from being blocked by stale OBC-side CAN state unrelated to the async polling change itself.

### Current Target Idle-Reboot Blocker

After the OBC-side CAN re-arm fix and after removing stale secure-auth proof
diagnostics defaults from `run_target_secure_auth_proof.sh`, the governed
node-`5` proof on this branch progressed further:

- S-band secure auth established successfully
- the first secure `GET_RESET_CAUSE` command after auth also completed

The current branch is therefore past the original oracle and RG1-sync-poll
hypotheses for that specific path. The remaining blocker observed on
`2026-06-12` is different: target `obc.local` can reboot even when no further
proof traffic is being sent. A passive no-command watch reproduced the same
host-loss and later rebooted uptime window, so the unresolved issue is now
classified as target steady-state instability rather than a remaining
proof-helper mismatch or the earlier shared-CSP scheduled blocking path.

This change does not yet claim a decoded root cause for that reboot behavior.
Current evidence only bounds it as separate from:

- the stale secure-auth proof diagnostics overrides
- the OBC-side `can0` restart-recovery wedge
- the old RG1 synchronous bridge polling model that this owner refactor removed

## Verification Strategy

- Local generate/build must pass with the new owner instance and topology injection.
- Owner-focused unit tests should cover queueing and request completion behavior.
- Existing component tests that rely on direct fake runtimes should continue to work through explicit injection.
- A fresh target node-5 secure-auth rerun remains the required system-level proof once the owner-mediated path is in place.
