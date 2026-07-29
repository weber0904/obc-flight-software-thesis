## 1. OpenSpec and Spec Deltas

- [x] 1.1 Finalize proposal, design, and tasks for `official-sequencing-system-resources-v1`
- [x] 1.2 Add delta specs for the active sequencing, file-ingress governance, and verification truth changes

## 2. File Ingress Governance

- [x] 2.1 Add `FileIngressAuthority` component and its helper logic for logical-path allowlist, physical-path rewrite, and denied-transfer state
- [x] 2.2 Wire `FileIngressAuthority` into the active `ComCcsds -> FileUplink` path
- [x] 2.3 Add classic L2 coverage and helper tests for accepted, denied, and follow-on dropped file-packet behavior

## 3. Sequence Admission and Stock Control Guarding

- [x] 3.1 Extend command authority catalog/policy to deny direct external stock sequence controls and classify new wrapper commands
- [x] 3.2 Add `SequenceAdmissionController`, admitted-copy workflow, synthetic stock-command builder, and bounded context table
- [x] 3.3 Add lifecycle callback fanout and ownership-aware manual/auto sequence control
- [x] 3.4 Add classic L2 coverage and helper tests for admission, TOCTOU, ownership, backup policy, and status truth

## 4. Official Service Integration

- [x] 4.1 Instantiate and wire `cmdSeqA`, `cmdSeqB`, and `seqDispatcher` with dedicated `CmdDispatcher` indices
- [x] 4.2 Instantiate and wire `systemResources`
- [x] 4.3 Move sequencer, file-manager, and `systemResources` scheduling to a truthful 1 Hz path and set nonzero sequencer timeouts in topology setup

## 5. Hosted Proof and Docs

- [x] 5.1 Add a repository-owned hosted probe for governed sequence upload/admission/execution and `SystemResources`
- [x] 5.2 Add `docs/test-records/official-sequencing-system-resources-v1/README.md`
- [x] 5.3 Update architecture, roadmap, operator, and verification-registry documents
- [x] 5.4 Run fresh focused verification and `openspec validate official-sequencing-system-resources-v1`
- [x] 5.5 Run `openspec validate --specs`
