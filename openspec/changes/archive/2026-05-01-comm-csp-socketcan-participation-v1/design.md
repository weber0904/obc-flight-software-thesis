## Context

The existing SocketCAN foundation proves EPS/ADCS over a CAN FD-capable bus with `subsystem.local:can1` kept reserved and isolated. The existing COMM physical serial evidence proves omitted-RF TT&C through COMM node `4` over hosted ZMQHUB. This change combines only the missing internal-carrier boundary:

```text
GDS -> ground_ttc_gateway -> lab serial ingress
  -> COMM node 4 on subsystem.local:can1
  -> shared SocketCAN bus
  -> OBC on obc.local:can0
```

EPS and ADCS remain on `subsystem.local:can0`. COMM uses the same services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`; only the internal CSP carrier changes.

## Decisions

- Probe-owned CAN bring-up is required.
  - Every hardware CAN smoke/probe shall set required interfaces `UP` with `bitrate 500000 dbitrate 2000000 fd on` before judging connectivity.
  - Missing interfaces or sudo bring-up failure are setup failures, not physical wiring verdicts.
- `subsystem.local:can1` becomes active COMM participation for this proof.
  - Prior reserved-channel isolation evidence remains historical for the earlier EPS/ADCS-only slice.
  - This proof records `can0` and `can1` health as active interfaces on the same shared bus.
- TT&C is command/event/channel only.
  - The formal verdict requires OBC readback, fprime-cli command events, and `GROUND_LINK_TX_BYTES`.
  - File/downlink remains a later change.
- Keep process contracts stable.
  - `comm_csp_node` keeps node `4` and services `30/31/32`.
  - OBC uses `GROUND_LINK_MODE=comm-csp` and `COMM_CSP_NODE=4`.

## Risks / Trade-offs

- [Risk] CAN failures can be misdiagnosed if interfaces are down.
  - Mitigation: probes bring interfaces up and record pre/post interface state.
- [Risk] Adding `can1` to the bus could disturb existing EPS/ADCS traffic.
  - Mitigation: Stage 0 checks EPS/ADCS before implementation and the formal probe rechecks EPS/ADCS alongside COMM TT&C.
- [Risk] This could be mistaken for dual-bus redundancy.
  - Mitigation: evidence states this is one shared bus; OBC still uses one CAN controller.
- [Risk] This could be mistaken for file/downlink or RF.
  - Mitigation: formal verdict remains command/event/channel TT&C only.
