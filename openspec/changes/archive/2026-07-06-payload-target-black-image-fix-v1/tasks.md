## 1. Governance

- [x] 1.1 Add OpenSpec proposal, design, and delta specs for target payload black-image closure.
- [x] 1.2 Update payload roadmap/evidence docs so Route 1 downlink closure and target source-image validity stay distinct.

## 2. Target Backend

- [x] 2.1 Add bounded warm-up capture behavior to `LibcameraPiCameraDriver` for target `AUTO` and `DETERMINISTIC` still capture.
- [x] 2.2 Keep current public payload command/FPP surface unchanged while failing closed on warm-up timeout.

## 3. Target Image Sanity Oracle

- [x] 3.1 Add a repo-owned raw-frame luma/stat helper that supports current target pixel formats and rejects near-black source artifacts.
- [x] 3.2 Add focused tests for the luma/stat helper and any extracted warm-up helper logic.

## 4. Target Proof

- [x] 4.1 Add a new governed A/B/C target payload capture sanity proof for VGA `AUTO` and `DETERMINISTIC` cases.
- [x] 4.2 Run fresh target verification against the real camera path and record the resulting onboard source-artifact evidence.

## 5. Validation

- [x] 5.1 Run focused local tests/build checks for the payload backend change.
- [x] 5.2 Update or add the target payload sanity test record plus current-note links from adjacent payload/Chapter 5 records.
