# Transport MTU And APID Governance V1

Status: PASS
Change: `transport-mtu-apid-governance-v1`
Date: 2026-05-29

## Purpose

Freeze the current active-baseline transport payload ceilings and APID
allocation governance needed by later COMM work without reopening reliable
transfer, dual-link orchestration, gateway ownership, or UHF runtime-policy
scope.

This record separates:

1. numeric ceiling derivation from checked-in source/constants
2. reused path-scope proof from existing registry entries and archived evidence
3. residuals and explicit non-claims

The repo-owned checker for this change now parses the current `ComCfg.Apid`
enum and the stock `FilePacket` header-size expressions directly from checked-in
source before it verifies the `docs/interfaces.md` contract rows.

## Numeric Ceiling Derivation

### S-band primary command ingress

- Governing transport unit: serialized inner `Fw::CmdPacket`
- Current ceiling: `446` bytes
- Readable derived inner command-argument ceiling: `440` bytes
- Budget components:
  - `FW_COM_BUFFER_MAX_SIZE = 512`
  - `SIZE_OF_FwOpcodeType = 4`
  - `SIZE_OF_FwPacketDescriptorType = 2`
  - `FW_CMD_ARG_BUFFER_MAX_SIZE = 512 - 4 - 2 = 506`
  - command-envelope fixed overhead `= 28 + 32 = 60`
  - admitted inner serialized command `= 506 - 60 = 446`
  - admitted inner arg payload `= 446 - 2 - 4 = 440`

### UHF backup command ingress

- Governing transport unit: serialized inner `Fw::CmdPacket`
- Current ceiling: `446` bytes
- Readable derived inner command-argument ceiling: `440` bytes
- Budget components: same current command-envelope and outer-command budget as
  `sband-primary`

### UHF primary-after-failover official file/downlink

- Governing transport unit: file data bytes per stock `Fw::FilePacket::DATA`
  packet
- Current ceiling: `243` bytes
- Budget components:
  - `FW_FILE_BUFFER_MAX_SIZE = 256`
  - `Fw::FilePacket::DataPacket::HEADERSIZE = 11`
  - `sizeof(FwPacketDescriptorType) = 2`
  - admitted file data `= 256 - 11 - 2 = 243`

### Reliable-transfer bounded non-generalization

- Current helper-path fact: `160` bytes per reliable-transfer `DATA` segment
- Scope: bounded default S-band node-`5` reliable-transfer sidecar only
- Explicit non-claim: not a generic repo transport MTU and not the governing
  command/file ceiling for other paths

## Reused Path-Scope Proof

These existing records are cited only to prove path identity or active APID
flow scope, not to derive the numeric ceiling values above.

- Registry entry `43`: default hosted CCSDS S-band node-`5` path identity and
  decoded APID flows `0/1/2/3`
- Registry entry `43A`: hosted UHF node-`6` CCSDS path identity, `VCID = 2`,
  and official UHF file/downlink path scope after explicit switch
- Registry entry `45`: authenticated command ingress path scope for
  `sband-primary` and `uhf-backup`
- Registry entry `60`: target/lab quiet node-`6` `uhf-backup` and
  `uhf-primary-after-failover` command and official file/downlink path scope

Current APID governance therefore freezes:

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
  - `0x07FF` idle packet
- invalid boundary:
  - `>= 0x0800`

## Residuals And Non-Claims

This change intentionally does **not** freeze:

- broader target-flight or cross-path universal MTU truth
- generic libcsp service payload ceilings
- UHF reliable transfer, ARQ, NACK, or CFDP behavior
- gateway-owned simultaneous S-band/UHF multiplexer behavior
- one-stock-GDS heterogeneous multi-upstream aggregation
- future APID expansion beyond the current reservation map

## Closeout-Skill Audit Verdict

No `.codex/skills/change-closeout/SKILL.md` change was needed.

Reason:

- the skill already requires current-baseline, next-work, operator, and target
  design updates when those layers actually changed
- it already points to the canonical-doc trigger table in
  `docs/architecture/current-development-architecture.md`
- the recurring drift addressed by this change was the transport/APID contract
  itself, not a concrete ambiguity in the closeout instructions

## Local Verification

| Step | Command | Result |
|---|---|---|
| syntax check | `python3 -m py_compile scripts/check_transport_mtu_apid_contract.py` | PASS |
| transport contract checker | `python3 scripts/check_transport_mtu_apid_contract.py` | PASS |
| full local gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| repo consistency | `python3 scripts/check_repo_consistency.py` | PASS |
| documentation governance | `python3 scripts/check_documentation_governance.py` | PASS |
| OpenSpec change validation | `openspec validate transport-mtu-apid-governance-v1` | PASS |
| OpenSpec specs validation | `openspec validate --specs` | PASS |

## Outcome

The current repo is now in a better position to start later target-side COMM
work because it has:

- a frozen current command ceiling for `sband-primary`
- a frozen current command ceiling for `uhf-backup`
- a frozen current official file/downlink ceiling for
  `uhf-primary-after-failover`
- a frozen current APID reservation / proof-split policy
- explicit residuals instead of hidden transport or APID guesswork
