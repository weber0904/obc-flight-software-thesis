# Proposal: payload-virtual-csp-node-v1

## Why

The payload is physically attached to the OBC Pi, but the architecture still
needs a payload-facing subsystem boundary that can evolve toward future split
deployments without redefining the public payload contract later.

## What Changes

- Reserve a new payload virtual CSP node and payload-owned CSP service ports.
- Implement the first payload CSP service surface on OBC node `1` payload-owned
  service ports so subsystem-style clients can query payload status without
  introducing a second ground operator plane.
- Keep node `7` reserved in the shared CSP allocation as the future split-ready
  identity for a physically separate payload process or computer.
- Add an OBC-internal payload CSP gateway/service layer that translates
  request/reply payload traffic to `PayloadOpsController` runtime snapshots.
- Keep the ground operator path unchanged; the CSP node is an internal subsystem
  boundary, not a second command plane.

## Non-Goals

- moving the real camera to subsystem Pi
- creating a new ground operator path
- claiming a physically separate payload computer
- claiming that node `7` is already hosted in the same OBC process
