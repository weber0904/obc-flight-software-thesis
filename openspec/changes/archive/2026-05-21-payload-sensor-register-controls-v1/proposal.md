# Proposal: payload-sensor-register-controls-v1

## Why

The next payload slice needs a governed path for OV5647 sensor-level debugging
and deterministic low-level experiments without broadening the payload contract
 into a generic byte tunnel.

## What Changes

- Add raw sensor register read/write commands under a dedicated `RAW_SENSOR`
  prepared session.
- Add a target-only backend adapter for OV5647 sensor controls.
- Keep hosted proof fake-only and explicit about the lack of real register
  effect.

## Non-Goals

- arbitrary raw byte forwarding
- generic plugin payload architecture
- replacing the higher-level auto/deterministic capture families
