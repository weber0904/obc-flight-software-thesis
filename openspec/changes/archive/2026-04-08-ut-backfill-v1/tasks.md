## 1. Targeted Test Additions

- [x] 1.1 Add a `GpsBridge` contract test covering valid/no-fix/rejected sentence behavior and source-mode switching semantics.
- [x] 1.2 Add a `StorageHealthBridge` contract test covering pre-configuration failure, configured scans, and scan counter/error behavior.
- [x] 1.3 Add a `HousekeepingSnapshotProvider` aggregation test covering required-provider failure and representative GPS/storage/comm/boot propagation.
- [x] 1.4 Add a `HousekeepingArchiveStore` logic test covering configuration/index validation and request rejection behavior.
- [x] 1.5 Add a standalone `TransparentLinkFraming` unit test for round-trip, escaping, and decode error cases.

## 2. Reporting And Matrix Alignment

- [x] 2.1 Update the verification inventory/report tool so it classifies the new contract/unit tests correctly.
- [x] 2.2 Update the checked-in verification matrix to reflect the new automated coverage and the remaining constrained gaps.
- [x] 2.3 Add the `verification-evidence` delta spec and any needed documentation index updates for the backfill policy.

## 3. Evidence And Validation

- [x] 3.1 Add a verification record summarizing the new backfill tests and any intentionally retained exceptions.
- [x] 3.2 Run the targeted new tests, the baseline verification gate, `openspec validate ut-backfill-v1`, and `openspec validate --specs`.

## 4. Finalization

- [x] 4.1 Archive the change after the backfill tests and matrix updates are aligned.
- [x] 4.2 Commit the backfill update using the repository's Conventional Commit rule.
