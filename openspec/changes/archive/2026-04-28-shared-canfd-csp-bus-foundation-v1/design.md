# shared-canfd-csp-bus-foundation-v1 Design

## Summary

The first CAN slice should be deliberately narrow:

- preserve the current OBC business-layer logic
- keep GPS and ground-side transport boundaries unchanged
- replace only the internal CSP carrier for `EPS` and `ADCS`
- treat the subsystem second CAN channel as reserved rather than over-claiming future COMM readiness

The governed proof is therefore:

`macOS fprime-cli -> GDS -> obc.local -> EPS/ADCS over a CAN FD-capable SocketCAN bus`

## Runtime Backend

`simulators/csp/CspRuntime` already owns runtime carrier selection through `CSP_TRANSPORT`. This change extends that existing abstraction instead of creating a second CSP runtime stack.

Chosen runtime surface:

- `CSP_TRANSPORT=socketcan`
- `CSP_CAN_DEVICE=<canX>`
- `CSP_CAN_PROMISC=<0|1>`, default `0`

Chosen semantics:

- `promisc=0` is formal acceptance mode
- `promisc=1` is debug-only mode
- the runtime does not program bitrate or `fd on`
- Linux link bring-up remains an operator prerequisite outside the process

The backend uses official libcsp SocketCAN integration via `csp_can_socketcan_open_and_add_interface(...)`. On Linux, the runtime performs fail-fast checks before bind:

- device name is syntactically safe
- device exists
- interface type is CAN
- interface is `UP`

If those checks fail, runtime init returns immediately with a clear process-side diagnostic rather than degrading into a later timeout.

## Physical Topology

Near-term hardware available for this slice:

- `obc.local`
  - Raspberry Pi 3B+
  - single-channel `MCP2518FD Pro`
  - direct GPS UART already in use
- `subsystem.local`
  - Raspberry Pi 3B+
  - dual-channel `MCP2518FD` HAT

Chosen v1 topology:

- one active shared bus between:
  - OBC CAN controller
  - subsystem primary CAN channel
- one reserved subsystem channel:
  - enumerated
  - self-tested in isolation
  - kept off the active bus

Important architectural limit:

- `EPS` and `ADCS` remain separate logical CSP nodes on one subsystem host and one subsystem primary controller
- this is not evidence of independent physical EPS and ADCS controllers
- OBC-side dual-bus redundancy remains unproven because `obc.local` has only one controller

## Operator Flow

The governed operator flow is split by role:

- `run_ground_gds_only_stack.sh`
  - macOS only
  - starts headless `fprime-gds`
- `run_subsystem_sim_can_stack.sh`
  - subsystem host only
  - starts `eps_simulator` and `adcs_simulator` on the primary CAN device
- `run_rpi_can_csp_stack.sh`
  - OBC host only
  - starts `OBC` on the OBC CAN device
- `run_shared_canfd_csp_gds_probe.sh`
  - macOS orchestration script
  - coordinates the three-host validation path

The probe records:

- subsystem primary and reserved `parentdev` mapping
- pre/post `ip -details -statistics`
- active-bus `candump`
- reserved-channel idle capture during active-bus traffic

## Evidence Boundary

This slice proves:

- the first physical internal CSP carrier can replace hosted `zmqhub`
- `EPS` and `ADCS` business behavior survives that carrier change
- the direct GDS path can still drive bounded subsystem commands after the internal-carrier migration

This slice does not prove:

- COMM bus participation
- omitted-RF TT&C
- RF behavior
- CAN FD large-payload use by libcsp
- full disconnect/reconnect robustness
- independent physical EPS/ADCS nodes
- dual-bus redundancy
