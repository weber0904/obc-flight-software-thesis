## Why

The active baseline already has governed mode entry, authenticated command
ingress, official sequence execution, standard resource telemetry, and bounded
hosted plus Raspberry Pi verification paths. It still does not have a governed
payload operation contract for the real OV5647-based Raspberry Pi CSI camera
that is already connected to the OBC Pi camera interface.

This change is needed now to stop treating camera use as an ad hoc lab action.
The repository needs one narrow, truthful payload contract that:

- keeps payload ownership in flight software rather than shell scripts
- lets official sequencing consume payload commands without becoming the payload
  owner
- models payload power and initialization explicitly even though the current lab
  hardware uses direct Pi camera-interface power rather than a real
  EPS-switched rail
- keeps `libcamera` backend details out of the operator-facing OBC command
  contract

## What Changes

- Add a repo-owned `PayloadOpsController` as the only public payload command,
  telemetry, and event owner in active `TopCcsds`.
- Add payload support layers under that owner:
  - `PiCameraManager`
  - `IPiCameraDriver`
  - target-only `LibcameraPiCameraDriver`
  - hosted/test `StubPiCameraDriver`
- Add the governed `PAYLOAD_*` command family:
  - `PAYLOAD_SET_DEFAULTS`
  - `PAYLOAD_PREPARE`
  - `PAYLOAD_CAPTURE_STILL`
  - `PAYLOAD_ABORT`
  - `PAYLOAD_SHUTDOWN`
  - `PAYLOAD_GET_STATUS`
- Extend command authority vocabulary and policy with payload resource labeling
  so payload commands fit the same governed model as the existing subsystem
  controls.
- Add hosted proof and target Pi camera proof for the new payload operation
  path, with explicit documentation that EPS PDU channel `3` is only a lab
  proxy notification path in v1 and not a physically switched payload rail.

## Capabilities

### New Capabilities

- governed camera payload operation contract for the OV5647-based Raspberry Pi
  CSI camera
- official-sequence-consumable payload command path on active `TopCcsds`
- payload-local logical power and prepare lifecycle with EPS simulator proxy
  channel `3`

### Modified Capabilities

- `core-system-contracts`: adds the public `PAYLOAD_*` family, payload
  authority/resource semantics, and bounded mode-gating truth
- `platform-baseline`: active runtime now includes the payload owner plus
  hosted/target camera backend split
- `resource-storage`: governs payload capture storage under
  `<runtime-root>/persistent-data/payload/camera/`
- `verification-evidence`: requires hosted contract proof and target Pi camera
  proof for payload operations

## Impact

- Affected runtime areas:
  - `OBC/TopCcsds`
  - command authority catalog and policy
  - `ModeManager` / `ModeSafetyController` runtime integration boundary
  - `EpsBridge` runtime integration boundary
  - official sequencing consumer path
- Affected proof and docs areas:
  - architecture truth
  - interface documentation
  - verification registry and test record

## Non-Claims

This change does not claim:

- an onboard scheduler
- a time-tagged mission planner
- persistent schedule storage
- a generic payload registry or plugin framework
- payload autonomy or closed-loop ADCS imaging logic
- camera downlink closure or final image-retention policy
- a physically switched EPS payload rail on current lab hardware
