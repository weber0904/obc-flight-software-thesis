## 1. Scenario Replay Contract

- [x] 1.1 Define the first repository-owned offline scenario timeline contract and document which scenario fields belong in this slice.
- [x] 1.2 Add the OpenSpec capability and delta specs for the scenario bridge plus EPS/ADCS simulator scenario hooks.

## 2. Simulator Scenario Bridge

- [x] 2.1 Add a host-side scenario timeline loader and replay bridge under `simulators/`.
- [x] 2.2 Extend the EPS simulator model with scenario-driven sunlight and battery SoC inputs.
- [x] 2.3 Extend the ADCS simulator model with scenario-seeded angular-rate initialization that preserves later control-response dynamics.

## 3. Validation And Docs

- [x] 3.1 Add simulator-level tests that verify offline replay updates EPS state and seeds ADCS deployment-rate behavior without overriding later detumble dynamics.
- [x] 3.2 Update simulator-facing docs so the scenario bridge boundary and current scope limits are clear.
- [x] 3.3 Run the relevant local verification commands and capture the results needed for the first scenario-architecture slice.
