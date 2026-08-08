## 1. OpenSpec And Contract Updates

- [x] 1.1 Add proposal, design, tasks, and delta specs for `payload-data-products`, `payload-operations`, `comm-subsystem`, `resource-storage`, and `verification-path-registry`
- [x] 1.2 Reconcile active docs and runbook wording for dual local artifacts, preview auto-publish, raw on-demand publish, and the bounded `FULL` raw non-claim

## 2. Payload Dual-Artifact Implementation

- [x] 2.1 Extend `PayloadOpsController` FPP/runtime types for `captureIndex`, `PayloadArtifactKind`, artifact-aware metadata, and the new official payload header/bytes records
- [x] 2.2 Implement local `PIC%02X.bin/.jpg` storage, preview auto-publish, raw on-demand publish, and `FULL` raw bounded rejection
- [x] 2.3 Update the helper protocol, CSP metadata readback, and state/event/tlm surfaces to expose artifact-specific paths and publication state

## 3. Driver And Transport Work

- [x] 3.1 Update the target and hosted payload drivers so one raw capture produces one local raw artifact plus one valid preview JPEG from the same source frame
- [x] 3.2 Raise `FW_FILE_BUFFER_MAX_SIZE`, `ComCfg::TmFrameFixedSize`, and S-band file buffer sizing to the new governed packet budget

## 4. Tests, Tooling, And Proof

- [x] 4.1 Update or add focused unit tests for dual-artifact metadata, record serialization, preview auto-publish, raw publish, overwrite-by-index, and bounded `FULL` raw rejection
- [x] 4.2 Extend payload `.fdp` tooling to decode new artifact-based products while keeping archived legacy payload `.fdp` evidence decodable
- [x] 4.3 Run focused hosted and target proof for preview plus bounded raw official downlink, then run `openspec validate payload-raw-preview-dual-artifact-v1` and `openspec validate --specs`
