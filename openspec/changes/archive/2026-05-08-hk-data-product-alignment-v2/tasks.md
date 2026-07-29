## 1. OpenSpec And Contracts

- [x] 1.1 Create proposal, design, tasks, and delta specs for V4 HK data-product alignment.
- [x] 1.2 Validate the active change before implementation proceeds.

## 2. HK Trend V4 Payload

- [x] 2.1 Extend `StateSnapshot` and default/legacy topology snapshot sources with existing cached-gap fields.
- [x] 2.2 Migrate `HkTrendRecord` to V4 while preserving product record name/id.
- [x] 2.3 Map V4 fields from cached snapshot data and keep missing-source validity explicit.
- [x] 2.4 Update `HkTrendProductProducer` classic UT coverage for V4 version, field mapping, and missing-source behavior.

## 3. Hosted CCSDS S-band FDP Parity

- [x] 3.1 Add a repository-owned hosted probe that runs `.fdp` byte-match/decode over the default CCSDS S-band node-5 path.
- [x] 3.2 Ensure the probe asserts generated `.fdp`, `DpCatalog` build/xmit, received byte match, V4 decode, and fallback-boundary exclusions.

## 4. Evidence And Registry

- [x] 4.1 Add `evidence/records/hk-data-product-alignment-v2/README.md` with focused tests, probe evidence, exclusions, and path selection.
- [x] 4.2 Update `evidence/verification-path-registry.md` for the default hosted CCSDS S-band official `.fdp` parity path.
- [x] 4.3 Update main specs through OpenSpec archive and post-archive reconciliation.

## 5. Verification And Closeout

- [x] 5.1 Run focused UTs for HK trend producer, onboard state data/source, and affected catalog/probe helpers.
- [x] 5.2 Run fresh build/UT gate and the focused CCSDS S-band `.fdp` parity probe after the fresh build.
- [x] 5.3 Run `openspec validate hk-data-product-alignment-v2` and `openspec validate --specs`.
- [x] 5.4 Archive the change, update local untracked `pending/` handoff notes, and prepare a reviewable local branch without pushing.
