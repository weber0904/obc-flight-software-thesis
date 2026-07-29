## 1. Artifact Completion

- [x] 1.1 Finalize `storage-health` design and delta specs for resource storage, housekeeping archive, verification evidence, and verification-path registration

## 2. Storage Scan Model

- [x] 2.1 Add first-version storage health runtime types for bounded per-root statistics, warning state, and scan metadata
- [x] 2.2 Implement a bounded governed-root scanner covering `hk`, `persistent-data`, `staging`, and `logs`
- [x] 2.3 Add threshold evaluation and degraded-state handling for missing or unreadable roots

## 3. Storage Health Bridge And OBC Integration

- [x] 3.1 Implement `StorageHealthBridge` with owned `STORAGE_*` command, telemetry, and event behavior plus cached runtime access
- [x] 3.2 Wire the storage-health subsystem into the OBC topology and runtime configuration without folding it into `HousekeepingArchive` or `BootManager`
- [x] 3.3 Extend housekeeping snapshot runtime structures and `HousekeepingSnapshotProvider` to include storage health cached state

## 4. Verification And Evidence

- [x] 4.1 Add focused unit and/or component tests for the scanner and storage bridge cached-state behavior
- [x] 4.2 Add hosted validation covering representative governed-root contents, missing-root or threshold-warning behavior, and housekeeping archive capture of storage health state
- [x] 4.3 Record repository evidence and update the verification-path registry with the hosted storage health path
