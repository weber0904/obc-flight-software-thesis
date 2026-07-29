## 1. OpenSpec Governance

- [x] 1.1 Create the governed change artifacts for `legacy-top-retirement-docs-reorg-v1`.
- [x] 1.2 Update affected delta specs for platform baseline, comm subsystem, core contracts, verification evidence, verification path registry, and planning docs.
- [x] 1.3 Validate the change and current specs before closeout.
- [x] 1.4 Archive the change after implementation and validation.

## 2. Legacy Top Runtime Retirement

- [x] 2.1 Delete `OBC/Top/` and `OBC/MainComFprimeLegacy.cpp`.
- [x] 2.2 Remove `OBC_ComFprimeLegacy`, `OBC_Top`, and `OBC/Top` from maintained build registration.
- [x] 2.3 Remove `OBCAppComFprimeLegacy.*` command-policy entries and keep active `OBCApp.*` entries.
- [x] 2.4 Update verification inventory helper paths to active `OBC/TopCcsds` sources.

## 3. Script Cleanup

- [x] 3.1 Delete ComFprime-only launch and probe wrappers whose only purpose is legacy regression.
- [x] 3.2 Port current operator/lab scripts that still use legacy dictionary names or command prefixes to the active `OBC` dictionary and `OBCApp.*` namespace.
- [x] 3.3 Refresh `scripts/README.md` around active hosted, active target/lab, historical evidence, and retired scripts.

## 4. Documentation Reorganization

- [x] 4.1 Move repo-tracked thesis content from `docs/reporting/thesis/` to `docs/thesis/`.
- [x] 4.2 Collapse roadmap into `README.md`, `current-baseline.md`, `next-work.md`, and `archive/completed-through-2026-05-17.md`.
- [x] 4.3 Remove retired redirect folders/files and update repo links.
- [x] 4.4 Refresh current architecture, target design, interfaces, README, AGENTS if needed, roadmap, thesis, and reporting indexes.

## 5. Evidence And Verification

- [x] 5.1 Add `evidence/records/legacy-top-retirement-docs-reorg-v1/README.md`.
- [x] 5.2 Run forbidden-reference checks and repo consistency checks.
- [x] 5.3 Run active deployment generation, build, unit-test generation, unit-test build, and full verification CI.
- [x] 5.4 Run focused hosted CCSDS S-band and UHF adoption probes after the fresh build.
- [x] 5.5 Record verification results and prepare a reviewable commit.
