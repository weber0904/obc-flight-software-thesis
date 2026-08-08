## Context

The desired flight data system has three separate roles:

- onboard monitoring: consume existing cached subsystem state and reduce it for internal consumers without flooding downlink,
- live beacon: emit a small current-health frame on a 17-second cadence through a COMM-facing broadcast sink,
- historical storage: write housekeeping trend records as official F' data products so `DpWriter` and `DpCatalog` own file/catalog semantics.

PR #41 implemented the first role and a preliminary beacon, but it also stored HK trend and beacon history through a repo-local `StateDataArchive` and `catalog.csv`. The replacement baseline keeps the useful low-coupling state interfaces and switches stored products to official F' data-product components.

## Goals / Non-Goals

**Goals:**

- Keep high-frequency source data onboard-only by default and expose only reduced state, low-rate telemetry summaries, important events, live beacon frames, or official data products.
- Keep `OnboardStateMonitor`, `BeaconPublisher`, and `HkTrendProductProducer` independently testable through small interfaces.
- Use actual cached subsystem measurements plus validity flags for `BeaconV1` and HK trend records.
- Use official `DpManager`, `DpWriter`, and `DpCatalog` for HK trend file/catalog behavior under `<runtime-root>/data-products/`.
- Preserve existing `ComFprime`, `TlmChan`, commands, events, telemetry, `HousekeepingArchive`, and `FileDownlink`.

**Non-Goals:**

- No `MissionExecutive`, LOW_POWER, DETUMBLE, load shedding, or other autonomy action.
- No RF behavior claim, vendor radio framing claim, CFDP, ARQ, CCSDS migration, or target-side SD-card claim.
- No replacement of the existing housekeeping ring/index archive.
- No mission `BEACON_HISTORY` data product.

## Decisions

### Reduced state reads cached subsystem state through a small source interface

`OnboardStateMonitor` depends only on `IStateSnapshotSource`. The production topology supplies `OnboardStateSnapshotSource`, which reads existing runtime cached state from EPS, ADCS, GPS, storage, COMM, radio, UART, boot, and mode components. A source failure invalidates the cached reduced state so downstream consumers cannot continue using a stale healthy value.

### Live beacon is broadcast payload, not normal telemetry packetization

`BeaconPublisher` uses `IReducedStateSource` and `IBeaconSink` only. The runtime sink is topology-owned and COMM-facing. Beacon sequence advances only after the sink accepts the frame, and not-configured failures are reported separately from source-unavailable failures.

### HK trend products use official F' data product containers

`HkTrendProductProducer` reads `IStateSnapshotSource`, maps raw cached values and validity flags into an FPP serializable `HkTrendRecordV1`, requests a data-product container, and sends the filled container to `DpManager`/`DpWriter`. `DpCatalog` owns catalog build and file-downlink queueing.

### Repo-local product catalog shim is removed

`StateDataArchive`, `ProductCatalog`, OPD1 product file encoding, and `catalog.csv` are removed from the formal mission baseline. Hosted debug tooling may decode captured beacon frames, but those captures are evidence artifacts rather than mission data products.

## Risks / Trade-offs

- `DpCatalog` downlink uses the existing `FileHandling.fileDownlink` path, so concurrent manual file downlink and catalog transmission should be treated as mutually exclusive in this first baseline.
- Official F' data product files do not match prior OPD1 files. The hosted probe and evidence should validate official file generation/catalog behavior and explicitly reject the old `catalog.csv` baseline.
- The live beacon sink is hosted/COMM-facing only in this change. RF, pass automation, and real-radio behavior require later governed validation.
