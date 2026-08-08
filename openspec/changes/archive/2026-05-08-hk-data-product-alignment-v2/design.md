## Context

`HkTrendProductProducer` currently serializes `HkTrendRecordV3` from `IStateSnapshotSource`, and the product record name/id are already stable as `HkTrendRecord` id `0`. V3 includes the current mode-model-v2 semantics and data-products storage-root visibility, but the fallback HK ring still captures several cached runtime fields that are useful for final-design health/state history and are not yet available in the official `.fdp` record.

The current default hosted operator path is CCSDS S-band through `OBC`, `OBCApp.*`, `ground_ttc_gateway`, and `sband_comm_csp_node` node `5`. The existing onboard-state data-product probe proves `.fdp` parity on a direct Native GDS path. This change adds focused evidence on the default CCSDS S-band path rather than reinterpreting the direct-path evidence.

## Goals / Non-Goals

**Goals:**

- Make `HkTrendRecord` V4 the current official HK/state history payload.
- Include cached/provider-backed gaps already available in runtime state without adding new subsystem polling.
- Preserve missing-data semantics through explicit validity flags and quality masks.
- Prove `.fdp` generation, `DpCatalog` build/xmit, GDS-received byte match, and decode over default hosted CCSDS S-band.
- Keep `HousekeepingArchive` as a bounded transitional fallback and document that `hk/index.csv` has different ring-slot semantics from `DpCatalog`.

**Non-Goals:**

- No `health_manifest.json` or replacement manifest.
- No HK ring retirement, capacity expansion, or fallback schema change.
- No payload, FDIR, scheduler, process-health, persistent event/fault, or clear-log command implementation.
- No arbitrary onboard file downlink, reliable transfer, RF, target/Pi, UHF CCSDS, or command-authority claim.
- No backward decode promise for V1/V2/V3 `.fdp` files with the V4 dictionary.

## Decisions

### V4 is the current payload boundary

The product record remains `HkTrendRecord` id `0`, but its FPP type becomes `HkTrendRecordV4`, and `version` is set to `4`. This intentionally separates the final-design alignment payload from V3, even though old product identity remains stable for operator cataloging. Evidence targets only V4 products generated with the current dictionary.

### V4 uses existing cached providers only

The producer continues to read `IStateSnapshotSource` and does not poll EPS, ADCS, GPS, COMM, storage, boot, radio, UART, or CSP transports directly. V4 extends the snapshot source to include values already available from existing runtime providers: ADCS quaternion/magnetometer, GPS speed/course/UTC date/time, full storage-root stats, COMM active band, CSP initialized/node/error/free-buffer counters, radio status, radio/UART link error counters, and boot metadata.

### Missing data stays explicit

Subsystem-level booleans remain the authority for interpreting subsystem values. When a subsystem snapshot is missing, V4 fields for that subsystem are set to neutral serialized values, the subsystem validity flag is false, and the quality/fault masks retain the missing-source signal where already defined. This prevents ground decode from treating neutral numeric values as valid measurements.

### `DpCatalog` is official; HK ring index is fallback-only

`DpCatalog` is the official `.fdp` catalog/index and transmit surface. `HousekeepingArchive` and `hk/index.csv` remain unchanged because their slot/generation/active-state metadata describes a bounded fallback ring, not official product catalog semantics.

### Default hosted evidence is CCSDS S-band

The new focused probe uses `OBC`, `OBCApp.*`, CCSDS GDS framing, `ground_ttc_gateway` raw relay, S-band TCP, and `sband_comm_csp_node` node `5`. It proves `.fdp` byte fidelity and decode on the default hosted path, while excluding RF, UHF, target/Pi, packet-loss recovery, arbitrary path downlink, and repository-level reliable transfer.

## Risks / Trade-offs

- V4 dictionary changes require explicit evidence and no same-version historical decode claim.
- Adding cached-gap fields increases product size and UT surface, but avoids repeated future version bumps for fields already present in runtime state.
- The CCSDS S-band probe depends on a longer socket/GDS chain than the direct probe; mitigate with isolated runtime roots, isolated GDS storage, fresh ports, and bounded PASS markers.
