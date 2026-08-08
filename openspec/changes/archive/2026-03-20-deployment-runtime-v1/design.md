# Design: deployment-runtime-v1

## Summary

This change adds the missing "runnable product" layer on top of the already-implemented hosted components. The chosen design is intentionally minimal: a hosted F' deployment that wires time and text-event services, reuses the existing component implementations, and drives them through a simple interactive runtime loop. This avoids inventing a parallel demo-only stack while still satisfying the first-version goal of a project that can actually be launched and operated.

## Runtime Architecture

### Hosted deployment

- Add an `OBC_Top` topology module with:
  - `Svc.PosixTime`
  - `Svc.PassiveTextLogger`
  - the existing OBC component instances
- Keep the first integrated hosted runtime simple:
  - connect time ports
  - connect text-event ports so operator-visible events print to the console
  - leave command uplink / packetized downlink / GDS wiring for a later change

### Runtime control surface

- Add `OBC/Main.cpp` as the hosted runtime executable.
- The executable:
  - initializes the topology
  - configures the external comm transport as TCP mock or PTY-backed UART
  - periodically polls EPS / ADCS / radio status
  - advances `CommController` and `BootManager`
  - exposes a small interactive REPL for basic operations

### Component runtime helpers

The existing components were command-oriented and unit-test oriented. To keep the hosted runtime from bypassing them, this change adds narrow public helpers that:

- invoke the same transport/state logic already used by the component command handlers
- expose basic runtime state snapshots needed by the REPL
- avoid introducing a second implementation of subsystem behavior

## External comm mock

- Extract the hosted mock radio server into a standalone executable `radio_mock_server`.
- Support loopback TCP by default.
- Support PTY-backed operation via a symlink option so the same OBC runtime can switch between TCP mock and UART-like byte-stream paths.

## Dev stack entrypoint

- Add `scripts/run_dev_stack.sh`.
- The script launches:
  - `eps_simulator`
  - `adcs_simulator`
  - `radio_mock_server`
  - `OBC`
- The script acts as the shortest path to the first-version `dev-macos` profile.

## Verification plan

Minimum evidence for this change:

- `fprime-util build`
- runtime launch of the full dev stack
- interactive operations covering:
  - `status`
  - external comm status
  - EPS command path
  - ADCS mode change
  - pass-window behavior
- updated narrative/spec alignment

Remaining limits are still explicit:

- no real Raspberry Pi or physical radio path in this change
- no full GDS uplink/downlink deployment yet
