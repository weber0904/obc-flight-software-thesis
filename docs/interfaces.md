# Interface Contracts

This document summarizes the active interfaces of the
`OBC/TopCcsds/topology.fpp` deployment. Normative requirements are maintained
under [`openspec/specs/`](../openspec/specs/).

## Ground Links

| Surface | Contract |
|---|---|
| S-band | CCSDS command, telemetry, event, and file traffic through the ground gateway and COMM CSP node `5` |
| UHF | CCSDS traffic through COMM CSP node `6`, with link policy for primary operation and autonomous failover |
| Handshake | APID `0x00FE` challenge-response exchange scoped by service and source |
| Secure command | HMAC-authenticated envelope with active-session and monotonic-sequence validation |
| Mission Console | Dashboard, operations, readback, sequence, file, trend, beacon, and packet-lab views |

Transport carrier, CSP node identity, application service, and mission
authority are independent layers. Each test record identifies the complete
path it exercises.

## Command Authentication Configuration

| Stage | Interface |
|---|---|
| Source example | `config/security/command-auth.example.ini` |
| Hosted setup | `scripts/bootstrap_dev_config.sh` creates ignored `config/security/command-auth.ini` with mode `0600` |
| Target package | `OBC_PACKAGE_KEYSTORE_PATH=/absolute/path/private.ini bash scripts/package_rpi_bundle.sh` |
| Installed runtime | `config/security/command-auth.ini` inside the installed bundle |
| Provenance | Package manifest records the installed keystore SHA-256 |

Keys are fixed at package time. Runtime command-line and environment-variable
key injection are rejected.

## APID Allocation

`lib/fprime/default/config/ComCfg.fpp` is the allocation source of truth.

| APID | Packet class | Operational use |
|---|---|---|
| `0x0000` | Command packet | active |
| `0x0001` | Telemetry packet | active |
| `0x0002` | Log/event packet | active |
| `0x0003` | File packet | active |
| `0x0004` | Packetized telemetry | reserved |
| `0x0005` | Data product | reserved |
| `0x0006` | F' idle | reserved |
| `0x00FE` | Handshake | active |
| `0x00FF` | Unknown packet | reserved |
| `0x07FF` | CCSDS idle packet | reserved |
| `>= 0x0800` | Invalid / uninitialized | rejected |

## Payload Ceilings

| Path | Ceiling | Derivation |
|---|---:|---|
| Hosted CCSDS ground frame | `4096` bytes | GDS and gateway framing configuration |
| Authenticated command envelope | `60` bytes overhead | `28`-byte header plus `32`-byte MAC |
| S-band serialized inner `Fw::CmdPacket` | `446` bytes | command argument buffer `506` minus envelope overhead `60` |
| S-band inner command arguments | `440` bytes | serialized ceiling minus descriptor `2` and opcode `4` |
| UHF serialized inner `Fw::CmdPacket` | `446` bytes | shared authenticated command envelope budget |
| UHF inner command arguments | `440` bytes | serialized ceiling minus descriptor `2` and opcode `4` |
| Stock file-downlink data | `2019` bytes per `Fw::FilePacket::DATA` | file buffer `2032` minus packet header `11` and descriptor `2` |
| Reliable-transfer helper `DATA` segment | `160` bytes | sidecar protocol segment size on node `5` or explicit node `6` |

The values above are path-specific. A new transport or packet format defines
and verifies its own ceiling before admission.

## Internal CSP Services

| Node | Owner | Services |
|---:|---|---|
| `1` | OBC runtime | CSP routing owner and subsystem client |
| `2` | EPS | power telemetry, controls, and fault response |
| `3` | ADCS | attitude state, pointing, and recovery controls |
| `5` | S-band COMM | primary CCSDS packet service |
| `6` | UHF COMM | UHF packet and failover service |
| virtual payload node | payload backend | capture and sensor-control boundary |

Hosted services can use ZMQ or TCP; target services can use TCP, SocketCAN, or
UART adapters according to the deployment configuration.

## Mission And Recovery

- Mode transitions pass through the mode-safety policy.
- TTC windows and payload operations use official commands and sequences.
- Detector components publish local fault state.
- Recovery executors perform bounded subsystem reset, safe-mode entry, process
  restart, or watchdog action.
- The hardware watchdog is supervised independently from hosted process
  recovery.

## Data Products

- F Prime `.fdp` files are the mission-history storage format.
- Housekeeping chunks carry record identity, chunk metadata, flush state, and
  transfer status.
- Payload preview and raw captures are separate files with source and decode
  provenance.
- Live telemetry, events, explicit readback, and persistent products have
  separate interfaces and retention behavior.
