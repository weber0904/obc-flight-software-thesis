## Context

`mode-model-v2` established the active `SatMode` vocabulary and left `PAYLOAD` and `TTC` as commandable shells without subsystem behavior. `mode-safety-policy-v1` then added `ModeSafetyController` as the cached-EPS SoC fallback owner for autonomous `SAFE -> HELL`, `HELL -> SAFE`, and active-mode `-> SAFE` transitions.

The current runtime still lets `ModeManager.MODE_SET` and the hosted shell set any parsed `SatMode` directly through `setModeForRuntime`. That is too permissive for `HELL`, and it lets operators jump directly into `PAYLOAD` or `TTC` from unsafe states. This change replaces the arbitrary operator setter with a first-version transition policy while preserving the official F Prime separation between command dispatch and component-owned policy: command infrastructure routes commands and reports status; `ModeManager` and `ModeSafetyController` own mode state and protection decisions.

## Goals / Non-Goals

**Goals:**

- Guard every operator-requested transition from `MODE_SET` and hosted `mode <...>` with the same v1 matrix.
- Keep `ModeSafetyController` as the owner of cached-EPS SoC protection decisions, including the operator `HELL -> SAFE` recovery guard.
- Preserve the existing `SYS_MODE_CHANGE(mode)` public event shape and add a deterministic rejection event.
- Harden the internal safety apply path so it is visibly separate from operator requests.
- Provide focused component/helper tests, hosted shell regression evidence, and default hosted CCSDS S-band command/event/telemetry evidence.

**Non-Goals:**

- No SoC admission threshold for `SAFE -> IDLE`, `IDLE -> PAYLOAD`, or `IDLE -> TTC`.
- No TLE/GPS/pass-window TTC scheduler, ADCS ground tracking, payload/camera control, payload power sequencing, link authority/auth/session policy, UHF failover, CCSDS route change, HK data-product field change, watchdog, subsystem timeout, retry, reset, or broader FDIR behavior.
- No revival of the retired provisional `MissionExecutive` low-power, sun-safe, or detumble policy.

## Decisions

### Operator Requests Use A Guarded ModeManager Surface

`ModeManager` remains the public mode command/telemetry/event owner. It will expose an operator request helper returning `Fw::CmdResponse`, and `MODE_SET_cmdHandler` plus hosted runtime services will call that helper. This avoids duplicating matrix logic in the hosted shell while keeping command routing separate from mission policy.

Alternatives considered:

- Put transition checks in the hosted parser only. Rejected because F Prime `MODE_SET` would still bypass the policy.
- Put all policy inside `ModeManager`. Rejected for SoC-dependent recovery because cached EPS fallback ownership already belongs to `ModeSafetyController`.

### ModeSafetyController Implements The Narrow Guard Interface

Add a narrow transition guard interface in the `ModeSafetyController` runtime boundary. `ModeManager` calls this interface for operator requests. `ModeSafetyController` makes only the decisions that depend on the safety policy and cached EPS availability. Matrix-only rejections can be decided in the same guard helper so the reason mapping is single-source and testable.

The interface returns a decision containing accepted/rejected status and a stable `U32` reason code for rejected transitions. If `ModeManager` has no guard configured, it rejects operator requests with `GUARD_UNCONFIGURED` and `EXECUTION_ERROR`.

### Operator Matrix

The v1 operator matrix is:

| From \ To | SAFE | IDLE | PAYLOAD | TTC | HELL |
|---|---:|---:|---:|---:|---:|
| SAFE | no-op | allow | reject | reject | reject |
| IDLE | allow | no-op | allow | allow | reject |
| PAYLOAD | allow | allow | no-op | reject | reject |
| TTC | allow | allow | reject | no-op | reject |
| HELL | guarded allow | reject | reject | reject | no-op |

`HELL -> SAFE` is accepted only when cached EPS SoC is strictly greater than `15%`. It is rejected when EPS cache is unavailable or SoC is `<= 15%`.

### Cached EPS Semantics

The guard uses the current repository EPS cache contract:

- SoC is `F32` percent from `OBC::EPS::StatusData::soc`.
- EPS cache is available when `IModeSafetyEpsStatus::getCachedStatusForRuntime(...)` returns `true`.
- Uninitialized cache and stale cache after a failed later EPS poll are unavailable, matching the existing `mission-autonomy` spec.
- This change does not add timestamp-based freshness because the current cache interface has no timestamp.
- This change does not add new EPS numeric range validation; validity remains an EPS-provider responsibility unless a later change creates a formal status-validity contract.

### Response, Event, And Telemetry Semantics

Accepted mode-changing operator transitions update the runtime mode, emit exactly one existing `SYS_MODE_CHANGE(mode)` event, eventually publish `SYS_MODE` with the target mode, and return `OK`.

Same-mode operator requests return `OK`, leave the mode unchanged, emit zero `SYS_MODE_CHANGE` events, emit zero rejection events, and may publish the unchanged state through the normal `ModeManager` state publication path.

Rejected operator transitions leave the mode unchanged, return `VALIDATION_ERROR` except for guard-unconfigured wiring failure, emit exactly one `SYS_MODE_TRANSITION_REJECTED(fromMode, toMode, reasonCode)`, emit zero `SYS_MODE_CHANGE` events, and do not publish `SYS_MODE` with the rejected target.

Guard-unconfigured requests are rejected with reason `GUARD_UNCONFIGURED` and command response `EXECUTION_ERROR` because that is a runtime wiring/configuration failure rather than an invalid operator transition.

Invalid F Prime enum payloads are rejected by generated deserialization before `ModeManager.MODE_SET_cmdHandler` runs and therefore produce `FORMAT_ERROR` without transition events.

### Rejection Reason Codes

The public rejection reason is a stable `U32` event argument:

| Code | Name |
|---:|---|
| 1 | `DISALLOWED_OPERATOR_TRANSITION` |
| 2 | `INTERNAL_ONLY_TARGET` |
| 3 | `SOC_RECOVERY_GUARD_UNAVAILABLE` |
| 4 | `SOC_RECOVERY_GUARD_NOT_MET` |
| 5 | `GUARD_UNCONFIGURED` |

Use named constants in code and tests; do not scatter magic numbers.

### Internal Apply API

Rename the neutral `setModeForRuntime` mode-control method to an explicit internal-source apply method such as `applyModeForInternalSource(mode, source)`. The source enum includes `SafetyFallback`, `SafetyRecovery`, and `TestSetup`.

`ModeSafetyController` uses `SafetyFallback` for `SAFE -> HELL` and active-mode `-> SAFE`, and `SafetyRecovery` for autonomous `HELL -> SAFE`. Operator paths must not call this internal apply method. Production code must not use `TestSetup`; that source is for focused tests or fixtures where a current mode must be arranged without pretending an operator command did it.

### Hosted Shell Parser

The hosted parser keeps exact lowercase canonical inputs: `safe`, `idle`, `payload`, `ttc`, and `hell`. The parser maps `hell` to `SatMode::HELL`; the guard then rejects ordinary operator entry or accepts same-mode no-op/recovery according to the current mode.

Retired spellings `nominal`, `low-power`, `debug`, and `update`, plus unknown and mixed-case spellings, remain parser errors. Parser errors do not emit transition rejection events because no valid `SatMode` target reached the transition guard.

### Verification Split

Hosted shell regression proves parser behavior and that the shell uses the guarded operator request path. Default hosted CCSDS S-band evidence proves the F Prime command path using existing registry entry 42, with `fprime-cli` sending bounded `MODE_SET` commands and observing command responses, events, and final `SYS_MODE` telemetry over the CCSDS S-band path.

## Risks / Trade-offs

- [Risk] Guard and mode-control interfaces create a bidirectional runtime relationship between `ModeManager` and `ModeSafetyController`. -> Mitigation: use narrow non-owning interfaces configured during topology setup; keep the guard method side-effect-free except for reading cached EPS through the existing provider.
- [Risk] Existing probes use arbitrary `mode hell` or direct `SAFE -> PAYLOAD` setup. -> Mitigation: update probes to use valid operator sequences or explicit internal/test setup paths, and document the breaking behavior in evidence.
- [Risk] Hosted CCSDS command/event/telemetry observations can arrive out of order. -> Mitigation: probes assert final state and event/response presence, not transport-layer ordering.
- [Risk] `HELL -> SAFE` operator recovery may race with autonomous safety recovery in a full hosted runtime. -> Mitigation: exact manual `HELL -> SAFE` semantics are covered in component tests; hosted safety evidence continues to prove autonomous recovery.
