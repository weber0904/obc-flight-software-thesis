## Context

The current baseline has four relevant truths that this change must preserve:

- hosted CCSDS S-band node `5` is the default active hosted command/event/channel and official `.fdp` path;
- hosted UHF node `6` is already a separate proof boundary, but today it does not participate in the active `TopCcsds` command-policy runtime;
- `CommandIngressAuthority` already owns authenticated envelope verification, `SESSION_OPEN(seq0)` lifecycle, and strict-monotonic runtime sequence;
- `DpCatalog` is the official `.fdp` catalog/history surface, while `HousekeepingArchive` is only a bounded transitional forensic fallback.

What is missing is not more transport foundation. The missing layer is a COMM-owned runtime that turns those surfaces into a single operational policy closure.

## Goals / Non-Goals

**Goals:**

- make COMM own explicit link-role and primary-link runtime state
- keep `CommandIngressAuthority` as the auth/session owner while making runtime command allow/deny dynamic and COMM-driven
- make active `TopCcsds` able to prove both S-band node `5` and UHF node `6` command ingress within one governed baseline
- move HK/DP shared downlink ownership under a deterministic COMM-owned scheduler
- define clear convergence for primary switch, link loss, and observe-only pass transitions
- leave reviewable events, telemetry, and counters that hosted probes can assert

**Non-Goals:**

- no new RF claim, CCSDS-on-UHF claim, or target/Pi hardware claim
- no reliable transfer, CFDP, ARQ/NACK, or arbitrary onboard file downlink
- no persistent replay protection or hardware-backed secure key/session state
- no payload/TTC scheduler, broad FDIR, or general mission-sequence engine

## Decisions

### Decision: Keep a single public COMM owner, but allow internal helpers

`CommController` remains the public runtime owner for commands, telemetry, events, and runtime state. This change may add helper logic or a small COMM-owned internal helper component/class where that reduces coupling, but the PR will still land as one operational capability closure rather than a helper-only PR.

### Decision: `passActive` is observe-only

`COMM_START_PASS` and `COMM_STOP_PASS` continue to describe contact phase and leave reviewable pass state, but they do not gate authenticated command admission or new file ownership. The real policy gate is link availability plus primary-link role.

### Decision: Active baseline becomes dual-ingress

`TopCcsds` keeps the current hosted CCSDS S-band node-`5` branch as the default path and adds a separate bounded UHF node-`6` ingress branch. These two branches stay distinct in topology, runtime logs, and evidence language. The change does not redefine UHF as a CCSDS path.

### Decision: COMM drives dynamic ingress roles; `CommandIngressAuthority` keeps class evaluation

`CommandIngressAuthority` keeps the authenticated envelope/session/sequence contract. COMM supplies each ingress port's current role/profile:

- ingress `0`: S-band path
- ingress `1`: UHF path

`CommandIngressAuthority` uses that runtime profile plus the existing catalog metadata to decide allow/deny. This keeps source-bound auth logic and dynamic COMM policy coupled without duplicating the authority catalog in two places.

### Decision: Default role state is `S-band primary`, `UHF backup`

At startup:

- `primaryCommandLink = S-band`
- `primaryTelemetryLink = S-band`
- `primaryFileLink = S-band`
- UHF is present as a bounded backup command path if available

`COMM_SET_ACTIVE(UHF)` changes all three primary roles to UHF. `COMM_SET_ACTIVE(S_BAND)` changes them back to S-band.

### Decision: `UHF backup` stays low-risk; `UHF primary` becomes full authority

Runtime command policy is:

- `S-band primary`: authenticated full catalog
- `UHF backup`: authenticated `READ_STATUS` plus explicit low-risk allowlist only
- `UHF primary`: authenticated full catalog, including `DATA_PRODUCT` and `FILE_TRANSFER`

This keeps `allow_uhf_simple_commands_while_s_band_busy` true while avoiding accidental file ownership from the backup path.

### Decision: Downlink arbitration is COMM-owned, non-preemptive, and DP-primary

The active shared file/downlink surface becomes:

- `HousekeepingArchive.fileOut -> COMM-owned scheduler`
- `DpCatalog.fileOut -> COMM-owned scheduler`
- `COMM-owned scheduler -> FileHandling.fileDownlink.SendFile`

Scheduling policy:

- only one active owner at a time
- at most one pending request at a time
- pending is reserved for `DpCatalog`
- active `DpCatalog` causes new HK requests to be rejected `BUSY`
- active HK allows one pending `DpCatalog`; when HK completes, COMM immediately launches the pending DP request
- no preemption

Because `DpCatalog` is the official history/catalog surface, it is the only source that may wait pending instead of being rejected.

### Decision: COMM owns completion mapping and transition cleanup

`DpCatalogFileDownlinkGate` leaves the active path. The new COMM-owned scheduler tracks actual `FileDownlink` context, active owner, pending request, and owner link. It is responsible for:

- forwarding matching completion to `DpCatalog.fileDone`
- clearing owner state on completion or cancel
- dropping pending state on owner-link loss or primary-file-link switch
- emitting explicit events/counters for owner start, busy reject, pending set, completion, drop, and failover cleanup

### Decision: Link availability drives convergence

Link availability is fed into COMM runtime from the bounded active topology path. When the active primary link becomes unavailable:

- primary command/telemetry/file roles move to the surviving policy state
- any authenticated sessions bound to the old primary role are revoked
- any active file owner on the lost primary file link is aborted/cleared
- new ownership claims are rejected until the new primary file link is available

This is the first operational failover policy claim, but it remains limited to hosted evidence.

## Runtime Shape

`CommController` runtime state grows to include:

- per-link availability for S-band and UHF
- primary command/telemetry/file role
- pass state and transition reason
- per-ingress current authority profile
- active downlink owner type/link/context
- optional pending DP request metadata
- counters for policy allow/deny, session revoke, owner busy reject, owner complete, and owner drop

`CommandIngressAuthority` runtime state grows to include:

- current COMM-driven profile per ingress
- session revoke reason/event when COMM invalidates an existing session due to role/link transition

## Risks / Trade-offs

- **[Risk] Dual-ingress active topology could blur proof boundaries.** Mitigation: keep S-band node `5`, UHF node `6`, command-policy proof, and file/downlink proof as separate evidence sections and separate registry language.
- **[Risk] Moving completion ownership out of `DpCatalogFileDownlinkGate` could regress catalog callbacks.** Mitigation: keep direct completion-mapping tests plus classic `CommController` L2 coverage for DP completion forwarding.
- **[Risk] UHF primary file/downlink could be read as reliable UHF transfer.** Mitigation: bound the evidence to current stock `FileDownlink` behavior only and explicitly exclude reliable transfer and arbitrary file claims.

## Migration Plan

1. Add and validate the OpenSpec change artifacts and delta specs.
2. Extend `CommController` contract/state and add COMM-owned policy/downlink scheduling helpers as needed.
3. Integrate `CommandIngressAuthority` with COMM-driven dual-ingress profiles and revocation hooks.
4. Rewire active `TopCcsds` so both ingress paths and the shared downlink scheduler are live on the active baseline.
5. Add L1/L2 coverage, then add the new bounded hosted probe and evidence.
6. Run fresh verification, focused regression probes, OpenSpec validation, and doc/evidence updates.

Rollback strategy: revert to the current single-owner `CommController` shell, restore direct HK/DP file-downlink wiring, and remove the new UHF-primary file/downlink evidence path while keeping existing node-`5`/node-`6` baseline proofs intact.

## Open Questions

- none
