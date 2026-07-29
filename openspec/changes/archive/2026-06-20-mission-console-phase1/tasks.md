## 1. Change Scaffolding

- [x] 1.1 Track the Mission Console Phase 1 change on `feature/mission-console-phase1` and keep the implementation aligned with `docs/roadmap/mission-console-phase1-handoff.md`.

## 2. Manual Operator Refactor

- [x] 2.1 Refactor `scripts/manual_ops/manual_secure_ops.py` into importable structured action helpers while preserving the current CLI verbs and JSON outputs.
- [x] 2.2 Add structured request/result types and shared helper functions so Gateway code can call auth, command, upload, sequence, and status actions without subprocess parsing.

## 3. Mission Gateway Backend

- [x] 3.1 Add the `scripts/mission_console/` Flask application skeleton with local routes, JSON APIs, job runner, and persistent Mission Console runtime root.
- [x] 3.2 Implement surface registry loading for hosted and target manual dual-GDS contexts, including manifest/status/session-state parsing and owner/port drift detection.
- [x] 3.3 Implement gateway-owned `fprime-cli events/channels` listener management, live snapshot caches, recent event ring buffers, and action history persistence.
- [x] 3.4 Implement curated readback parsing for the planned `channel-refresh-based` and `event-based` command families.
- [x] 3.5 Implement the bounded packet-lab backend for replay, stale-session, duplicate/tampered sequence, and tampered-MAC demo cases with parsed packet summaries and evidence capture.

## 4. Mission Console UI

- [x] 4.1 Add server-rendered dashboard, ops, readback, surfaces, and packet-lab pages with lightweight polling JavaScript.
- [x] 4.2 Implement hosted-first operator workflows for auth, secure command, governed upload, sequence actions, dashboard updates, and detailed readback presentation.
- [x] 4.3 Extend the same UI/Gateway model to target-manual-ground parity while preserving target provenance/preflight gates and read-only shared-baseline presentation.

## 5. Docs And Verification

- [x] 5.1 Update the relevant current docs so Mission Console behavior, readback tiers, and packet-lab boundaries are documented without conflicting with the current manual baseline.
- [x] 5.2 Add focused tests for structured manual actions, registry/listener/readback logic, and packet-lab parsing/classification.
- [x] 5.3 Run the relevant verification for the new Mission Console surface and confirm hosted-first closure plus bounded target parity.
