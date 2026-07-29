# Proposal: payload-target-backend-hardening-v1

## Why

The payload contract still lacks truthful hard-hang recovery for the target
camera backend and still needs fresh real OV5647 target closure. In-process
timers do not safely recover from a blocking `libcamera` or kernel call that
never returns.

## What Changes

- Introduce a target-only helper process for camera backend isolation.
- Move target camera operations to an IPC boundary with explicit deadlines.
- Record truthful target `libcamera`, OV5647, and raw-register closure or
  explicit bounded non-claims.
- Investigate, but do not pre-claim, any true Raspberry Pi camera power-control
  capability.

## Non-Goals

- broad mission autonomy or scheduler work
- unconditional physical payload power-switch closure
