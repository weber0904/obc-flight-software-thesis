## Overview

This change updates the repository's host-target conventions so the checked-in defaults match the near-term deployment shape:

- `macOS`: ground host
- `obc.local`: OBC target
- `subsystem.local`: subsystem simulator host

The change does not add a full dual-Pi launch workflow. It only aligns naming, SSH-target defaults, and formal documentation so later dual-host execution work can build on a clean baseline.

## Design Decisions

### Canonical Host Variables

The repository will standardize on:

- `OBC_SSH_TARGET` for the OBC Pi
- `SUBSYSTEM_SIM_SSH_TARGET` for the subsystem simulator Pi

`RPI_SSH_TARGET` remains supported as a compatibility alias for `OBC_SSH_TARGET`.

### Precedence

For current OBC-target scripts, target resolution will follow this order:

1. `OBC_SSH_TARGET`
2. `RPI_SSH_TARGET`
3. repo default `operator@obc.local`

This preserves existing user habits while making the new role-based naming explicit.

### Script Ownership Boundary

This slice distinguishes:

- OBC-target scripts that SSH into the OBC Pi today
- future subsystem-sim-targeted workflows that are not being implemented yet

The second host variable is introduced now so future work has a stable configuration surface, but existing remote-CSP scripts that only point at a remote host by IP do not suddenly become SSH-orchestrated dual-host scripts.

### Documentation Boundary

Normative docs and operator instructions will be updated to use:

- `obc target`
- `subsystem simulator host`

Historical evidence records remain historical. They may continue to show `operator@youjun.local` in observed command transcripts.

### Formal Spec Boundary

Only `platform-baseline` requires a formal requirement update in this slice. The existing reporting and architecture-review capabilities already have broad enough requirements to host updated role naming without a separate capability delta.
