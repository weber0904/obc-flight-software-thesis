## 1. Inventory Tool Repair

- [x] 1.1 Update `scripts/report_verification_inventory.py` so it classifies real F' components separately from helper/support modules.
- [x] 1.2 Make the report expose whether each real component has classic F' L2 harness coverage.
- [x] 1.3 Make the report expose which helper/support modules keep direct L1 tests and which owning capability or component they support.

## 2. Matrix And Entry Docs

- [x] 2.1 Rewrite `docs/verification-matrix.md` to use explicit component-versus-helper reporting.
- [x] 2.2 Remove the misleading early/late framing and replace it with the corrected drift explanation.
- [x] 2.3 Update any related entry docs or evidence indices that still describe the old mixed classification.

## 3. Evidence And Validation

- [x] 3.1 Add a verification record summarizing the repaired reporting model and any remaining genuine gaps.
- [x] 3.2 Run `python3 scripts/report_verification_inventory.py` and `python3 scripts/report_verification_inventory.py --json`.
- [x] 3.3 Run `openspec validate verification-matrix-repair-v1` and `openspec validate --specs`.

## 4. Finalization

- [x] 4.1 Archive `verification-matrix-repair-v1` after the report and docs align with the new component/helper split.
- [x] 4.2 Prepare the change for governed closeout on the dedicated feature branch.
