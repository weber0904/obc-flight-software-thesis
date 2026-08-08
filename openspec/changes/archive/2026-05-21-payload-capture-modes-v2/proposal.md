# Proposal: payload-capture-modes-v2

## Why

`payload-ops-contract-v1` established a narrow governed camera contract, but it
only supports one still-capture surface with limited exposure/gain overrides.
The active baseline now needs distinct operator-visible capture modes for:

- rapid ground test and tuning with automatic camera behavior
- deterministic mission-data capture with fixed and reviewable parameters
- richer capture metadata and capability readback without claiming raw sensor
  register control yet

## What Changes

- Add `AUTO` and `DETERMINISTIC` payload session semantics and make session kind
  explicit during prepare.
- Replace the single canonical still-capture command with separate
  `PAYLOAD_CAPTURE_AUTO` and `PAYLOAD_CAPTURE_DETERMINISTIC` contracts while
  keeping v1 commands as compatibility wrappers during the PR.
- Replace the single defaults surface with separate OFF-only default profiles
  for auto and deterministic sessions.
- Add `PAYLOAD_GET_CAPABILITIES` and `PAYLOAD_GET_LAST_CAPTURE_METADATA`.
- Add governed sidecar metadata artifacts adjacent to captured JPEG files.

## Non-Goals

- raw sensor register control
- payload CSP node virtualization
- helper-process isolation or hard-hang closure
- physical EPS rail switching or new power-hardware claims
- non-JPEG operator artifacts
