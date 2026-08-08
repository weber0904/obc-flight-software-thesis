## Context

`mode-model-v2` and `payload-ttc-mode-entry-v1` made `TTC` a first-version operator-enterable mode shell, but the active runtime still has no TTC automation owner. The existing system can report GPS state, COMM link availability, and guarded primary mode transitions, yet no component owns the bounded policy that answers:

- when TTC should start automatically
- when TTC should stop automatically
- how TTC enablement, pass-window truth, GPS time validity, and COMM availability constrain that behavior
- how automatic TTC coexists with the manual mode path without creating split ownership

This change introduces a focused TTC policy owner while preserving the current architecture split:

- `ModeManager` remains the mode state owner
- `ModeSafetyController` remains the SoC-only safety owner
- `CommController` remains the COMM owner
- `RecoveryExecutor` remains the shared recovery owner
- `TtcPassManager` becomes the TTC pass-window automation owner

## Goals / Non-Goals

**Goals:**

- Add one focused runtime owner for TTC pass-window automation.
- Accept a bounded runtime TTC config with `enabled` and `loss_of_lock_timeout_sec`.
- Accept one bounded pass window using `start_unix_sec` and `end_unix_sec`.
- Auto-enter `TTC` from `IDLE` when TTC is enabled, the pass window is active, and GPS time basis is valid.
- Auto-exit `TTC` to `IDLE` when the window ends, the window is cleared, GPS validity is lost, or COMM loss exceeds the configured timeout.
- Keep manual `TTC` entry/exit available while making active `TTC` policy-owned.
- Preserve safety/recovery precedence and avoid any parallel mode store.
- Provide reviewable status/event/telemetry surfaces, focused tests, hosted proof, and canonical documentation updates.

**Non-Goals:**

- No generic scheduler, multi-window queue, time-tagged command engine, or payload scheduling.
- No TLE upload, orbital propagation, or onboard pass prediction engine.
- No ADCS ground target tracking system or pointing-control framework.
- No COMM session redesign, RF behavior claim, or target/Pi deployment closure.
- No revival of `MissionExecutive`.

## Decisions

### Dedicated `TtcPassManager` Owns TTC Pass Policy

Introduce a new real component `TtcPassManager` under `OBC/Components/` as the single owner of TTC pass-window policy.

`TtcPassManager` is responsible for:

- accepting bounded TTC config and one active pass window
- reading current mode, GPS cached state, and COMM availability through narrow provider interfaces
- deciding whether TTC policy wants `TTC` active or inactive
- requesting `ModeManager` transitions only through the existing normal internal mode-control path with an explicit TTC policy source
- reporting bounded TTC policy status for review and probes

Alternatives considered:

- Put TTC policy inside `ModeManager`. Rejected because it would blur mode state ownership with TTC mission policy.
- Put TTC policy inside `ModeSafetyController`. Rejected because it would break the SoC-only safety boundary.
- Reuse `CommController` pass/session commands. Rejected because COMM pass activity is not the owner of TTC mission mode transitions.

### Direct Epoch Window Contract Instead Of TLE Or Split UTC

The v1 pass-window input contract uses one configured window with:

- `start_unix_sec: U64`
- `end_unix_sec: U64`

This contract is deliberately smaller than TLE upload or generic scheduled activity. Ground may derive the window however it wants, but the active runtime only consumes the resulting epoch interval.

`TtcPassManager` rejects inputs fail-closed when:

- `start_unix_sec == 0`
- `end_unix_sec == 0`
- `end_unix_sec <= start_unix_sec`

An explicit clear command removes the configured window.

Alternatives considered:

- Split UTC `date + sec_of_day` window. Rejected because epoch is simpler for runtime policy comparison and aligns better with future scheduler boundaries without adding a scheduler now.
- Onboard TLE parsing or propagation. Rejected because it would materially expand scope and create a half-built scheduler/orbit stack.

### GPS Time Basis Uses Cached GPS State Only

`TtcPassManager` uses `GpsBridge` cached state only. It does not consume scenario truth or any external clock authority directly.

The GPS time basis is valid only when all of the following are true:

- `hasSample == true`
- `fixValid == true`
- `utcDateYmd != 0`
- `utcSecondsOfDay != 0`
- the cached GPS sample is fresh
- `utcDateYmd + utcSecondsOfDay` converts successfully to Unix epoch seconds

Freshness is measured by `acceptedSentenceCount` advancing within a fixed 15-second threshold. `TtcPassManager` uses component `Time` only to timestamp accepted-sentence changes and COMM timeout age; it does not use component time as the authority for pass-window comparison.

If conversion fails, UTC fields are impossible, or freshness cannot be proven, the time basis is invalid and policy fails closed.

### TTC Entry Guard

`TtcPassManager` requests `IDLE -> TTC` only when:

- current mode is `IDLE`
- TTC config `enabled == true`
- a pass window is configured
- GPS time basis is valid
- current GPS-derived epoch time is inside the window using `start <= now < end`

If those conditions become true while current mode is not `IDLE`, v1 does not force any cross-mode takeover. This keeps the change bounded and avoids arguing over PAYLOAD/TTC arbitration or SAFE bypasses.

### TTC Exit Guard

While current mode is `TTC`, `TtcPassManager` requests `TTC -> IDLE` when any of the following becomes true:

- TTC config is disabled
- the pass window is cleared
- the pass window is no longer active
- GPS time basis becomes invalid
- COMM loss-of-lock timeout exceeds the configured threshold

The loss-of-lock proxy is intentionally bounded in v1:

- treat loss-of-lock as `sbandAvailable == false && uhfAvailable == false`
- count timeout only while current mode is `TTC`
- reset the timeout immediately when either link becomes available

This is a COMM availability proxy only. It does not claim RF lock, acquisition, or full pass/session closure.

### Manual TTC And Policy TTC Coexist Through Policy Override

The existing manual `MODE_SET TTC` / hosted `mode ttc` path stays available.

Precedence is:

- `ModeSafetyController` and `RecoveryExecutor` still win first for safety and recovery behavior.
- `TtcPassManager` owns whether `TTC` may remain active under TTC policy.
- Operator entry can place the system into `TTC`, but once current mode is `TTC`, the policy may return it to `IDLE` if TTC conditions are not valid.

Operationally:

- manual `IDLE -> TTC` remains allowed through the current guarded operator path
- manual `TTC -> IDLE` remains allowed and clears active TTC policy state such as loss timer bookkeeping
- manual `TTC -> SAFE` remains allowed and safety/recovery exits also clear TTC policy state
- if an operator manually enters `TTC` while TTC is disabled, no window exists, GPS is invalid, or COMM timeout expires, `TtcPassManager` requests `IDLE` on the next policy cycle

This keeps one mode owner and one TTC policy owner without inventing separate manual-vs-auto TTC states.

### TTC Policy Uses Normal Mode Infrastructure With A Distinct Source

Extend the internal mode source enum with a TTC policy source value, for example `TtcPassPolicy`.

`TtcPassManager` uses the normal internal mode apply path with that source. It does not mutate the mode directly and does not keep a shadow current-mode store.

If another owner has already moved the system out of `TTC`, `TtcPassManager` observes the new current mode and clears its internal policy bookkeeping. Safety/recovery therefore remains authoritative.

### ADCS Hook Is Explicitly Deferred

This change does not send any ADCS pointing request. That keeps the PR bounded and avoids claiming a ground-tracking system or a half-built pointing framework.

### Scheduling Boundary Stays Explicit

This change manages one active pass window only. It does not accept multiple windows, future queued activities, command payloads, or generalized time-based action execution.

Future `time-tagged-command-scheduler-v1` work remains separate and may later consume similar epoch truth, but this PR does not create scheduler scaffolding or scheduler ownership.

## Risks / Trade-offs

- [Risk] GPS cached-state freshness is inferred from accepted sentence progress rather than an explicit provider timestamp. -> Mitigation: keep the rule simple, bounded, and fail-closed; document that richer time-validity metadata is future work.
- [Risk] Manual `TTC` entry can now self-exit quickly when policy conditions are invalid. -> Mitigation: make precedence explicit in the design, tests, shell output, status telemetry, and evidence.
- [Risk] COMM availability is only a proxy for loss-of-lock. -> Mitigation: bound the claim carefully, use clear naming in status/evidence, and defer real RF/lock semantics.
- [Risk] Adding a new internal mode source touches existing mode/recovery tests. -> Mitigation: keep the new source narrow, review existing source assertions, and extend focused tests instead of broadening semantics.
