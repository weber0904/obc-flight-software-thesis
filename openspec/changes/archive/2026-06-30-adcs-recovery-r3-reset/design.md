## Context

EPS already implements a bounded `R3_RESET_SUBSYSTEM_INTERFACE` action through
an owned CSP reset service, a transport `reset(...)` API, simulator-side reset
semantics, and an `EpsBridge` recovery-control surface consumed by
`RecoveryExecutor`. ADCS currently differs at every layer: `AdcsSimModel` has a
local `reset()` helper, but there is no ADCS CSP reset service, no transport
entry point, no bridge recovery-control surface, and no shared recovery action
other than the current R2 process restart.

The active recovery architecture already reserves `R3_RESET_SUBSYSTEM_INTERFACE`
for subsystem-owned reset behavior and keeps relatch escalation above the first
action. This change aligns ADCS with that existing architecture without
renaming recovery levels, broadening recovery into a generic framework, or
changing the higher-level relatch path.

## Goals / Non-Goals

**Goals**

- Make ADCS reset reachable over its owned CSP request/reply path.
- Make `AdcsBridge` provide a recovery-only reset surface that reuses its
  normal cache/apply behavior.
- Make the first ADCS shared scheduled-poll recovery action a real ADCS reset
  at `R3_RESET_SUBSYSTEM_INTERFACE`.
- Keep relatch escalation and current reboot/process-restart semantics for
  higher levels unchanged.
- Add focused validation that proves request/reply, state restoration, recovery
  mapping, and hosted recovery behavior.

**Non-Goals**

- No new public flight command such as `ADCS_RESET`.
- No OBC reboot, full power cycle, subsystem Pi service restart, or generic
  multi-subsystem abstraction.
- No BootManager persistence on the first ADCS reset-only incident.
- No thesis-file edits.

## Decisions

- ADCS reset uses CSP service port `24`, preserving the existing `20..23`
  assignments and avoiding libcsp reserved low ports.
- The ADCS reset service returns the standard ADCS `StateData` reply payload so
  `decodeAdcsReply(...)` and bridge apply logic stay uniform.
- `AdcsSimModel::processCspRequest()` handles reset by calling the same default
  loading path used by the local `reset()` helper so mode, quaternion, target,
  rates, sensor-valid state, and derived fields return to one deterministic
  baseline.
- `RecoveryRuntime.hpp` gains a minimal `IRecoveryAdcsControl` interface with
  `resetAdcsForRecovery(OBC::ADCS::StateData&)`. `RecoveryExecutor` depends on
  this interface directly rather than on `AdcsBridge`.
- `RecoveryExecutor` adds an ADCS reset external action and counts ADCS reset
  executions with the existing `RECOVERY_TOTAL_RESET_ACTIONS` counter used by
  EPS reset.
- `ADCS_POLL_TRANSPORT` and `ADCS_POLL_FRESHNESS` move from initial level `R2`
  to initial level `R3`. On first open they queue ADCS reset; when boot
  safe-fallback clamp is active, they still queue the reset first and safe
  fallback second so the first action remains the ADCS reset.
- Relatch escalation remains unchanged: once the ADCS incident reopens after a
  clear, the executor may still escalate to the existing reboot path instead of
  repeating indefinite reset loops.

## Risks / Trade-offs

- Hosted recovery evidence and target-recovery documentation currently describe
  ADCS as an R2 process-restart source. They must be updated carefully so the
  repo no longer claims the old first-fault behavior on the active baseline.
- Reusing the normal `StateData` reply keeps the protocol small but means reset
  success must be validated through full default-state assertions rather than a
  unique reset acknowledgment field.
- Wiring `RecoveryExecutor::configureRuntime(...)` to accept one more control
  dependency changes topology and tests, but the impact is bounded to the known
  call sites and keeps the runtime contract explicit.
