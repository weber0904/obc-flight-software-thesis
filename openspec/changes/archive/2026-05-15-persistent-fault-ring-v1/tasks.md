## 1. Formalize the 06 change boundary

- [x] 1.1 Create the proposal and delta specs for the new `persistent-fault-ring` capability plus the affected boot, recovery, storage, and verification contracts.
- [x] 1.2 Capture the implementation design decisions for owner boundary, dual-copy snapshot format, writer wiring, hosted shell readback, and stale narrative refresh.

## 2. Build the persistent fault ring foundation

- [x] 2.1 Add the shared runtime types/interfaces and `PersistentFaultStore` helper with focused L1 coverage for empty load, append, wraparound, newest-first readback, newer-copy corruption fallback, and both-invalid empty load.
- [x] 2.2 Add the passive `PersistentFaultManager` component, its bounded command/event contract, and a classic F' L2 harness for the public readback surface.
- [x] 2.3 Register the new component and support sources in the build so both active and legacy topologies can instantiate it.

## 3. Wire boot, recovery, topology, and hosted runtime surfaces

- [x] 3.1 Wire `BootManager` to the boot-writer interface and update its tests to cover boot-observed and recovery-boot-ack breadcrumb writes without changing boot metadata truth.
- [x] 3.2 Wire `RecoveryExecutor` to the recovery-writer interface and update its tests to cover incident-open, action lifecycle, reboot, and clear breadcrumbs without adding detector-local duplicate writers.
- [x] 3.3 Instantiate and configure `PersistentFaultManager` in `TopCcsds`, legacy `Top`, and hosted runtime services, then add the hosted `fault history [count]` shell surface.

## 4. Add governed probes and refresh current narrative

- [x] 4.1 Add the repository-owned hosted persistent fault ring probe that proves same-runtime-root relaunch persistence and newer-copy corruption fallback.
- [x] 4.2 Refresh `docs/architecture-review/current/` and the architecture-review follow-up roadmap text so completed 01/03 work and retired HK fallback claims are no longer stale.
- [x] 4.3 Record the new evidence README and verification-path registry updates for the hosted persistent fault ring relaunch path.

## 5. Validate and close the local implementation loop

- [x] 5.1 Run the focused unit/component tests and the hosted persistent fault ring probe, then capture the exact verification commands and outcomes in the evidence record.
- [x] 5.2 Run the full fresh gate needed for this change, including `bash scripts/run_verification_ci.sh ...`, `openspec validate persistent-fault-ring-v1`, and `openspec validate --specs`.
- [x] 5.3 Update this task list to reflect the completed implementation and verification work.
