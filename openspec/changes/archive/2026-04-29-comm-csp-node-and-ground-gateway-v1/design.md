## Context

The repository currently has three adjacent but distinct communication baselines:

- `Drv.TcpClient -> ComFprime` for the direct `GDS -> TCP -> OBC` development path
- `CommController`, `RadioController`, and `UartDriver` for controller-oriented external comm work
- internal libcsp business traffic for `EPS` and `ADCS`

The newly completed shared CAN FD foundation already proved that `EPS` and `ADCS` can continue to behave as CSP-facing logical subsystem nodes over the governed physical carrier, while `COMM` remained reserved and out of scope. This change fills that gap by adding the first COMM-owned CSP traffic path and by introducing the first ground gateway that keeps stock `fprime-gds` and stock F' framing intact.

The main implementation constraint is that `ComFprime` is currently wired to a single byte-stream driver in the static topology. The lowest-risk way to add a COMM-backed TT&C path without forking the whole deployment is to replace the stock `Drv.TcpClient` instance with a repo-owned driver component that can preserve the old direct TCP behavior and also select a new COMM/CSP backend at runtime.

## Goals / Non-Goals

**Goals:**

- preserve the current direct `GDS -> TCP -> OBC` path as an unchanged runtime-selectable baseline
- introduce `COMM` as CSP node `4` with reserved service ports `30-39`
- keep stock F' framed ground traffic intact across the new omitted-RF path instead of inventing a new command/event/tlm wire format
- implement a first repository-owned ground gateway that proxies `fprime-gds` TCP traffic to lab-side serial ingress
- implement a subsystem-side COMM node that bridges serial ingress and the internal CSP bus
- implement an OBC-side ground-link component that reuses `ComFprime` and switches between direct TCP and COMM/CSP backends
- capture bounded bidirectional `command/event/tlm` evidence while explicitly keeping file/downlink out of scope

**Non-Goals:**

- do not replace or rename the current controller-oriented external comm baseline
- do not introduce a custom `fprime-gds` communication plugin
- do not implement RF behavior, vendor-radio control semantics, or real radio hardware support
- do not claim dual-bus redundancy or full independent multi-controller physical realism beyond the existing shared CAN FD proof
- do not include file/downlink behavior in the first formal gateway-backed proof

## Decisions

### Decision: Replace the stock topology ground driver with a repo-owned runtime-selectable ground-link driver

The OBC topology will replace the `Drv.TcpClient` instance currently connected to `ComFprime.comStub` with a repo-owned `GroundLinkDriver` component that still implements the same F' byte-stream driver contract.

Rationale:

- `ComFprime` already knows how to process framed ground traffic, command dispatch, event downlink, and telemetry downlink
- reusing the existing `ComFprime` integration keeps the new TT&C path aligned with stock F' framing and minimizes changes to command routing
- runtime backend selection keeps the direct TCP baseline alive without requiring a second deployment or a topology fork

Alternative considered:

- create a second OBC deployment just for COMM-backed TT&C  
  Rejected because it would duplicate most of the current deployment and create needless baseline drift

### Decision: The first `COMM <-> OBC` handoff is byte-stream tunneling over CSP, not a new application command protocol

The first OBC/COMM integration boundary will move bounded raw F' framed byte chunks across the internal CSP bus rather than introducing a new custom application protocol for commands, events, or telemetry.

The first COMM-owned service allocation is:

- port `30`: uplink poll from OBC to COMM
- port `31`: downlink write from OBC to COMM
- port `32`: link/status query
- ports `33-39`: reserved for future COMM expansion

Rationale:

- this preserves the stock F' wire format from `fprime-gds` through to `ComFprime`
- it allows the first slice to remain limited to bounded `command/event/tlm` proof without needing a new command packet schema
- it fits the current `ICspRuntime::requestReply()` abstraction, so OBC can remain the active CSP requester for the first implementation

Alternative considered:

- send parsed command objects from `COMM` into a dedicated OBC listener port  
  Rejected for the first slice because it would require a new OBC-side command ingestion protocol, a second command-routing abstraction, and a separate downlink schema

### Decision: The first COMM node uses queued serial-to-CSP bridging

The subsystem-side COMM executable will own:

- one lab-side serial ingress/egress endpoint
- one libcsp node configured as node `4`
- a bounded uplink byte queue populated from serial reads
- synchronous CSP request/reply handlers for uplink polling, downlink writes, and link status

Rationale:

- this keeps the lab-side UART clearly outside the spacecraft-side bus while still making COMM the subsystem boundary
- queue-based polling is straightforward to verify and fits the current runtime and service patterns already used by EPS and ADCS
- bounded chunk transport makes it clear that the first proof is about TT&C path ownership, not about maximum throughput

Alternative considered:

- make OBC a passive CSP listener for COMM-pushed application packets  
  Deferred because the first slice can meet its bounded proof goals with a simpler OBC-driven polling model

### Decision: The first ground gateway is a GDS-facing TCP client plus serial proxy

The ground gateway will connect northbound to stock `fprime-gds` as a TCP client and southbound to the lab-side serial device as a raw byte proxy.

Rationale:

- current GDS already exposes the governed TCP adapter listener used by the direct baseline
- acting as the TCP client preserves stock `fprime-cli -> GDS` workflows and keeps GDS unaware of COMM specifics
- the gateway can stay small and repository-owned: read bytes from one side, write them to the other, and report bounded connection state

Alternative considered:

- require GDS to connect out to a gateway-side TCP server or a custom plugin  
  Rejected because it would either invert the current validated ground semantics or force early GDS coupling

### Decision: The first `GroundLinkDriver` backend surface is `direct-tcp` or `comm-csp`

`GroundLinkDriver` will support two runtime-selected backends:

- `direct-tcp`: connect straight to `fprime-gds` and preserve the current direct baseline
- `comm-csp`: poll COMM node `4` over CSP and exchange bounded byte chunks via the COMM service ports

Rationale:

- this preserves the direct baseline in the same topology and executable
- the new backend boundary is explicit and testable
- future transport additions can be constrained to the driver/backend layer instead of leaking into `ComFprime`

Alternative considered:

- overload the existing external `comm` controllers to carry F' ground framing  
  Rejected because that would collapse the legacy external comm baseline into the new TT&C story

## Risks / Trade-offs

- **[Risk] A repo-owned ground-link driver can regress the existing direct GDS baseline** → Mitigation: keep `direct-tcp` as a first-class backend, add focused component tests, and rerun the governed direct GDS probe after integration.
- **[Risk] Bounded chunk transport may hide throughput or frame-size issues** → Mitigation: state explicitly that the first proof covers bounded `command/event/tlm` traffic only and reject out-of-bounds chunks deterministically.
- **[Risk] Worker-thread driver code can be harder to test than pure synchronous components** → Mitigation: keep backend logic behind narrow helper interfaces and back it with both L1 helper tests and a classic F' component harness for the real component surface.
- **[Risk] Serial proxy behavior could be mistaken for RF equivalence** → Mitigation: keep the gateway and evidence wording explicit that the serial segment is only the lab-side omitted-RF ingress.
- **[Risk] Reusing stock F' framing may accidentally allow file/downlink traffic to appear** → Mitigation: bound the proof and evidence to command/event/telemetry, and treat any larger or unrelated traffic as out of scope for this slice.

## Migration Plan

1. Add and sync the formal contracts for COMM node `4`, service-port reservation `30-39`, gateway-backed path registration, and bounded gateway evidence.
2. Replace `Drv.TcpClient` in the topology with `GroundLinkDriver`, keeping `direct-tcp` as the default backend so current behavior remains available.
3. Implement the COMM CSP protocol and the subsystem-side COMM node executable.
4. Implement the ground gateway TCP-client/serial-proxy executable.
5. Add governed scripts and probes for:
   - direct baseline regression through `direct-tcp`
   - gateway-backed COMM path through `comm-csp`
6. Record the new verification-path registry entry and evidence README that cite the reused direct baseline separately from the newly proven COMM path.

Rollback strategy:

- if the COMM backend proves unstable, operators can switch the OBC runtime back to `direct-tcp` without removing the new component or invalidating the earlier direct GDS evidence
- if the serial/gateway side proves unstable, the repository still keeps the existing shared CAN FD and direct GDS baselines intact

## Open Questions

- whether the first bounded proof should eventually add a stricter “COMM-to-ground connected” readiness handshake before `GroundLinkDriver` announces ready
- whether future file/downlink work should reuse the same byte-chunk CSP services or reserve separate COMM service ports in the remaining `33-39` range
