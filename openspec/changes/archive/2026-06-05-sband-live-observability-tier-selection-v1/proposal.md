## Why

`sband-auth-gated-observability-v1` fixed who may see node-`5` S-band live
observability, but it intentionally did not decide which post-auth
`event/tlm` surfaces should remain in the live stream. The current maintained
baseline still emits broad scheduled `EPS`, `ADCS`, `GPS`, `Radio`, and
`Storage` data after auth, which leaves pass-time operator observability noisy
and makes low-value detail compete with the few fields operators actually need
to watch continuously.

This change narrows that live surface without redesigning the auth gate,
packetization model, or command plane. It formalizes the first current content
tiers, keeps onboard cached truth distinct from fresh readback, and makes the
first family set (`EPS`, `GPS`, `partial ADCS`, `RADIO`, `STORAGE`) usable as
bounded operator readback instead of broad ambient chatter.

## What Changes

- Create a formal observability audit and tiering matrix for node-`5`
  post-auth S-band live content selection.
- Define four current tiers:
  - `keep-live summary`
  - `fresh GET-driven bounded readback`
  - `onboard cached truth`
  - `non-baseline live`
- Keep `CommController` and `CommEgressMux` as access-gating owners only; move
  live-content governance into component-owned scheduled versus GET/readback
  behavior.
- Reduce scheduled live surfaces for `EPS`, `GPS`, `partial ADCS`, `RADIO`,
  and `STORAGE` to summary or critical-transition visibility.
- Strengthen `RADIO_GET_STATUS` into the same fresh domain-applied readback
  family used by `EPS_GET_STATUS`, `GPS_GET_STATE`, and `ADCS_GET_ATTITUDE`.
- Change `STORAGE_GET_STATUS` to fresh-scan readback and retire
  `STORAGE_SCAN_NOW`.
- Update hosted and target node-`5` proofs so they prove curated post-auth live
  summary plus bounded detailed readback, not just auth-gated access.
- Update current docs/specs to mark `systemResources`, transport/queue/driver
  internals, and remaining COMM internals as `non-baseline live` rather than
  current operator baseline truth.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: distinguish access gating from current content governance
  and tighten `RADIO_*` readback semantics.
- `eps-subsystem`: keep scheduled EPS live visibility summary-only while
  preserving fresh detailed `EPS_GET_STATUS`.
- `gps-subsystem`: keep scheduled GPS live visibility summary-only while
  preserving fresh detailed `GPS_GET_STATE`.
- `adcs-subsystem`: keep scheduled ADCS live visibility summary-only while
  preserving fresh detailed `ADCS_GET_ATTITUDE`.
- `storage-health`: make `STORAGE_GET_STATUS` the fresh readback command and
  retire `STORAGE_SCAN_NOW`.
- `resource-storage`: record that scheduled storage cache refresh remains
  background state maintenance, while operator readback is now fresh-by-default
  through `STORAGE_GET_STATUS`.
- `interface-contract-index`, `onboard-data-products-and-live-beacon`, and
  `verification-path-registry`: describe the new current tiers and the new
  curated hosted/target observability proofs.

## Impact

- Affected code:
  - `OBC/Components/EpsBridge`
  - `OBC/Components/AdcsBridge`
  - `OBC/Components/GpsBridge`
  - `OBC/Components/RadioController`
  - `OBC/Components/StorageHealthBridge`
  - `OBC/Components/CommandIngressAuthority`
  - focused hosted/target node-`5` observability probes
- Affected docs:
  - current architecture, interfaces, roadmap, verification/evidence, and
    thesis companion references
- Non-goals:
  - no `TlmPacketizer`
  - no generic event/tlm filter framework
  - no UHF command-paced observability implementation
  - no redesign of secure auth, command authority, or telemetry schemas
