## Context

The official target TCP parity topology is fixed as:

- `macOS`: `fprime-gds`, ground-side listeners, `ground_ttc_gateway`
- `obc.local`: `OBC` only
- `subsystem.local`: `sband_comm_csp_node(node 5)`,
  `uhf_comm_csp_node(node 6)`, `eps_simulator(node 2)`,
  `adcs_simulator(node 3)`

This differs from historical split-host proofs because the matrix needs node
`5` and node `6` to be first-class governed southbound proof surfaces.

## Design

### Target TCP Parity Launcher

The change adds a dedicated target TCP parity launcher that coordinates the
three hosts and keeps per-case artifact ownership reviewable.

Node `6` uses TCP-based southbound emulation as the UHF carrier proof surface.
The evidence must never describe that as physical UART closure.

### First-Wave Matrix Cells

The foundation closes these target TCP cells first:

- `csp-reachability`
- `sband-command`
- `sband-file`
- `uhf-primary-command`
- `uhf-primary-file`

Sequence and failover wait for the next target TCP change once the topology and
carrier foundation is stable.

## Boundaries

- No target direct-control case
- No target TCP sequence or failover closure yet
- No physical-UHF provenance claim
