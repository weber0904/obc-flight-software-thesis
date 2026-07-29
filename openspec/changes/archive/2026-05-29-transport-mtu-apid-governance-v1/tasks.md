## 1. Change Setup

- [x] 1.1 Add proposal, design, tasks, and spec deltas for `transport-mtu-apid-governance-v1`.

## 2. Transport And APID Contract

- [x] 2.1 Freeze the current `sband-primary` and `uhf-backup` command-ingress ceilings with explicit budget breakdowns.
- [x] 2.2 Freeze the current `uhf-primary-after-failover` official file/downlink per-packet ceiling with explicit source derivation.
- [x] 2.3 Freeze the current `ComCfg.Apid` reservation / proof-split governance and document change-control for future APID claims.
- [x] 2.4 Document that the `1024` CCSDS frame size and the `160`-byte reliable-transfer segment ceiling are bounded facts with narrower scope than a repo-wide MTU claim.

## 3. Docs, Evidence, And Checker

- [x] 3.1 Update `docs/interfaces.md` and the related current architecture / roadmap notes to reflect the frozen contract and residuals.
- [x] 3.2 Update the relevant main specs and change delta specs for transport-ceiling and APID-governance truth.
- [x] 3.3 Add a repo-owned checker for the transport ceilings, APID map, and interface-doc drift.
- [x] 3.4 Add a checked-in evidence record that separates numeric derivation, reused path-scope proof, and remaining residuals.
- [x] 3.5 Audit `.codex/skills/change-closeout/SKILL.md` and record whether a clarification was actually needed.

## 4. Validation

- [x] 4.1 Run the checker syntax and execution locally.
- [x] 4.2 Run `python3 scripts/check_repo_consistency.py` and `python3 scripts/check_documentation_governance.py`.
- [x] 4.3 Run `openspec validate transport-mtu-apid-governance-v1` and `openspec validate --specs`.
