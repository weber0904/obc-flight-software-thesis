# Manual Ops

This subtree is the maintained manual operator family for the current dual-GDS
COMM baseline. It is separate from proof/probe wrappers and keeps all new
manual entrypoints out of the root `scripts/` surface.

## Layout

- `hosted/`
  - hosted dual-GDS lifecycle wrappers
- `target/`
  - target baseline snapshot wrappers
  - target local dual-GDS ground-surface wrappers
- `lib/`
  - shared owner, manifest, and status helpers
- `manual_secure_ops.py`
  - secure auth, secure-v2 command, governed `.sequence-staging/<leaf>` upload,
    and governed `SEQ_*` operator helper

## Surface Split

- hosted manual surface:
  - owns the shared hosted `TopCcsds` runtime plus both local GDS/gateway
    surfaces
- target manual baseline:
  - owns only the bounded shared-target readiness check/snapshot surface
- target manual ground surface:
  - owns only local macOS GDS/gateway helpers and local secure-helper state

## Current Non-Claims

- no GDS UI command interception
- no root-level `scripts/run_*` wrapper expansion for this family
- no one-GDS aggregation or one-gateway simultaneous multiplexer
- no RF or broad mission-ops closure
