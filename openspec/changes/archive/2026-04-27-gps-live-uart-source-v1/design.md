## Context

The existing GPS subsystem already has the correct architectural seam for source expansion. `GpsBridge` owns GPS command, telemetry, event, and cached-state behavior, while `IGpsSentenceSource` abstracts where NMEA sentences come from. That design should be extended rather than bypassed.

The next hardware step has also been chosen: `GY-GPS6MV2` will connect directly to `obc.local:/dev/serial0`, and the prior OBC-side serial comm path will no longer be the active target baseline. This means the change must do two things together:

1. add the UART-backed source cleanly inside the GPS subsystem
2. realign the repository narrative so old `/dev/serial0` comm evidence remains historical rather than current

The first live hardware slice stays narrow. It is not trying to prove sky visibility, PPS discipline, or omitted-RF integration. It only needs to prove that real UART sentences can enter the existing GPS runtime path safely and reviewably.

## Goals / Non-Goals

**Goals:**

- keep `GpsBridge` as the owner of GPS runtime state and source-mode behavior
- add `LIVE_UART` as a third governed source mode
- use a dedicated POSIX serial-backed source implementation rather than reusing `comm` transports
- preserve fake/replay defaults and hosted regressions
- prove at least one successful hardware-backed `gps get` on `obc.local`
- move `/dev/serial0` ownership to GPS in the active target baseline wording

**Non-Goals:**

- live-sky fix guarantee
- PPS or timing discipline
- RF, TT&C, or `comm-subsystem` migration implementation
- custom GDS plugin work
- GPS over CSP
- downlinking GPS over a new path in this change

## Decisions

### Decision: Add a dedicated serial GPS source instead of reusing comm transport

GPS is a line-oriented UART NMEA producer, not a request/response radio peer. The implementation will add a dedicated `IGpsSentenceSource` backend that owns serial open/configure/read behavior independently of `CommController`, `RadioController`, and `ByteStreamTransport`.

Alternative considered:

- reuse `obc_comm_transport`
  - rejected because that abstraction is shaped around comm/radio exchange semantics and would blur subsystem ownership

### Decision: Keep fake as the default source mode

Even after live UART support exists, the default runtime mode remains `fake`. Software-only and hosted baselines should stay runnable without hardware.

Alternative considered:

- switch the default to `live-uart` on Raspberry Pi
  - rejected because it would make target startup fragile before the hardware path is always available

### Decision: `live-uart` mode validates device availability at activation time

If the configured serial device cannot be opened or configured, runtime mode activation fails with `VALIDATION_ERROR`. This keeps the failure visible at the `GpsBridge` contract surface instead of silently leaving the bridge in an unusable state.

Alternative considered:

- allow activation to succeed and only fail later during polling
  - rejected because it makes source-mode state misleading

### Decision: First hardware probe proves ingestion, not fix quality

The first target-side probe only requires hardware sentence ingestion and bounded state/counter movement. It does not require valid fix or fix-loss transitions.

Alternative considered:

- require `GPS_FIX_ACQUIRED`
  - rejected because it couples the first hardware slice to sky conditions instead of transport/runtime behavior

### Decision: Old OBC-side serial comm evidence stays historical

The repository will not delete or falsify prior `/dev/serial0` comm evidence. Instead, specs and registry wording will mark it as historical/superseded once GPS takes over the active OBC-side UART.

Alternative considered:

- keep `/dev/serial0` shared or time-multiplexed
  - rejected because that would reintroduce the serial ownership ambiguity this slice is resolving

## Risks / Trade-offs

- **[Risk] target-side serial comm probes become non-current after `/dev/serial0` moves to GPS** → Mitigation: keep them as historical evidence and explicitly say active comm development remains on TCP/dev paths until later migration
- **[Risk] hardware probe may see only no-fix or sparse data indoors** → Mitigation: acceptance requires sentence ingestion and state movement, not fix quality
- **[Risk] serial source could wedge on partial lines** → Mitigation: add PTY-backed L1 coverage for timeout, trimming, and partial-line behavior
- **[Risk] env/config surface could drift between OBC shell and probe scripts** → Mitigation: use the same governed `OBC_GPS_*` variables in runtime, operator docs, and probe scripts

## Migration Plan

1. Add `gps-live-uart-source-v1` delta specs and task list
2. Implement serial-backed GPS source plus `GpsBridge`/operator/runtime changes
3. Add L1/L2 focused tests
4. Add target-side GPS live probe and evidence
5. Sync/archive the change while preserving the older OBC `/dev/serial0` comm proofs as historical records

## Open Questions

- none for this bounded slice; later comm migration work will decide the next target-side external comm hardware path
