# uhf-ccsds-hosted-adoption-v1 Evidence

Date: 2026-05-16.

OpenSpec change: `uhf-ccsds-adoption-v1`.

## Scope

This record proves the active hosted UHF node `6` path now uses CCSDS framing on
the default `OBC` / `TopCcsds` baseline after the governed active COMM runtime
explicitly switches the command, telemetry, and file roles to UHF.

The proof keeps historical UHF `ComFprime` evidence separate. It does not reinterpret `uhf-uart-backup-link-v1` as if it had always been CCSDS-backed.

Proven path after explicit switch to `UHF primary`:

```text
fprime-cli
  -> fprime-gds (CCSDS)
  -> ground_ttc_gateway (link=uhf serial)
  -> hosted serial
  -> uhf_comm_csp_node node 6
  -> CSP hub
  -> hosted OBC / TopCcsds
```

The switch itself is a prerequisite performed over the default hosted S-band CCSDS path via stage 2 of `run_comm_session_and_downlink_qos_probe.sh`. This record does not claim standalone UHF bootstrap, standalone UHF session-open authority, or an all-UHF proof boundary from cold start.

This evidence newly proves:

- active `TopCcsds` no longer depends on `OBCComFprime` for the UHF path
- active UHF command/file/downlink traffic can run on CCSDS after explicit
  switch to `UHF primary`
- UHF capture decode shows `SCID=0x44`, `VCID=2`, `TM frame size=1024`, and the governed command/file CCSDS classes needed by the current hosted UHF adoption boundary

Current semantic note:

- this record predates the later UHF-primary packet-quiet decoupling baseline
- it should not be cited as current proof that live `event/tlm` packet egress
  remains visible on the formal UHF path after the switch
- use the later packet-quiet proof path for current formal UHF live-packet
  suppression truth

## Not Covered

- RF behavior
- reliable transfer, ARQ/NACK, retransmission, or CFDP
- Raspberry Pi / target hardware closure
- historical legacy retirement
- broader authority expansion beyond the already governed active COMM/runtime policies

## Hosted Probe

Repository-owned proof entry point:

```bash
bash scripts/run_uhf_ccsds_hosted_adoption_probe.sh
```

The wrapper reuses the active stage-2 `UHF primary-after-switch` runtime from `run_comm_session_and_downlink_qos_probe.sh`, then decodes the raw UHF capture with `scripts/decode_ccsds_capture.py`.

Final passing run:

| Field | Value |
|---|---|
| verdict | `PASS` |
| formal verdict | `ccsds-hosted-uhf-adoption` |
| log root | `/tmp/obc-ccsds-uhf-adoption.vY4pVS` |
| boundary | `CCSDS ground_ttc_gateway serial + uhf_comm_csp_node node 6` |
| SCID | `0x44` |
| VCID | `2` |
| TM frame size | `1024` |

Observed summary markers:

```text
comm-session-and-downlink-qos-probe: PASS
formal-verdict=comm-session-and-downlink-qos
boundary-uhf=CCSDS ground_ttc_gateway serial + uhf_comm_csp_node node 6
case-uhf-primary-full-authority-and-file-downlink=PASS
ccsds-capture-decode: PASS
uhf-ccsds-hosted-adoption-probe: PASS
formal-verdict=ccsds-hosted-uhf-adoption
comm-node=6
framing=space-packet-space-data-link
scid=0x44
vcid=2
frame-size=1024
decoded-apids=command-uplink:0,file-downlink:3
```

## Decode Boundary

Decoded capture summary:

- uplink APID observations: `0`
- downlink APID observations required by the current proof: `3`
- bounded trailing partial bytes at capture end are tolerated by the decoder for the serial path as long as all complete preceding TM frames validate and no unexpected mid-stream bytes remain

Artifacts:

- capture uplink: `/tmp/obc-ccsds-uhf-adoption.vY4pVS/stage2-uhf-primary/captures/uhf-gds-to-southbound.bin`
- capture downlink: `/tmp/obc-ccsds-uhf-adoption.vY4pVS/stage2-uhf-primary/captures/uhf-southbound-to-gds.bin`
- decode summary: `/tmp/obc-ccsds-uhf-adoption.vY4pVS/ccsds-uhf-capture-summary.json`

## Verdict

PASS for active hosted UHF CCSDS adoption on node `6` after explicit switch to `UHF primary`.

This record authorizes citing the active UHF node-`6` path for:

- bounded authenticated command uplink on the active hosted `OBC` path after switch to `UHF primary`
- bounded `DpCatalog`-driven `.fdp` file/downlink over the active hosted UHF path
- decoded CCSDS framing observations for `SCID 0x44`, `VCID 2`, and the governed command/file APID classes required by the current proof

It does not remain current proof for continued live `event/tlm` visibility on
the formal UHF path once UHF-primary packet quiet is part of the active
baseline.
