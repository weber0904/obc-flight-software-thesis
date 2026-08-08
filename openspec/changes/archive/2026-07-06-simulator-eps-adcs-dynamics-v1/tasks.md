## 1. Governance

- [x] 1.1 Add OpenSpec proposal, design, and EPS/ADCS delta specs for the simulator dynamics change.
- [x] 1.2 Update simulator-facing narrative docs to reflect the new EPS load modes, PDU mapping, ADCS mode dynamics, and control surfaces.

## 2. EPS Simulator

- [x] 2.1 Extend the EPS simulator model with time-driven load evolution, named PDU channel weights, seeded pseudo-noise, and sticky `normal` / `high-draw` modes.
- [x] 2.2 Extend the EPS simulator control socket and helper script with `set-load-mode` while preserving `set-soc` and `drop-status`.
- [x] 2.3 Update EPS simulator tests to cover weighted loads, noise bounds, reproducibility, SoC integration, and the new control command.

## 3. ADCS Simulator

- [x] 3.1 Extend the ADCS simulator model with time-continuous `IDLE`, `DETUMBLE`, and synthetic-pass `POINTING` dynamics.
- [x] 3.2 Extend the ADCS simulator control socket with `restart-pointing-pass` while preserving `drop-state`.
- [x] 3.3 Update ADCS simulator tests to cover jittered idle state, detumble convergence, pointing-pass motion, and the new control command.

## 4. Validation

- [x] 4.1 Run focused simulator tests and any impacted simulator integration checks, then adjust the task list to reflect completed work.
- [x] 4.2 Run `openspec validate simulator-eps-adcs-dynamics-v1` and `openspec validate --specs`.
