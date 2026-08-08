## Context

The active baseline already freezes several adjacent transport facts:

- `ground_ttc_gateway` and the hosted GDS stack use CCSDS
  `space-packet-space-data-link` framing with `SCID = 0x44`
- the hosted S-band path is `VCID = 1`
- the active hosted and target/lab UHF node-`6` path is `VCID = 2`
- `CommandEnvelopeMetadata` defines a fixed `60`-byte envelope overhead
- the project-local buffer model keeps `FW_COM_BUFFER_MAX_SIZE = 512`
  and `FW_FILE_BUFFER_MAX_SIZE = 256`

What remained open was not the existence of those facts, but the governed
current contract that later work should rely on:

- the actual admitted inner command ceiling for `sband-primary`
- the actual admitted inner command ceiling for `uhf-backup`
- the actual official file/downlink payload ceiling for
  `uhf-primary-after-failover`
- the current APID reservation and change-control policy

This change freezes only that narrow contract. It does not claim simultaneous
dual-link proof, RF closure, UHF reliable transfer, gateway multiplexer
ownership, or a broader target-flight universal MTU.

## Goals / Non-Goals

**Goals**

- Freeze the current path-specific transport ceilings needed by the active
  baseline.
- Freeze the current APID reservation / allocation governance truth.
- Make the numeric ceiling derivation reviewable from checked-in source.
- Make `docs/interfaces.md` readable in one pass without overclaiming hosted
  framing facts or bounded helper-path limits.

**Non-Goals**

- Rework `FW_COM_BUFFER_MAX_SIZE`, `FW_FILE_BUFFER_MAX_SIZE`, or stock F'
  serializer behavior.
- Redesign UHF quiet, beacon suppress, gateway switching, or reliable transfer.
- Claim generic libcsp payload ceilings, RF closure, or universal target-flight
  transport truth beyond the governed current paths.
- Modify `.codex/skills/change-closeout/SKILL.md` unless a concrete ambiguity is
  found while implementing the doc updates.

## Decisions

- **Freeze command ingress in two units.** The governing transport unit is the
  complete serialized inner `Fw::CmdPacket` length, and the readable derived
  unit is the inner command-argument length.
- **Treat numeric ceilings as source-derived, not evidence-derived.** The
  checker and docs shall derive ceiling values from checked-in constants and
  serializer formulas. Verification-path registry entries and archived evidence
  shall be cited only to prove which operational path each ceiling belongs to.
- **Freeze the current repo-local file/downlink ceiling as `243` bytes.**
  `FW_FILE_BUFFER_MAX_SIZE = 256`, `Fw::FilePacket::DataPacket::HEADERSIZE = 11`,
  and `sizeof(FwPacketDescriptorType) = 2` yield a stock current
  `256 - 11 - 2 = 243` file-data-byte ceiling per `Fw::FilePacket::DATA`
  packet. The old `499` value remains a historical diagnostic failure case, not
  current baseline truth.
- **Freeze APID governance as a reservation map plus proof split.**
  `ComCfg.Apid` is the current baseline reservation source. Active
  path-proven flows are `0..3`; reserved current-code values, special values,
  and invalid ranges remain governed but are not overclaimed as active
  operational flows.
- **Keep reliable-transfer `160` bytes path-local.** The `160`-byte
  reliable-transfer `DATA` segment ceiling belongs only to the bounded default
  S-band node-`5` helper path and shall be documented as such, never as a
  generic repo transport MTU.
- **Leave the closeout skill unchanged unless evidence proves ambiguity.**
  Current repo-local closeout wording already points to canonical-doc review
  triggers in `docs/architecture/current-development-architecture.md`. This
  change will record that no skill clarification was needed unless
  implementation uncovers a concrete mismatch.

## Derived Current Contract

### Command ingress ceilings

- Outer command arg budget:
  `FW_CMD_ARG_BUFFER_MAX_SIZE = FW_COM_BUFFER_MAX_SIZE - SIZE_OF_FwOpcodeType - SIZE_OF_FwPacketDescriptorType`
- Current project values:
  - `FW_COM_BUFFER_MAX_SIZE = 512`
  - `SIZE_OF_FwOpcodeType = 4`
  - `SIZE_OF_FwPacketDescriptorType = 2`
  - therefore `FW_CMD_ARG_BUFFER_MAX_SIZE = 506`
- Envelope fixed overhead:
  - header `28`
  - MAC `32`
  - total `60`
- Current admitted inner serialized command ceiling:
  - `506 - 60 = 446`
- Current admitted inner command-argument ceiling:
  - `446 - 2 - 4 = 440`

These values apply to the current `sband-primary` and `uhf-backup` command
ingress paths because both paths use the same governed outer command-envelope
budget and inner `Fw::CmdPacket` vocabulary.

### Official file/downlink ceiling

- `FW_FILE_BUFFER_MAX_SIZE = 256`
- `Svc::FileDownlink` current `maxDataSize` formula:
  `FILEDOWNLINK_INTERNAL_BUFFER_SIZE - Fw::FilePacket::DataPacket::HEADERSIZE - sizeof(FwPacketDescriptorType)`
- current supporting values:
  - `FILEDOWNLINK_INTERNAL_BUFFER_SIZE = FW_FILE_BUFFER_MAX_SIZE = 256`
  - `Fw::FilePacket::DataPacket::HEADERSIZE = 11`
  - `sizeof(FwPacketDescriptorType) = 2`
- current admitted file-data ceiling:
  - `256 - 11 - 2 = 243`

This value is the current per-packet official file/downlink ceiling for the
stock `Fw::FilePacket::DATA` path, including the active
`uhf-primary-after-failover` quiet official file/downlink proof surface.

### APID governance

Current reservation truth comes from `ComCfg.Apid`:

- active path-proven operational flows:
  - `0x0000` command
  - `0x0001` telemetry
  - `0x0002` log/event
  - `0x0003` file
- reserved current-code values:
  - `0x0004` packetized telemetry
  - `0x0005` data product
  - `0x0006` F' idle
- special reserved values:
  - `0x00FE` handshake
  - `0x00FF` unknown
  - `0x07FF` CCSDS idle packet
- invalid boundary:
  - `>= 0x0800`

Future APID claims shall require a governed change that updates code, current
docs, specs, and evidence together. `ComCcsds.apidManager` remains the current
implementation owner for per-APID sequence counts on the CCSDS path; it is not
the APID allocation authority.

## Evidence Model

The final evidence for this change shall separate three proof classes:

1. **Numeric derivation proof**
   - checked-in constants
   - serializer/header sizes
   - the checker output
2. **Path-scope proof reused from existing evidence**
   - hosted S-band node-`5` CCSDS path identity
   - hosted and target/lab UHF node-`6` path identity
   - current APID flow observations on the governed CCSDS paths
3. **Residuals / non-claims**
   - broader target-flight MTU truth
   - generic libcsp service payload ceilings
   - future APID expansion
   - reliable transfer beyond the bounded default node-`5` helper path

## Risks / Trade-offs

- If any current doc wording still conflates hosted framing facts with admitted
  inner payload ceilings, reviewers may keep inferring a wider MTU than the
  actual repo-local contract. This change resolves that by separating framing,
  derived ceilings, and residuals.
- If APID governance is documented as “current code happens to map this way”
  without change-control wording, later work could bypass review. This change
  resolves that by explicitly freezing the reservation map and future-claim
  process.
- If the checker drifts from docs or source formulas, future doc updates could
  silently regress. The checker therefore validates both the formulas and the
  interface index rows.
