## Context

The current hosted simulator layer is deterministic but largely self-contained: EPS keeps its own sunlight and battery state, and ADCS starts from fixed angular-rate defaults. That is enough for subsystem bring-up, but not for the next extension phase where the project wants to validate scenario-driven behaviors such as low-battery entry, sunlit versus eclipse transitions, deployment-rate detumbling, and later ground-pass-aware operations.

The team has already aligned on several constraints:

- start with offline scenario replay rather than a live STK feed
- keep STK outside the OBC runtime and route scenario truth through a scenario bridge that drives simulators
- preserve the current direct TCP to `fprime-gds` baseline
- avoid mixing this slice with MissionExecutive, housekeeping/downlink, or new RF/vendor integration

This design therefore adds the minimum architectural layer needed to replay mission context into the existing hosted simulators while keeping later autonomy logic separate.

## Goals / Non-Goals

**Goals:**

- define a repository-owned offline scenario timeline format suitable for replay from pre-exported scenario data
- add a host-side scenario bridge that replays timeline samples into the EPS and ADCS simulator models
- make EPS responsive to scenario-driven sunlight and battery SoC so low-battery cases can be staged without inventing the full autonomy stack first
- make ADCS accept scenario-seeded angular-rate initial conditions while preserving the simulator's own control-responsive detumble and pointing behavior afterward
- capture ground-pass and link-availability fields in the bridge state now so later comm/autonomy slices can reuse the same timeline contract

**Non-Goals:**

- direct STK to OBC integration
- MissionExecutive or any other autonomy manager
- housekeeping logging/downlink, `Data Products`, or `FileDownlink`
- real RF effects, vendor-specific radio control, or `csp-es` migration
- a closed-loop high-fidelity dynamics model for EPS energy flow or ADCS attitude propagation

## Decisions

### Decision: Use a repository-owned CSV timeline for the first replay contract

The first replay slice uses a simple CSV file with explicit columns for replay time, sunlight, battery SoC, ground-pass-open, link-availability, and angular-rate seeds. STK or other tools can export into this format offline, but the repository-owned contract remains stable even if upstream tooling changes.

Alternative considered:

- consume STK-native exports directly
  - rejected because it would couple the repository to an external tool format before the project has validated the internal replay boundary

### Decision: Keep the scenario bridge on the host simulator side, not inside the OBC runtime

The bridge belongs with the hosted simulator layer because its job is to translate scenario truth into simulator inputs. This keeps the flight software runtime focused on subsystem behavior and preserves the agreed boundary that STK or scenario truth should not feed the OBC directly.

Alternative considered:

- add a scenario-consumer component inside `OBC/`
  - rejected because it would blur the line between mission context generation and flight software behavior under test

### Decision: Replay EPS state continuously but seed ADCS rates once

The current EPS simulator does not yet implement a richer battery-energy model, so replayed sunlight and battery SoC are treated as exogenous timeline inputs and may update over time. ADCS is different: the project wants to seed deployment-like initial angular rates, but once seeded, the simulator must remain responsive to OBC mode commands such as `DETUMBLE` and `POINTING`. The bridge therefore applies ADCS angular rates during initialization only and does not continually overwrite them on later timeline steps.

Alternative considered:

- continuously replay ADCS angular rates from the scenario timeline
  - rejected because it would hide or defeat later control-response validation

### Decision: Use zero-order hold for the first replay step model

Timeline lookup returns the last sample at or before the requested replay time. This is simple, deterministic, and appropriate for the first version because many fields are discrete mission-context states rather than continuously interpolated sensor truth. If later scenario work needs interpolation, it can extend the bridge without breaking the initial contract.

Alternative considered:

- interpolate every numeric field
  - rejected because it adds complexity and is unnecessary for the first simulator-architecture slice

## Risks / Trade-offs

- [Risk] A simple CSV replay contract may be too narrow for later richer mission scenarios. → Mitigation: keep the bridge and timeline types explicit and easy to extend while starting from a minimal, reviewable format now.
- [Risk] Scenario-driven battery SoC updates may look more authoritative than the current simplified EPS energy model deserves. → Mitigation: document that SoC replay is an exogenous validation aid for the early SIL architecture, not a high-fidelity battery model.
- [Risk] Future readers may assume ground-pass and link-availability fields are already wired into comm or autonomy behavior. → Mitigation: expose and test those fields in the bridge state, but keep specs and docs explicit that later consumers remain out of scope.
- [Risk] Later autonomy work may need different timing semantics than zero-order hold. → Mitigation: isolate timeline lookup behind the bridge so interpolation or richer event semantics can be added later without rewiring the simulator interfaces.

## Migration Plan

1. Add the new scenario capability artifacts and repository-owned replay contract.
2. Introduce the scenario timeline and bridge module under `simulators/`.
3. Extend EPS and ADCS simulator models with explicit scenario hooks.
4. Add simulator-level integration tests and update simulator docs.
5. Use this bridge as the input boundary for later autonomy-focused changes instead of feeding STK directly into the OBC runtime.

## Open Questions

- When the project starts the first autonomy-focused change, should ground-pass and link-availability drive a dedicated comm simulator hook first, or only a mission-level executive decision layer?
- When STK preprocessing is introduced, should the repository also own a converter tool, or only the bridge-side replay contract?
