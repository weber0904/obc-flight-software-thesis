## 1. Hosted Deployment Promotion

- [x] 1.1 Promote the CCSDS topology to default `OBC` with `OBCApp.*` namespace.
- [x] 1.2 Rename the old `ComFprime` topology to `OBCAppComFprimeLegacy.*`.
- [x] 1.3 Register `OBC_ComFprimeLegacy` and stop using spike naming as a formal deployment.
- [x] 1.4 Preserve shared `OBC_Runtime` command dispatch across default and legacy hosted deployments.

## 2. Script And Probe Defaults

- [x] 2.1 Make dictionary discovery deployment-aware and update GDS helpers.
- [x] 2.2 Make no-override hosted dev/GDS scripts use CCSDS S-band node `5`.
- [x] 2.3 Add the formal CCSDS S-band hosted adoption probe and keep the spike probe as a compatibility wrapper.
- [x] 2.4 Pin old `ComFprime` S-band/UHF regression probes to `OBC_ComFprimeLegacy`.

## 3. CCSDS Capture And Decode Evidence

- [x] 3.1 Add optional raw relay capture to `ground_ttc_gateway`.
- [x] 3.2 Add repo-local CCSDS capture decoder and focused tests.
- [x] 3.3 Wire the adoption probe to capture and decode TC/TM/APID observations.

## 4. Specs, Evidence, Docs, And Pending

- [x] 4.1 Add `ccsds-sband-hosted-adoption-v1` evidence record and update the verification-path registry.
- [x] 4.2 Update README/script documentation for CCSDS default and explicit legacy `ComFprime`.
- [x] 4.3 Update pending planning notes with merged runtime-maintainability status and the CCSDS adoption decision.

## 5. Validation And Closeout

- [x] 5.1 Run focused unit tests and hosted adoption/legacy probes after a fresh build.
- [x] 5.2 Run the repository verification gate.
- [x] 5.3 Run OpenSpec validation and archive/sync the change through the governed workflow.
- [x] 5.4 Prepare the local-ready git boundary without pushing.
