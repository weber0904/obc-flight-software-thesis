# platform-baseline Specification Delta

## ADDED Requirements

### Requirement: Payload CSP Service Lives Inside The OBC Process Before Split

The active baseline SHALL keep the real OV5647 camera local to the OBC Pi while
optionally exposing a payload CSP service inside the OBC process.

#### Scenario: First payload service surface does not imply payload relocation

- **WHEN** the active baseline exposes the first payload CSP service surface
- **THEN** it SHALL state that the camera remains attached to the OBC Pi and the
  service does not claim a separate payload processor

### Requirement: Node Seven Remains Reserved Until Process Split

The active baseline SHALL reserve payload virtual node `7` for future split
deployment while the first implementation uses payload-owned service ports on
local node `1`.

#### Scenario: Local service does not consume reserved node identity

- **WHEN** the repository documents or verifies the in-process payload CSP
  service
- **THEN** it SHALL distinguish the live node `1` service-port implementation
  from the reserved future node `7` identity

### Requirement: Node-One Shim Uses Bindable Local Ports

The first in-process payload CSP service SHALL use bindable local node-`1`
ports that do not overlap the current libcsp outgoing-client source-port band.

#### Scenario: Local payload CSP service avoids connection-source collisions

- **WHEN** the repository implements the node-`1` payload CSP shim
- **THEN** it SHALL use local payload service ports that are bindable on the
  current libcsp runtime and do not collide with client source ports `41+`
