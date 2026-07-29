## Scope Choice

This change is a bounded spike and comparative analysis with hosted proof. It is not a topology-wide migration and it does not claim that existing S-band or UHF evidence is CCSDS-compatible.

The hosted proof path is:

`fprime-cli -> fprime-gds(CCSDS framing) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> OBC CCSDS spike executable`

UHF node `6` remains analysis-only. The default `OBC` deployment continues to import `ComFprime.Subtopology`; the spike uses a separate topology/executable or build target so existing `ComFprime` S-band and UHF baselines remain valid and separately evidenced.

## F' CCSDS Framing Model

F' v4.1.0 provides `Svc/Subtopologies/ComCcsds`, which combines CCSDS Space Packet framing with TC/TM data link framing:

- Downlink: `ComQueue -> SpacePacketFramer -> TmFramer -> comStub`.
- Uplink: `comStub -> FrameAccumulator -> TcDeframer -> SpacePacketDeframer -> FprimeRouter`.
- APIDs come from `ComCfg.Apid`: command `0`, telemetry `1`, log/event `2`, file `3`.
- `ComCfg.SpacecraftId` is `0x44`.
- `ComCfg.TmFrameFixedSize` is `1024`.
- `ComCfg.FrameContext` carries the virtual channel; this spike uses VCID `1`.

The spike topology wires `GroundLinkDriver` to `ComCcsds.comStub` and wires command, event, telemetry, file uplink, and file downlink through `ComCcsds.comQueue` and `ComCcsds.fprimeRouter` equivalents. It keeps hosted OBC runtime in `GROUND_LINK_MODE=comm-csp` with `COMM_CSP_NODE=5`; direct GDS TCP remains disabled or outside the verdict boundary.

## Ground Gateway Compatibility Model

`ground_ttc_gateway` remains a raw byte relay. It does not parse `ComFprime`, CCSDS Space Packets, TC frames, TM frames, APIDs, SCID, VCID, sequence counts, commands, events, telemetry, or files.

The CCSDS compatibility verdict for `ground_ttc_gateway` can only prove transparent byte movement of CCSDS-framed traffic between the northbound GDS TCP connection and the southbound S-band TCP COMM endpoint. It must not claim gateway-level CCSDS interpretation or validation.

The GDS side uses:

- `--framing-selection space-packet-space-data-link`
- `--scid 0x44`
- `--vcid 1`
- `--frame-size 1024`

## Probe Strategy

Add `scripts/run_ccsds_ground_link_spike_probe.sh` with isolated runtime, file-storage, GDS, CSP hub, S-band node `5`, gateway, and CCSDS spike OBC paths. The probe is formal only after a fresh build/generate path has produced the spike executable.

The probe must assert:

- bounded command uplink for `EPS_SET_PDU` and `ADCS_SET_MODE`;
- OBC readback through command/event/channel observations;
- `GROUND_LINK_TX_BYTES` telemetry over the CCSDS-hosted S-band path;
- downlink of `HK_DOWNLINK_INDEX` and at least two `HK_DOWNLINK_SLOT` files;
- byte-for-byte comparison of received housekeeping files against source snapshots;
- explicit output markers: `formal-verdict=ccsds-hosted-sband-proof`, `comm-node=5`, `framing=space-packet-space-data-link`, `scid=0x44`, `vcid=1`, and `frame-size=1024`.

## Recommendation Criteria

The evidence record must choose one recommendation:

- `adopt now`: command, event, telemetry, bounded file/downlink, and gateway raw-byte compatibility pass without material blockers.
- `defer with blockers`: one or more proof areas fail or require broader migration, reliability, authority, hardware, or gateway semantics work.
- `keep ComFprime with CCSDS-aligned semantics`: CCSDS framing is not adopted now, but mission/session naming, APID planning, path separation, and evidence boundaries remain aligned for later CCSDS adoption.

## Scope Guards

This change must not:

- migrate the default `OBC` topology from `ComFprime` to `ComCcsds`;
- change S-band node `5`, UHF node `6`, or COMM services `30`, `31`, and `32`;
- claim RF, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, command authority, or failover policy;
- describe existing S-band or UHF records as CCSDS evidence unless this spike creates new evidence for that behavior.
