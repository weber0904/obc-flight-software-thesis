## Context

The earlier scenario-driven simulator work already introduced scenario-seeded ADCS initial angular rates specifically so deployment-style cases could be validated later. The first low-battery autonomy slice then introduced a minimal `MissionExecutive` with narrow runtime bindings to existing subsystem implementations. The next natural extension is to reuse that mission-autonomy boundary for the other scenario discussed in the notes: a deployment-like high-rate condition that should cause the OBC to command `DETUMBLE`.

This slice should remain intentionally small. The notes describe a richer future in which the software tracks deployment phase, records detumble completion, and transitions into the next mission phase. That is useful future work, but the immediate value is to prove that the existing hosted ADCS model, scenario seed path, and mission executive can already close the loop on the first high-rate response.

## Goals / Non-Goals

**Goals:**

- extend `MissionExecutive` with a first high-angular-rate detumbling response
- consume cached ADCS state through a narrow runtime interface rather than adding a larger port redesign
- command `DETUMBLE` when the cached ADCS angular-rate norm exceeds the existing mission detumble threshold
- verify locally that scenario-seeded initial rates trigger detumble autonomy and converge below threshold
- preserve the existing low-battery autonomy behavior without regressing it

**Non-Goals:**

- introducing a deployment-phase enum or mission-phase state machine
- automatic post-detumble transition into pointing, nominal, or payload operations
- new Pi-only probes or hardware-in-the-loop evidence
- extending comm, housekeeping, or ground-pass automation

## Decisions

### Decision: Reuse the current MissionExecutive instead of adding a separate deploy-phase controller

The current repository already has a dedicated system-level autonomy boundary. Adding the first detumbling case there keeps policy centralized and avoids fragmenting mission logic across multiple small managers.

Alternative considered:

- introduce a separate deployment/autonomy component
  - rejected because it would duplicate the policy boundary established in the previous change before that boundary has proven inadequate

### Decision: Trigger detumble from cached ADCS state, not from a separate deployment flag

The hosted validation infrastructure already gives the project the key observable needed for this slice: a scenario-seeded initial angular-rate state. Using the ADCS angular-rate norm directly proves the autonomy response without first inventing a deployment-phase source that the repository does not yet own.

Alternative considered:

- add a dedicated deployment flag to the scenario contract and mission executive first
  - rejected because it broadens this slice beyond the smallest useful detumbling case

### Decision: Preserve low-battery pointing and high-rate detumble through simple policy ordering

High-rate detumbling and low-battery pointing could otherwise issue conflicting ADCS mode commands. For this slice, `MissionExecutive` will treat high-rate detumbling as the higher-priority ADCS behavior: while the ADCS state still indicates detumble is needed, the mission layer should command `DETUMBLE` and suppress the low-battery sun-safe pointing command. Once the rate norm falls below threshold, low-battery behavior may proceed again if it is still active.

Alternative considered:

- let low-battery pointing and detumble issue independently
  - rejected because it can cause avoidable command conflicts in the exact scenario this slice introduces

### Decision: Keep detumble completion local to autonomy state and existing ADCS evidence

The ADCS bridge already emits `ADCS_DETUMBLE_COMPLETE` when the simulator rate norm falls below the mission threshold in `DETUMBLE`. This slice will rely on that existing subsystem evidence and mission-executive state rather than inventing a new top-level mission-phase completion event.

Alternative considered:

- add a dedicated mission-level detumble-complete event immediately
  - rejected because the subsystem already owns the concrete completion signal and a mission-phase event would be premature in this narrow slice

## Risks / Trade-offs

- [Risk] A direct rate-norm trigger is simpler than a true deployment phase model. -> Mitigation: keep the spec explicit that this slice proves the first detumble response only, not full deployment sequencing.
- [Risk] Policy ordering between detumble and low-battery pointing can be surprising. -> Mitigation: document that high-rate detumbling has priority while the rate norm remains above threshold.
- [Risk] Cached ADCS state may lag one scheduler cycle behind the newest simulator state. -> Mitigation: use the same cached-state pattern already accepted for low-battery autonomy and validate through hosted integration tests.

## Migration Plan

1. Extend the mission-autonomy delta spec with the first high-rate detumble response.
2. Add narrow runtime hooks for cached ADCS status access and detumble command control.
3. Add policy tests plus a hosted scenario-driven detumbling autonomy integration test.
4. Update evidence, validate locally, and archive the change.
