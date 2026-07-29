## Context

The current repository has four relevant but distinct baselines:

- direct `GDS -> TCP -> OBC`, used for hosted development but not valid UHF evidence
- generic gateway-backed COMM node `4`, preserved for compatibility
- S-band TCP COMM node `5`, proven for hosted S-band TT&C and bounded housekeeping archive file/downlink
- UHF COMM node `6`, currently proven only for identity/coexistence and services `30-32`

This change builds the first UHF-specific hosted path. It uses PTY serial pairs as macOS UART/USB/RS485 stand-ins, keeps command ingress on the existing COMM CSP service contract, and adds a UHF-only beacon side channel so beacon proof does not get confused with command/downlink chunk services.

## Goals / Non-Goals

**Goals:**

- use `uhf_comm_csp_node` as the only COMM node in the UHF v1 proof path
- prove bounded backup command ingress over hosted serial with OBC configured for `COMM_CSP_NODE=6`
- prove at least one BeaconV1 frame is emitted through a UHF node-6 side channel and decoded from a hosted serial capture
- keep launcher, probe, logs, evidence, and registry entries explicit about UHF link identity and node `6`
- preserve node `4` generic compatibility and node `5` S-band TCP behavior

**Non-Goals:**

- no S-band TCP or direct `GDS -> TCP -> OBC` evidence reuse
- no full UHF command authority, autonomous failover policy, or security/authority model
- no file/downlink, arbitrary onboard file transfer, reliable transfer, NACK/ARQ, or packet-loss recovery claim
- no CCSDS, APID mapping, `ComCcsds`, custom GDS plugin, RF, modem, antenna, target hardware, Raspberry Pi deployment, or physical USB/RS485 electrical validation

## Decisions

### Decision: UHF v1 includes both bounded ingress and beacon

The v1 proof includes two bounded evidence surfaces:

- backup command ingress: `fprime-cli -> fprime-gds -> ground_ttc_gateway(link=uhf serial) -> PTY -> uhf_comm_csp_node(node 6) -> internal CSP -> hosted OBC`
- beacon side channel: `OBC BeaconPublisher -> internal CSP beacon push -> uhf_comm_csp_node(node 6) -> hosted beacon PTY -> capture/decode`

These are recorded separately to avoid implying full co-channel multiplexing, failover, full command authority, or arbitrary downlink.

### Decision: Command ingress reuses services 30-32 unchanged

UHF backup command ingress uses existing services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`. The node id changes to `6`; service ports and packet layouts do not change.

### Decision: Beacon uses a UHF side channel, not services 30-32

Beacon frames are link-local broadcast-style payloads and do not require ground acknowledgement. The UHF beacon path will therefore use an explicit bounded node-6 beacon side channel rather than overloading the command/downlink chunk services. The implementation may add a small internal CSP service for BeaconV1 push from OBC to `uhf_comm_csp_node`, plus a UHF beacon serial output endpoint for hosted capture.

### Decision: Hosted PTY serial is the formal v1 medium

The formal evidence uses isolated PTY serial pairs so the probe is repeatable on macOS without physical devices. Evidence wording must call this a hosted UART/USB/RS485 stand-in and must not claim electrical RS485, real USB serial hardware, Pi, target hardware, or RF.

### Decision: Stock `ComFprime` and stock GDS remain northbound

The gateway continues to speak stock F' framing toward `fprime-gds`, and OBC remains in `comm-csp` ground-link mode. No CCSDS migration, GDS plugin, or northbound protocol change is part of this path.

## Risks / Trade-offs

- **[Risk] Beacon side channel could be mistaken for full UHF downlink.** Mitigation: evidence records only BeaconV1 capture/decode and explicitly excludes file/downlink, reliable transfer, and arbitrary downlink.
- **[Risk] Backup command ingress could be mistaken for full command authority.** Mitigation: probe uses bounded EPS/ADCS command/readback and event/channel assertions only; failover and authority policy stay future work.
- **[Risk] Hosted PTY can be mistaken for physical USB/RS485 validation.** Mitigation: registry and evidence state that PTY serial is a hosted stand-in and physical hardware remains unproven.
- **[Risk] Shared COMM code changes could regress node `4` or S-band node `5`.** Mitigation: rerun dual-link foundation and S-band TCP probes before closeout.

## Migration Plan

1. Create and validate OpenSpec artifacts, then stop for scope review before implementation.
2. Add UHF beacon-side-channel support with explicit node-6 logs and configuration.
3. Add the hosted UHF probe with isolated PTYs, ports, runtime roots, and logs.
4. Update docs, evidence, registry, and reconciliation matrix after the probe is stable.
5. Verify with focused tests, UHF probe, regression probes for node `4`/node `5`, full verification CI, OpenSpec validation, and archive.

Rollback strategy: remove the UHF beacon-side-channel wiring and UHF probe while leaving existing generic node `4`, S-band node `5`, and UHF services `30-32` foundation behavior intact.

## Open Questions

- none
