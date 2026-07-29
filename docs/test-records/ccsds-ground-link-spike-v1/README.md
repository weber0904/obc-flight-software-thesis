# ccsds-ground-link-spike-v1 Evidence

## Scope

This record proves a bounded hosted CCSDS ground-link spike through S-band COMM node `5`:

```text
fprime-cli
  -> fprime-gds with space-packet-space-data-link framing
  -> ground_ttc_gateway raw byte relay
  -> simulated S-band TCP segment
  -> sband_comm_csp_node node 5
  -> governed internal CSP substrate
  -> OBC_CcsdsGroundLinkSpike
  -> CCSDS downlink back through the same hosted S-band path
  -> fprime-gds file storage
```

The default `OBC` deployment remains a stock `ComFprime` topology. Existing S-band and UHF records remain `ComFprime` gateway baselines and are not reclassified as CCSDS evidence.

Newly proven scope:

- bounded command uplink through CCSDS TC framing to `OBC_CcsdsGroundLinkSpike`
- bounded event and telemetry downlink through CCSDS TM framing
- bounded housekeeping archive file/downlink through CCSDS framing
- `ground_ttc_gateway` transparent raw-byte relay compatibility for CCSDS-framed traffic
- received `hk-index.csv` and two selected `hk-slot-*.bin` files byte-matched against OBC runtime source snapshots

## Not Covered

- topology-wide migration of default `OBC` from `ComFprime` to `ComCcsds`
- UHF CCSDS evidence
- RF behavior or real radio behavior
- reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
- target hardware behavior
- Raspberry Pi deployment
- command authority, failover policy, pass scheduling, or contact automation
- arbitrary onboard file path downlink

## Implemented Entry Points

- `OBC_CcsdsGroundLinkSpike`
- `OBC/TopCcsds` importing `ComCcsds.Subtopology`
- `scripts/run_ccsds_ground_link_spike_probe.sh`
- `scripts/run_comm_ttc_file_downlink_probe.sh` with optional `OBC_BINARY_NAME`, `COMMAND_PREFIX`, `DICT_PATH`, and `GDS_FRAMING_SELECTION`
- `scripts/run_ground_gds_only_stack.sh` with optional CCSDS GDS framing arguments

## Hosted CCSDS Verdict

Verdict: `PASS` for bounded CCSDS command/event/telemetry, bounded housekeeping archive file/downlink, and gateway raw-byte compatibility.

Post-gate passing hosted run:

| Field | Value |
|---|---|
| date | `2026-05-07` |
| command | `bash scripts/run_ccsds_ground_link_spike_probe.sh` |
| formal verdict | `ccsds-hosted-sband-proof` |
| link mode | `hosted-sband-tcp` |
| log directory | `/tmp/obc-ccsds-ground-link-spike.8dY01t` |
| OBC binary | `OBC_CcsdsGroundLinkSpike` |
| command prefix | `OBCAppCcsds` |
| GDS framing | `space-packet-space-data-link` |
| SCID | `0x44` (`68` passed to `fprime-gds --scid`) |
| VCID | `1` |
| TM frame size | `1024` |
| S-band TCP endpoint | `127.0.0.1:18520` |
| GDS ports | `50520` / `50521` |
| CSP hub ports | `56520` / `57520` |
| runtime root | `/tmp/ccsds-ground-link-spike-runtime` |
| GDS file storage | `/tmp/ccsds-ground-link-spike-gds-downlink` |
| COMM executable | `sband_comm_csp_node` |
| COMM node | `5` |
| HK capture attempts | `10` |
| selected occupied slots | `2` |

Observed GDS CCSDS startup:

```text
GDS framing    : space-packet-space-data-link
comm: Starting uplinker/downlinker connecting to FSW using ip with space-packet-space-data-link
```

Observed final probe markers:

```text
ccsds-ground-link-spike-probe: PASS
formal-verdict=ccsds-hosted-sband-proof
comm-node=5
framing=space-packet-space-data-link
scid=0x44
vcid=1
frame-size=1024
```

## APID and Sequence Notes

The spike uses the upstream F' v4.1.0 `ComCcsds` subtopology and project default `ComCfg.Apid` assignments:

| Flow | APID |
|---|---:|
| command | `0` |
| telemetry | `1` |
| log/event | `2` |
| file | `3` |

Runtime proof by APID-bearing flow:

- command APID `0`: `EPS_SET_PDU` and `ADCS_SET_MODE` were accepted by the CCSDS uplink path and dispatched by `CdhCore.cmdDisp`.
- telemetry APID `1`: `OBCAppCcsds.groundLinkDriver.GROUND_LINK_TX_BYTES` telemetry returned over the CCSDS downlink path.
- log/event APID `2`: `OpCodeDispatched` and `OpCodeCompleted` events returned over the CCSDS downlink path.
- file APID `3`: `HK_DOWNLINK_INDEX` and `HK_DOWNLINK_SLOT` files returned over the CCSDS downlink path and byte-matched source snapshots.

The stock GDS and flight logs used by this probe do not print per-APID sequence counters. `ComCcsds` owns per-APID sequence management through `ComCcsds.apidManager`; a future broader adoption gate should add a frame decoder or raw-frame capture if sequence-counter deltas must be independently asserted in evidence.

## File Matches

The hosted run byte-matched the housekeeping archive index and two occupied archive slot files against hosted OBC runtime source snapshots.

| File | Source snapshot | Received path | Bytes | SHA-256 |
|---|---|---|---:|---|
| `hk-index.csv` | `/tmp/obc-ccsds-ground-link-spike.8dY01t/source-snapshots/source-hk-index.csv` | `/tmp/ccsds-ground-link-spike-gds-downlink/fprime-downlink/hk-index.csv` | `320` | `3c7c2e13b9d187a89a2e2887c9846d27df27011035457908c3948da078113272` |
| `hk-slot-00-g000000.bin` | `/tmp/obc-ccsds-ground-link-spike.8dY01t/source-snapshots/source-hk-slot-00-g000000.bin` | `/tmp/ccsds-ground-link-spike-gds-downlink/fprime-downlink/hk-slot-00-g000000.bin` | `4030` | `6249cc6948976a9518e0d1ecbbbaef798509b326314c60a0674021a42c3f601e` |
| `hk-slot-01-g000000.bin` | `/tmp/obc-ccsds-ground-link-spike.8dY01t/source-snapshots/source-hk-slot-01-g000000.bin` | `/tmp/ccsds-ground-link-spike-gds-downlink/fprime-downlink/hk-slot-01-g000000.bin` | `1223` | `b45828e4a07e846acf8da3d446b13787d1382f547ce84aec447abb27aeeb350c` |

## TT&C Proof

The probe sent bounded commands through the CCSDS-hosted S-band path:

```text
OBCAppCcsds.epsBridge.EPS_SET_PDU --arguments 2 true
OBCAppCcsds.adcsBridge.ADCS_SET_MODE --arguments POINTING
```

Observed OBC readback confirmed the command effects and selected COMM node:

```text
Ground link via COMM CSP node: 5
groundLink mode=comm-csp commNode=5
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.02,-0.01,0.01) pointingErr=0.00
groundLink connected=yes tx=11264 rx=161 txErr=0 rxErr=1
```

The run also observed command dispatch/completion events and nonzero `OBCAppCcsds.groundLinkDriver.GROUND_LINK_TX_BYTES` telemetry through `fprime-cli`.

## Recommendation

Recommendation: `adopt now` for the bounded hosted S-band CCSDS proof path.

This means future CCSDS work can use `OBC_CcsdsGroundLinkSpike` and `scripts/run_ccsds_ground_link_spike_probe.sh` as the current hosted proof baseline. It does not authorize a default `OBC` migration, UHF CCSDS claims, target/Pi deployment claims, RF claims, reliable transfer claims, or command authority/failover policy claims.

## Verification

| Step | Command | Result |
|---|---|---|
| OpenSpec change validation | `openspec validate ccsds-ground-link-spike` | PASS |
| OpenSpec spec validation | `openspec validate --specs` | PASS |
| CCSDS generate/build path | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f`; `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build` | PASS |
| focused groundlink unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_groundlink_unit_test` | PASS |
| focused model unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_sim_model_unit_test` | PASS |
| initial hosted CCSDS proof | `bash scripts/run_ccsds_ground_link_spike_probe.sh` | PASS |
| stock S-band `ComFprime` regression | `bash scripts/run_sband_tcp_ground_link_probe.sh` | PASS |
| UHF node `6` regression | `bash scripts/run_uhf_uart_backup_link_probe.sh` | PASS |
| full local gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-ccsds-ground-link-spike` | PASS |
| post-gate hosted CCSDS proof | `bash scripts/run_ccsds_ground_link_spike_probe.sh` | PASS |
