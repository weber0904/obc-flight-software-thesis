## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `persistent-command-freshness-v1`.
- [x] 1.2 Validate the change artifacts before implementation with `openspec validate persistent-command-freshness-v1`.

## 2. Persistent Freshness Store And Runtime Wiring

- [x] 2.1 Add a dedicated `CommandIngressAuthority` persistent freshness store under `persistent-data/command-ingress/` with dual-copy snapshot, generation, and CRC fallback behavior.
- [x] 2.2 Add a runtime persistent-root configuration surface for `CommandIngressAuthority` and wire it through `HostedRuntime`, `Main.cpp`, and `MainComFprimeLegacy.cpp`.
- [x] 2.3 Keep `BootManager` as the owner of boot trust and boot metadata; do not move command freshness into boot metadata.

## 3. Reboot-Safe Session Epoch Enforcement

- [x] 3.1 Upgrade `session_id` semantics to monotonic reopen epoch per source epoch on the active path.
- [x] 3.2 Require accepted `SESSION_OPEN(seq0)` to persist the new source-epoch floor before reopening the session.
- [x] 3.3 Reject replayed or lower/equal `SESSION_OPEN` values after restart, while keeping non-lifecycle traffic fail-closed until a fresh higher `SESSION_OPEN` succeeds.
- [x] 3.4 Keep per-command sequence monotonic enforcement in memory after open instead of persisting every accepted sequence.
- [x] 3.5 Add dedicated persistent-freshness telemetry, events, and fail-closed rejection reasons without changing the wire format or adding a new operator command.

## 4. Tests, Probes, And Evidence

- [x] 4.1 Extend component and helper tests for first-boot initialization, persisted-floor advance, restart replay rejection, fresh reopen acceptance, per-source independence, single-copy corruption fallback, and both-invalid fail-closed behavior.
- [x] 4.2 Add or update a hosted active-path probe that proves same-runtime-root restart rejects replayed old envelopes and accepts a fresh higher reopen.
- [x] 4.3 Add bounded Raspberry Pi persistence proof on the active path showing command-freshness state and boot-trust metadata survive target restart or reboot truthfully.
- [x] 4.4 Update `docs/interfaces.md`, active architecture docs, verification registry, and evidence records to reflect the new monotonic `session_id` contract and bounded non-claims.

## 5. Verification And Closeout

- [x] 5.1 Run the fresh local verification gate plus focused affected tests and probes.
- [x] 5.2 Run `openspec validate persistent-command-freshness-v1` and `openspec validate --specs`.
- [x] 5.3 Keep the worktree review-ready without claiming legacy retirement or broader secure-boot closure.
