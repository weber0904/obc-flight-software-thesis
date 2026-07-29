## Context

The project already has an established pattern for subsystem integration: `EpsBridge` and `AdcsBridge` own subsystem-facing command, telemetry, and event contracts, keep a cached runtime view of the latest valid subsystem state, and let other OBC features reuse that cached state instead of issuing a second transport poll loop. The recently added housekeeping archive follows the same pattern by capturing cached/runtime state from subsystem bridges and core controllers into a governed archive snapshot.

The new GPS slice should follow that same architecture. Although the target hardware module (`GY-GPS6MV2`, commonly built around `NEO-6M`) often exposes a UART interface, its role is not equivalent to the external comm path. The current `comm-subsystem` owns radio and byte-stream link control for external communications, while GPS is a navigation and timing sensor feed. Treating GPS as "just another UART inside comm" would blur that boundary and make later housekeeping, autonomy, and navigation use cases harder to reason about.

There is also a practical hardware constraint: the GPS module is not yet wired to the Raspberry Pi target, indoor development may not yield a valid fix, and the project still needs a reviewable first-version contract before the real hardware bring-up. This makes a fake-first and replay-friendly slice the correct first implementation: prove parser behavior, cached state ownership, telemetry/event semantics, and archive integration now, while keeping the future Pi UART path explicit and deferred.

## Goals / Non-Goals

**Goals:**

- introduce a dedicated `gps-subsystem` capability rather than folding GPS into `comm-subsystem`
- define first-version GPS-owned command, telemetry, and event families through a `GpsBridge`
- parse a bounded first-version NMEA subset suitable for `GY-GPS6MV2` / `NEO-6M` style outputs
- support fake and replayable GPS sources so hosted validation does not depend on outdoor reception or immediate hardware access
- keep the latest valid GPS state cached for runtime consumers
- extend housekeeping archive snapshots to include GPS cached state without adding another archive-time transport poll path
- make the lack of Raspberry Pi UART hardware evidence explicit in specs and verification scope
- register the new hosted GPS fake/replay validation path so later changes do not need to infer it from generic hosted-runtime behavior

**Non-Goals:**

- real Raspberry Pi UART hardware bring-up for GPS in this change
- PPS, disciplined clocking, or precise time synchronization
- full GPS receiver configuration management over UART
- navigation fusion, orbit propagation, or closed-loop use of GPS data by autonomy or ADCS
- routing GPS bytes through the external comm/radio subsystem
- claiming validated indoor live-fix performance

## Decisions

### Decision: Introduce GPS as its own subsystem instead of reusing `comm-subsystem`

GPS happens to use UART at the transport layer, but its system role is sensor/state ingestion, not external communications control. A dedicated `gps-subsystem` keeps ownership of GPS semantics in one place: sentence parsing, fix validity, cached navigation state, and GPS-specific telemetry/events. This also preserves the current meaning of `comm-subsystem` as the owner of radio/link control.

Alternative considered:

- reuse the current comm/radio/UART stack for GPS input
  - rejected because it would mix navigation semantics with radio/link behavior and would make housekeeping/autonomy consumers depend on the wrong subsystem boundary

### Decision: Implement fake/replay source support first and defer real UART source support

The target module is not yet wired, and indoor development may not yield a fix even when it is connected. A fake/replay source still lets the project prove the important first-version contracts: parser behavior, cached runtime state, invalid-fix handling, events, telemetry, and archive integration. The design will leave a clear transport seam for a future UART-backed source, but this change will not claim hardware integration.

Alternative considered:

- block the full change until live UART hardware is available
  - rejected because it would unnecessarily delay subsystem contract and archive work that can be reviewed and verified today

### Decision: Bound the first parser to a narrow NMEA subset with explicit validity handling

The first slice should accept a bounded set of NMEA sentences that are sufficient to expose the common GPS state needed by OBC telemetry and housekeeping: fix validity, latitude, longitude, altitude, UTC time, speed/course (if present), and satellite count. Invalid checksum, malformed fields, and no-fix sentences should preserve the last valid cached fix while surfacing the degraded condition through the owned event path.

Alternative considered:

- parse every NMEA sentence the receiver may emit
  - rejected because it broadens the change without improving the first-version OBC contract

### Decision: Keep GPS state cached and archive that cache, not a live transport read

The housekeeping archive already requires that captures reuse existing cached/runtime state rather than issuing another transport poll. GPS will follow the same rule: `GpsBridge` owns the latest valid GPS state and fix metadata, and `HousekeepingSnapshotProvider` reads that cache when capturing a snapshot. This keeps archive capture deterministic and avoids coupling archive timing to live GPS transport behavior.

Alternative considered:

- let housekeeping archive ask the GPS source directly for a fresh sample during capture
  - rejected because it would introduce another transport access path and violate the archive design boundary

### Decision: Keep the first GPS command surface minimal

The first GPS slice should expose only the commands needed to support validation and operator inspection, such as an immediate refresh from the current source and a bounded source-mode selection for fake/replay operation. It should not expose a broad receiver configuration API yet.

Alternative considered:

- expose full receiver configuration commands immediately
  - rejected because real hardware configuration is deferred and would create a misleadingly large public contract

## Risks / Trade-offs

- [Risk] Fake/replay validation could be mistaken for completed hardware integration. → Mitigation: make the deferred Raspberry Pi UART path explicit in specs, evidence, and test records.
- [Risk] A narrow NMEA subset may omit fields later needed by autonomy or scheduling. → Mitigation: define a stable cached-state boundary now and expand sentence support in a later change without collapsing the subsystem boundary.
- [Risk] Indoor live modules may emit no-fix or stale data that differs from ideal fake feeds. → Mitigation: treat invalid/no-fix handling as first-class behavior in the bridge and test it deliberately with replayed sentences.
- [Risk] Adding GPS fields to housekeeping snapshots changes the archive payload shape. → Mitigation: keep the added fields bounded and document the archive format extension in the housekeeping delta spec.

## Migration Plan

1. Add the new `gps-subsystem` capability and delta specs for housekeeping archive and verification evidence.
2. Introduce the first GPS runtime model, parser, and bridge with fake/replay source support.
3. Extend the housekeeping snapshot and provider to include GPS cached state.
4. Add hosted tests and evidence covering valid-fix, no-fix, and malformed-sentence behavior.
5. Register the hosted GPS fake/replay validation path in the repository verification-path registry.
6. Leave Raspberry Pi UART bring-up explicitly deferred for a later change once hardware wiring is ready.

## Open Questions

- When the real UART-backed GPS source is added, should the project prefer GPIO UART bring-up on Raspberry Pi first or use a development-only USB serial adapter to keep the comm UART path isolated?
- Which exact NMEA sentence set does the user’s specific `GY-GPS6MV2` board emit by default, and does it need a later configuration slice to align with the parser assumptions?
- Should later mission-scenario work consume GPS time/location directly from `GpsBridge`, or should a separate navigation/runtime aggregator be introduced once more sensors exist?
