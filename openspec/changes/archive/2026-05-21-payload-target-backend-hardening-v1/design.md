# Design: payload-target-backend-hardening-v1

## Summary

Target hardening moves the real camera backend into `PiCameraBackendHelper`, a
target-only helper process owned by the OBC runtime. The main payload component
interacts with the helper through a bounded IPC contract.

## Helper Isolation

- helper owns real `libcamera` and target raw-register adapters
- OBC owns request deadlines and helper lifecycle
- timeout -> helper kill -> payload state `FAULT`
- future bounded restart is optional and policy-controlled

## Why This Solves The Remaining Timeout Problem

The residual v1 issue was not timeout selection. The real problem was lack of a
safe recovery boundary when a single backend call stalls indefinitely inside the
same process. The helper process provides the recovery boundary that threads do
not.

## Closure Goals

- target build truthfully detects and links the real backend
- target probe truthfully enumerates OV5647
- `AUTO` capture produces a real JPEG
- `DETERMINISTIC` capture proves requested/applied settings
- raw-register path proves at least one real register round-trip or records an
  explicit bounded non-claim
- power-control investigation records whether a maintainable Raspberry Pi camera
  rail control path exists
