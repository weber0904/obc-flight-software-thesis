# Design: payload-virtual-csp-node-v1

## Summary

This change introduces an OBC-internal payload CSP service so the payload can be
addressed like a subsystem service while the real camera remains local to the
OBC Pi. The first implementation lives on OBC node `1` payload-owned service
ports; node `7` is reserved for a future split deployment.

## Allocation

- payload virtual node id: `7`
- payload future split-ready service-port reservation: `40..49`
- first in-process implementation binds payload service shim ports `34..36` on
  local node `1` because the current libcsp runtime only allows bindable local
  application ports through `40`, while client connection source ports consume
  `41+`

## Runtime Shape

- `PayloadCspService` binds CSP request/reply handlers for payload service ports
- the node-`1` shim uses local ports `34` (`STATUS`), `35`
  (`CAPABILITIES`), and `36` (`LAST_CAPTURE_METADATA`)
- `PayloadCspGateway` translates payload service requests into local
  `PayloadOpsController` runtime snapshots
- the public `PAYLOAD_*` command family remains the only ground operator
  contract
- the first service surface is read-oriented: status, capabilities, and last
  capture metadata
- node `7` remains allocation-only in this change because the current libcsp
  runtime is a single local-node runtime per process

## Architecture Clarification

This change also records why the payload backend hard-hang problem differs from
EPS/ADCS/GPS timeout semantics:

- EPS/ADCS use request/reply timeout boundaries
- GPS uses polling/serial timeout boundaries
- payload real-camera prepare can stall inside a single blocking backend call

## Verification

- UT for request/reply translation and payload snapshot mapping
- hosted proof on internal CSP runtime from a non-OBC client node into OBC node
  `1` payload service ports
