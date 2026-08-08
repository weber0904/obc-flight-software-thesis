## 1. OpenSpec And Contract Updates

- [x] 1.1 Add proposal, design, and delta specs for payload async capture acknowledgement and governed target payload `.fdp` proof
- [x] 1.2 Reconcile active docs, registry, and evidence wording so payload capture completion is state-driven and target node-`5` payload proof is no longer deferred

## 2. Async Capture Acknowledgement Implementation

- [x] 2.1 Update `PayloadOpsController` so accepted `PAYLOAD_CAPTURE_*` commands return `OK` immediately instead of waiting for capture or `.fdp` publication completion
- [x] 2.2 Add explicit post-capture publication observability, including `PSTATE_PUBLISHING`, and keep final success/failure on payload state/event/tlm/readback surfaces
- [x] 2.3 Update payload unit tests and any dependent hosted probe logic to use the new async completion oracle

## 3. Target Payload FDP Proof

- [x] 3.1 Add a repository-owned governed target node-`5` payload `.fdp` proof wrapper with byte-match, decode, and JPEG hash parity
- [x] 3.2 Register the target payload proof path and update the hosted/target payload evidence split

## 4. Verification

- [x] 4.1 Run focused native build and payload UT coverage
- [x] 4.2 Rerun hosted payload `.fdp` proof on the async model
- [x] 4.3 Run governed target node-`5` payload `.fdp` proof
- [x] 4.4 Run `openspec validate payload-async-capture-ack-and-target-proof-v1` and `openspec validate --specs`
