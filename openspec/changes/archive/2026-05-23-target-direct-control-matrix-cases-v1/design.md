## Context

The registered direct `OBC -> GDS` target path already exists, but it is not
yet expressed as dedicated target matrix wrappers. That makes `direct-control`
uneven across environments and keeps target debugging coupled to broader stack
launchers.

## Design

### Dedicated Target Direct Wrappers

The change adds target TCP and target CAN direct-control wrappers that:

- start subsystem-side services so the surrounding environment is realistic
- connect `OBC` directly to `fprime-gds`
- avoid `ground_ttc_gateway`, `node 5`, and `node 6`
- preserve isolated runtime roots and rerun-safe artifact handling

### Evidence Boundary

These wrappers prove only the direct `fprime-cli -> GDS -> OBC` control path on
the target environments. They do not widen any satcom or southbound claim.

## Boundaries

- No new target TCP parity logic
- No change to CAN or UHF carrier behavior
- No registry update unless the dedicated direct-control cells pass
