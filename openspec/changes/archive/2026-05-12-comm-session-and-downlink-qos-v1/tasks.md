## 1. Formalize and review the change boundary

- [x] 1.1 Create proposal, design, tasks, and delta spec artifacts for COMM-owned session/link-role runtime, dual-ingress command policy, and shared downlink arbitration.
- [x] 1.2 Run `openspec validate comm-session-and-downlink-qos-v1`.

## 2. Implement COMM runtime ownership

- [x] 2.1 Extend `CommController` runtime state to own primary command/telemetry/file link roles, per-link availability, observe-only pass state, and reviewable events/telemetry/counters.
- [x] 2.2 Add bounded COMM-owned helper/runtime logic as needed so policy, scheduling, and completion mapping stay reviewable without turning this into a helper-only PR.
- [x] 2.3 Make `COMM_SET_ACTIVE` perform explicit primary-role switching and default startup to `S-band primary + UHF backup`.

## 3. Integrate authenticated ingress with COMM policy

- [x] 3.1 Extend active `TopCcsds` to provide distinct S-band node-`5` and UHF node-`6` command ingress paths.
- [x] 3.2 Extend `CommandIngressAuthority` so COMM can drive runtime role/profile decisions per ingress while auth/session/sequence behavior stays owned there.
- [x] 3.3 Implement `S-band primary`, `UHF backup`, and `UHF primary` command-class policy, including session revocation on link/primary transition.

## 4. Implement COMM-owned downlink arbitration

- [x] 4.1 Rewire `HousekeepingArchive.fileOut` and `DpCatalog.fileOut` through a COMM-owned shared scheduling surface ahead of stock `FileDownlink`.
- [x] 4.2 Remove `DpCatalogFileDownlinkGate` from the active path and move completion/context ownership into the new COMM-owned scheduler.
- [x] 4.3 Implement non-preemptive DP-primary arbitration, pending-DP behavior, busy reject, completion cleanup, and owner-drop behavior on link loss or primary-file-link switch.

## 5. Add tests

- [x] 5.1 Expand the classic `CommController` L2 harness for role/profile transitions, observe-only pass behavior, HK/DP arbitration, and link-loss convergence.
- [x] 5.2 Expand the classic `CommandIngressAuthority` L2 harness for dual-ingress runtime policy, UHF backup allow/deny, UHF primary full authority, and COMM-driven session revocation.
- [x] 5.3 Add direct L1 tests for any new helper/runtime scheduling or completion-mapping logic.

## 6. Add hosted probes and evidence

- [x] 6.1 Add a bounded repository-owned `run_comm_session_and_downlink_qos_probe.sh`.
- [x] 6.2 Prove hosted S-band authenticated full-authority command admission, UHF backup low-risk admission/high-risk rejection, and UHF low-risk continuity while S-band file/downlink is active.
- [x] 6.3 Prove deterministic HK/DP arbitration and bounded UHF node-`6` file/downlink after primary switch.
- [x] 6.4 Keep command-policy proof and file/downlink proof separated in the evidence record and verification-path registry.

## 7. Update docs and close the change

- [x] 7.1 Add `docs/test-records/comm-session-and-downlink-qos-v1/README.md`.
- [x] 7.2 Update `docs/verification-path-registry.md`, `docs/roadmap/README.md`, any necessary roadmap detail docs, and `docs/architecture/current-development-architecture.md`.
- [x] 7.3 Run fresh local verification, focused regression probes, `openspec validate comm-session-and-downlink-qos-v1`, and `openspec validate --specs`.
