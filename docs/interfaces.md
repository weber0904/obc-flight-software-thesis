# Current Interface Contract Index

Status: canonical public narrative interface index.  
Last reconciled: 2026-07-29.

Formal SHALL requirements live under `openspec/specs/`. This index gives
reviewers a compact map of implemented interfaces and their proof status.

## External Ground Interfaces

| Surface | Current contract | Status |
|---|---|---|
| S-band | CCSDS traffic through gateway and COMM CSP node `5` | implemented and hosted-verified; target previously demonstrated |
| UHF | Governed node `6` non-quiet primary/failover | implemented; target previously demonstrated |
| Secure auth | APID `0x00FE` challenge-response per service | implemented and hosted-verified |
| Secure commands | HMAC-authenticated strict active-session sequence | implemented and hosted-verified |
| File/sequence admission | Auth- and role-gated staging plus official sequencing | implemented and route-verified |
| Mission Console | Dashboard, ops, readback, sequences, files, trends, beacon, packet lab | implemented and hosted-verified |

## Command-Auth Configuration

| Stage | Interface |
|---|---|
| Public source | `config/security/command-auth.example.ini`; known, non-deployable development credentials |
| Hosted setup | `scripts/bootstrap_dev_config.sh` creates ignored `config/security/command-auth.ini` mode `0600` |
| Target packaging | `OBC_PACKAGE_KEYSTORE_PATH=/outside/repo/private.ini bash scripts/package_rpi_bundle.sh` |
| Installed runtime | Fixed `config/security/command-auth.ini` inside the release bundle |
| Provenance | Bundle manifest records installed keystore SHA-256 |
| Forbidden | Runtime `--command-auth*` or `COMMAND_AUTH_*` key injection |

## APID Allocation And Governance

`lib/fprime/default/config/ComCfg.fpp` is the reservation source of truth.
Reserved values do not imply an active operational proof.

| APID | Current policy | Status |
|---|---|---|
| `0x0000` | Command packet | `verified` |
| `0x0001` | Telemetry packet | `verified` |
| `0x0002` | Log/event packet | `verified` |
| `0x0003` | File packet | `verified` |
| `0x0004` | Packetized telemetry | reserved |
| `0x0005` | Data product | reserved |
| `0x0006` | F' idle | reserved |
| `0x00FE` | Handshake | `verified` |
| `0x00FF` | Unknown packet | reserved |
| `0x07FF` | CCSDS idle packet | reserved |
| `>= 0x0800` | Invalid / uninitialized | rejected |

## MTU And Payload Ceilings

| Path | Ceiling | Status | Derivation / proof class | Notes |
|---|---:|---|---|---|
| Hosted CCSDS ground frame | `4096` bytes | `configured-hosted` | Current hosted GDS / gateway framing configuration | Hosted framing fact only; not a generic admitted inner-payload ceiling |
| Command envelope fixed overhead | `60` bytes | `verified` | `28-byte header + 32-byte MAC`; structure-only source fact | Envelope structure overhead only; not a path MTU |
| `sband-primary` command ingress | `446` serialized inner `Fw::CmdPacket` bytes | `verified` | `FW_CMD_ARG_BUFFER_MAX_SIZE 506 - envelope overhead 60`; current repo-local source-derived ceiling | Path scope reused from hosted S-band primary command ingress evidence |
| `sband-primary` command ingress | `440` inner command-argument bytes | `verified` | `446 - descriptor 2 - opcode 4`; readable derived inner-argument ceiling | Same current path as the governing serialized-inner ceiling above |
| `uhf-backup` command ingress | `446` serialized inner `Fw::CmdPacket` bytes | `verified` | Same command-envelope formula as `sband-primary`; backup ingress uses the same current outer command budget | Path scope reused from bounded UHF backup command-ingress evidence |
| `uhf-backup` command ingress | `440` inner command-argument bytes | `verified` | Same readable derived ceiling as `sband-primary` | Same current path as the governing serialized-inner ceiling above |
| Active stock file/downlink packet ceiling | `2019` file-data bytes per `Fw::FilePacket::DATA` packet | `configured-hosted` | `FW_FILE_BUFFER_MAX_SIZE 2032 - DataPacket::HEADERSIZE 11 - descriptor 2`; stock current FileDownlink formula | Current global file-packet budget after the payload dual-artifact uplift; adjacent UHF throughput claims remain bounded separately |
| Bounded reliable-transfer helper `DATA` segment | `160` bytes | `verified` | Current sidecar helper-path segment size only | Not a generic repo transport MTU; applies only to the bounded reliable-transfer sidecar path on default node `5` and explicit-switched node `6`; distinct from the stock UHF `243`-byte file/downlink ceiling |
| Generic internal libcsp service payload ceiling | `residual-gap` | `residual-gap` | No current repo-owned ceiling freeze | Do not infer from generic libcsp docs alone |

## Residual Boundaries

| Surface | Current status | Boundary |
|---|---|---|
| Broader target-flight MTU truth beyond the current governed paths | `residual-gap` | The repo now freezes only the current path-specific command and file ceilings; broader target-flight or cross-path MTU closure still needs later proof |
| Future APID expansion beyond the current reservation map | `residual-gap` | The current reservation and proof split are frozen, but later active APID claims still require their own governed change and evidence |

## Internal CSP Services

| Node family | Owner | Representative behavior |
|---|---|---|
| OBC node `1` | active OBC runtime | CSP routing owner and subsystem client |
| EPS node `2` | EPS simulator/flight adapter | power status, controls and fault response |
| ADCS node `3` | ADCS simulator/flight adapter | state, pointing and recovery controls |
| S-band node `5` | COMM service | default CCSDS packet path |
| UHF node `6` | COMM service | non-quiet UHF packet and failover path |
| Payload virtual node | payload backend/service | capture and sensor-control boundary |

Transport carrier, CSP node identity, service semantics, and mission authority
are separate layers. A proof on ZMQ, TCP, SocketCAN, or UART cannot be silently
reused for an adjacent carrier.

## Mission And Recovery Interfaces

- Mode changes are mediated by the current mode-safety owner.
- TTC and payload flows use official command/sequence surfaces.
- Detectors publish local fault truth.
- Recovery executors perform bounded subsystem reset, safe fallback, process
  restart, or watchdog actions.
- Hardware watchdog reset is distinct from hosted process recovery.

## Data Products

- Official F Prime `.fdp` files are the active mission-history mechanism.
- HK chunks expose explicit record identity, chunk metadata, flush/status
  controls and bounded transfer policy.
- Payload preview/raw files remain separate artifacts with explicit source and
  decode provenance.
- Events, live telemetry, explicit readback and persistent products are not
  interchangeable evidence tiers.

## Fact Status

- `verified`: exact current hosted release path was rerun
- `previously demonstrated`: recorded target/lab path at its original commit
- `implemented`: code exists but this release does not claim a fresh path proof
- `planned`: target design only
