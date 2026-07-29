# core-system-contracts Specification Delta

## ADDED Requirements

### Requirement: Payload Virtual CSP Identity Is Governed

The shared CSP contract SHALL reserve node `7` and application service ports
`40` through `49` for the payload virtual subsystem service.

#### Scenario: Payload virtual node is not ad hoc

- **WHEN** the payload subsystem-style service is implemented
- **THEN** it SHALL use governed payload-owned node and service identifiers

### Requirement: Node-One Shim Ports Stay Distinct From Future Node-Seven Allocation

The first in-process payload CSP service implementation SHALL use node-`1`
payload-owned shim ports that stay distinct from the future node-`7`
reservation.

#### Scenario: First implementation does not consume future split ports

- **WHEN** the repository hosts the first payload CSP service inside the OBC
  process
- **THEN** it SHALL keep node `7` and ports `40..49` reserved for future split
  deployment and use dedicated node-`1` shim ports instead
