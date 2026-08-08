## Context

The current repository has three relevant but distinct baselines:

- direct `GDS -> TCP -> OBC`, where the hosted OBC connects directly to stock GDS
- generic gateway-backed COMM node `4`, where `ground_ttc_gateway` bridges GDS traffic through a serial ingress into `comm_csp_node`
- dual-link simulator foundation, where `sband_comm_csp_node` exists as S-band node `5` but only proves identity/coexistence and service behavior

This change builds the next S-band path by replacing the lab-side serial stand-in with a S-band simulated TCP segment while keeping the spacecraft-side path as `sband_comm_csp_node(node 5) -> COMM CSP services 30-32 -> OBC`.

## Goals / Non-Goals

**Goals:**

- use `sband_comm_csp_node` as the only COMM node in the S-band path
- make the S-band simulated TCP segment explicit in launcher/probe logs and evidence
- keep OBC in `comm-csp` ground-link mode with `COMM_CSP_NODE=5`
- prove bounded EPS and ADCS command flow, command events, and `GROUND_LINK_TX_BYTES` telemetry through the S-band path
- prove bounded housekeeping archive file/downlink over the same S-band path with received files byte-matched against source snapshots

**Non-Goals:**

- no UHF UART/RS485/USB/macOS backup or beacon behavior
- no CCSDS, APID mapping, `ComCcsds`, or custom GDS plugin work
- no RF, real radio, antenna, modem, target hardware, or Pi deployment claim
- no reliable transfer, NACK/ARQ, packet-loss recovery, or missing-packet retransmission claim
- no arbitrary onboard file path downlink or broad file transfer scope

## Decisions

### Decision: S-band COMM listens, gateway connects

`sband_comm_csp_node` owns the S-band simulated RF endpoint and listens on a configured TCP host/port. `ground_ttc_gateway` connects southbound to that endpoint. This keeps the spacecraft-side S-band identity explicit and prevents the gateway's TCP port from being confused with direct GDS connectivity.

### Decision: Reuse the existing COMM chunk service contract

S-band uses the existing `UPLINK_POLL`, `DOWNLINK_WRITE`, and `LINK_STATUS` services on ports `30`, `31`, and `32`. The node id changes to `5`; service ports and packet layouts do not change.

### Decision: Keep stock `ComFprime` and stock GDS northbound

The gateway continues to speak stock F' framing toward `fprime-gds`. No CCSDS migration, GDS plugin, or northbound protocol change is part of this v1 path.

### Decision: Include bounded housekeeping archive file/downlink in v1

The probe first proves TT&C readiness over S-band TCP, then uses the existing housekeeping archive command surface to downlink `hk-index.csv` and at least two occupied `hk-slot-*.bin` files. Success requires byte-for-byte matches against OBC runtime source snapshots.

## Risks / Trade-offs

- **[Risk] S-band TCP can be mistaken for direct GDS TCP** -> Mitigation: OBC runs with `GDS_PORT=0`, `GROUND_LINK_MODE=comm-csp`, and `COMM_CSP_NODE=5`; evidence records both GDS TCP and S-band TCP ports separately.
- **[Risk] TCP reconnect behavior can block CSP services** -> Mitigation: COMM node TCP accept/read loops remain bounded and keep polling CSP services.
- **[Risk] File/downlink expands runtime duration and flake exposure** -> Mitigation: reuse the existing bounded HK archive file/downlink probe pattern with isolated runtime roots, bounded command attempts, source snapshots, and byte-match verdicts.

## Migration Plan

1. Add TCP endpoint mode to shared COMM node app/server support while preserving existing serial endpoint behavior.
2. Add gateway southbound TCP client mode while preserving existing serial-device behavior.
3. Add focused unit coverage for TCP southbound byte movement.
4. Add the hosted S-band TCP probe with TT&C prerequisite and HK file/downlink assertions.
5. Update docs, OpenSpec specs, registry, evidence, and reconciliation matrix.

Rollback strategy: remove the new TCP endpoint modes and S-band probe while leaving the existing serial-backed generic COMM and dual-link foundation behavior intact.

## Open Questions

- none
