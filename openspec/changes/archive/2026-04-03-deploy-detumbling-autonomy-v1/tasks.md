## 1. MissionExecutive Scope

- [x] 1.1 Add the OpenSpec delta specs for the first deployment-style detumbling autonomy case.
- [x] 1.2 Define the high-rate trigger, detumble command behavior, and policy ordering relative to the existing low-battery slice.

## 2. Core Implementation

- [x] 2.1 Extend `MissionExecutive` runtime bindings so it can inspect cached ADCS state and command `DETUMBLE`.
- [x] 2.2 Implement the high-angular-rate autonomy response and preserve deterministic behavior when low-battery and detumbling policies could overlap.
- [x] 2.3 Add or update automated tests covering detumble triggering and convergence under scenario-seeded initial rates.

## 3. Validation And Evidence

- [x] 3.1 Update repo-local evidence and user-facing docs for the first detumbling autonomy slice.
- [x] 3.2 Run the relevant local verification commands and validate OpenSpec artifacts/specs.
- [x] 3.3 Archive the change after the implementation and evidence are complete.
