# Tasks: payload-public-surface-convergence-v1

- [x] 1. Formalize the change boundary
  - [x] 1.1 Add proposal, design, and delta specs for payload public-surface convergence.

- [x] 2. Converge payload API and runtime
  - [x] 2.1 Replace `PAYLOAD_SET_DEFAULTS` and `PAYLOAD_PREPARE_SESSION` with `PAYLOAD_SET_CAMERA_DEFAULTS` and `PAYLOAD_PREPARE_RAW_SENSOR`.
  - [x] 2.2 Split prepare-ready truth from capture-policy truth in payload runtime types, metadata, manifests, and readback/event surfaces.
  - [x] 2.3 Narrow AUTO and DETERMINISTIC defaults so shared non-RAW session-level controls live only on `PAYLOAD_SET_CAMERA_DEFAULTS`.
  - [x] 2.4 Preserve backward-compatible load behavior for legacy capture manifests.

- [x] 3. Refresh focused verification
  - [x] 3.1 Update component/helper/parser tests for the new command surface, new ready/policy truth, and legacy-manifest compatibility.
  - [x] 3.2 Run focused payload UT and parser checks after the fresh build.

- [x] 4. Migrate maintained payload proofs and current docs
  - [x] 4.1 Update current Route 1 target payload wrapper, payload target sanity proof, and hosted persistent-session proof to the new command and metadata surface.
  - [x] 4.2 Update current docs/specs/registry/test-records and mark old `SET_DEFAULTS` / `PREPARE_SESSION(AUTO|DETERMINISTIC)` / `PSESSION_*` truth as historical or superseded.

- [x] 5. Close out locally
  - [x] 5.1 Rerun focused maintained payload proofs and Route 1 target payload closure.
  - [x] 5.2 Run `openspec validate payload-public-surface-convergence-v1`, `openspec validate --specs`, `python3 scripts/check_repo_consistency.py`, and `python3 scripts/check_documentation_governance.py`.
