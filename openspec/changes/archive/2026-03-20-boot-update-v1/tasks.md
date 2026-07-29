## 1. Define the first boot/update implementation slice

- [x] 1.1 Add the change proposal, design notes, and spec deltas for `boot-update-v1`
- [x] 1.2 Reconcile the narrative boot/update source document with any newly explicit implementation decisions

## 2. Implement BootManager and metadata persistence

- [x] 2.1 Add `BootManager` plus file-backed metadata storage under `OBC/Components/BootManager/`
- [x] 2.2 Implement the prepare, verify, activate, confirm, and rollback flow using the staged-file path and persisted metadata

## 3. Add automated verification

- [x] 3.1 Add focused F' unit tests for the hosted boot/update lifecycle and invalid-metadata recovery
- [x] 3.2 Run the normal build, unit-test build, and project check gate

## 4. Capture evidence and close the change

- [x] 4.1 Record the implementation and verification results under `docs/test-records/boot-update-v1/`
- [x] 4.2 Run `openspec validate boot-update-v1` and archive the change after the implementation and evidence are complete
