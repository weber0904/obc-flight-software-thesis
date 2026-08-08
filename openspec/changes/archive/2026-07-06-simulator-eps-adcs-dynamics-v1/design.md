## Context

The current hosted EPS and ADCS simulators are intentionally simple, but their fixed-value outputs now undercut the repo's demo and operator-observability goals. The active OBC baseline already depends on simulator-owned external control sockets for EPS runtime SoC stimulation and ADCS reply-drop testing, so the cleanest extension point is to keep the new demo controls simulator-owned and leave `EpsBridge`, `AdcsBridge`, and OBC public commands unchanged.

The repository also treats simulator behavior as governed specification surface. This means the new "random-looking" curves cannot be truly nondeterministic; they must remain seeded and time-driven so tests, proofs, and screenshots can be reproduced.

## Goals / Non-Goals

**Goals:**

- Make EPS telemetry evolve over monotonic time instead of staying flat.
- Make ADCS mode behavior visually distinct across `IDLE`, `DETUMBLE`, and `POINTING`.
- Preserve current CSP request/reply payloads and current OBC public command ownership.
- Keep the new curves deterministic enough for reviewable testing.

**Non-Goals:**

- No real orbital, sunlight, RF, or ground-station geometry model.
- No new OBC runtime owner for subsystem power semantics.
- No new profile-selection framework or generalized simulator scripting API.
- No change to `TtcPassManager`, `ModeSafetyController`, `EpsBridge`, or `AdcsBridge` public behavior beyond accepting changing values.

## Decisions

### Keep simulator ownership for demo controls

The new controls stay on the existing Unix-domain control sockets rather than becoming new OBC commands. This matches the existing EPS SoC-control precedent and avoids teaching flight software about demo-only power modes or ADCS replay controls.

Alternative considered: add new `EPS_*` / `ADCS_*` commands. Rejected because it would formalize simulator-only controls into the flight-software contract.

### Use time-driven seeded pseudo-noise

Both simulators will derive bounded noise from monotonic time buckets and fixed internal seeds. This keeps plots non-flat while remaining reproducible across test runs and independent of poll frequency.

Alternative considered: true random noise. Rejected because it would make the simulator harder to test and would conflict with the repository's deterministic-review bias.

### Model EPS as weighted named loads plus a sticky overlay mode

EPS keeps the existing `pdu_status` bitmask, but specific channels now map to named subsystem weights. `high-draw` is implemented as a sticky overlay on top of the current PDU-derived base load instead of being auto-followed by payload mode or PDU changes.

Alternative considered: make only payload channel `3` affect power. Rejected because the user explicitly wants subsystem binding and the simulator already exposes an 8-channel PDU model.

### Model ADCS pointing as a synthetic repeating pass

`POINTING` will follow a 60-second synthetic roll/pitch/yaw sweep converted to a target quaternion, and the current state will first-order track that desired attitude. This produces plausible trend lines without introducing real target geometry or full waypoint orchestration.

Alternative considered: use `ADCS_SET_TARGET` as the full pointing-profile owner. Rejected because it would expand scope into profile APIs and would complicate the TTC-driven best-effort `POINTING` flow.

## Risks / Trade-offs

- [Risk] Existing tests may assume exact EPS/ADCS values. → Mitigation: convert simulator tests to range/trend assertions and keep fixed reset invariants where practical.
- [Risk] Time-driven models can become request-rate dependent. → Mitigation: advance both simulators from monotonic wall-clock deltas and derive noise from quantized time buckets instead of per-request stepping.
- [Risk] EPS SoC ramps and dynamic load integration may fight each other. → Mitigation: keep the existing lazy target-ramp state but apply load-driven integration after ramp resolution on each update cycle.
- [Risk] ADCS pointing error may no longer converge under the old integration oracle. → Mitigation: update ADCS integration tests to assert motion and bounded error behavior rather than strict convergence to one static quaternion.

## Migration Plan

1. Create the OpenSpec delta specs and task list for EPS and ADCS simulator behavior changes.
2. Implement the EPS model/control updates and adjust simulator tests to the new bounded trend behavior.
3. Implement the ADCS model/control updates and adjust simulator tests to the new time-continuous mode behavior.
4. Update simulator narrative docs and validate the change with focused simulator tests plus `openspec validate`.

Rollback is straightforward: revert the simulator model/control patches and restore the prior exact-value test assertions.

## Open Questions

None for v1. The fixed PDU mapping, load constants, control names, and pointing-pass duration were decided during planning.
