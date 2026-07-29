## 1. OpenSpec And Contract Updates

- [x] 1.1 Add the proposal, design, and delta specs for `payload-data-products`, `payload-operations`, `comm-subsystem`, `resource-storage`, and `verification-path-registry`
- [x] 1.2 Reconcile active docs and runbook wording for canonical payload `.fdp` delivery, payload non-claims, secure-auth baseline progress, current UHF nonquiet truth, and the deferred target payload `.fdp` proof boundary

## 2. Payload Canonical FDP Implementation

- [x] 2.1 Extend `PayloadOpsController` FPP/runtime types to define payload data-product records, product ports, publication telemetry/detail state, and canonical artifact identity
- [x] 2.2 Implement local-JPEG-to-canonical-`.fdp` publication, including `512 KiB` ceiling enforcement, failure semantics, and metadata/readback propagation
- [x] 2.3 Update `TopCcsds` data-product wiring and `DpBufferManager` policy so payload products can use the official `DpCatalog` path without widening reliable-transfer scope

## 3. Test And Tooling Coverage

- [x] 3.1 Add or update unit tests for payload product serialization, publication failure, oversize rejection, metadata/readback parity, and payload CSP metadata reply changes
- [x] 3.2 Add repo-owned payload `.fdp` decode and JPEG extraction tooling for proof scripts and evidence validation

## 4. Hosted And Target Proof

- [x] 4.1 Add the hosted node-`5` payload `.fdp` proof path, including multi-resolution capture size sampling, GDS byte-match, decode, and JPEG hash-match assertions
- [x] 4.2 Explicitly defer the bounded target node-`5` payload `.fdp` proof path until a single repo-owned COMM-backed payload wrapper exists, and record that non-claim in docs and the verification registry
- [x] 4.3 Run fresh local verification, hosted proof, the bounded target proof when it exists, and `openspec validate payload-e2e-downlink-closure-v1` plus `openspec validate --specs`
