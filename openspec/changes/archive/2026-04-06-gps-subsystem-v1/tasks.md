## 1. Artifact Completion

- [x] 1.1 Finalize `gps-subsystem` design and delta specs for housekeeping archive, verification evidence, and verification-path registration

## 2. GPS Runtime Model And Parser

- [x] 2.1 Add first-version GPS runtime types for cached fix validity, position, time, and basic reception metadata
- [x] 2.2 Implement a bounded NMEA parser with valid-sentence, malformed-sentence, and no-fix handling
- [x] 2.3 Add fake or replay GPS source support that can drive the GPS bridge in hosted validation

## 3. GPS Bridge And OBC Integration

- [x] 3.1 Implement `GpsBridge` with owned `GPS_*` command, telemetry, and event behavior plus cached runtime access
- [x] 3.2 Wire the GPS subsystem into the OBC topology and runtime configuration without folding it into `comm-subsystem`
- [x] 3.3 Extend housekeeping snapshot runtime structures and `HousekeepingSnapshotProvider` to include GPS cached state

## 4. Verification And Evidence

- [x] 4.1 Add focused unit and/or component tests for the parser and GPS bridge cached-state behavior
- [x] 4.2 Add hosted validation covering valid fix, no-fix, and malformed-sentence cases plus housekeeping archive capture of GPS state
- [x] 4.3 Record repository evidence and a repo-external development note that captures repeated workflow steps for later skill evaluation
