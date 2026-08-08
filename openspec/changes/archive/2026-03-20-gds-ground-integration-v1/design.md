# Design: gds-ground-integration-v1

## Summary

This change replaces the minimal hosted runtime wiring with a proper first-version F' ground stack while preserving the hosted developer ergonomics already added in `deployment-runtime-v1`. The design goal is not to invent a second operator path, but to let the same hosted `OBC` executable support both local REPL use and a real `fprime-gds` TCP connection.

## Topology changes

- Import `CdhCore.Subtopology` for:
  - `Svc.CommandDispatcher`
  - `Svc.EventManager`
  - `Svc.TlmChan`
  - `Svc.PassiveTextLogger`
  - `Svc.FatalHandler`
- Import `ComFprime.Subtopology` for:
  - `Svc.ComQueue`
  - `Svc.BufferManager`
  - `Svc.FrameAccumulator`
  - `Svc.FprimeDeframer`
  - `Svc.FprimeFramer`
  - `Svc.FprimeRouter`
  - `Svc.ComStub`
- Add local deployment instances for:
  - `Svc.PosixTime`
  - `Svc.ActiveRateGroup`
  - `Svc.RateGroupDriver`
  - `Svc.LinuxTimer`
  - `Drv.TcpClient`
  - the existing OBC components

## Runtime behavior

- Keep the hosted REPL in `OBC/Main.cpp` for operator convenience.
- Move periodic subsystem progression to rate groups instead of manual per-loop ticking.
- Accept `--gds-host` and `--gds-port` so the same binary can run with the ground link disabled or enabled.
- Keep the external comm mock path independent from the ground path:
  - external radio / UART simulation stays on the existing TCP mock or PTY transport
  - ground connection uses `Drv::TcpClient` to connect to `fprime-gds`

## Launch path

- `scripts/run_dev_stack.sh` remains the shortest hosted stack launcher and now accepts optional `GDS_HOST` / `GDS_PORT`.
- `scripts/run_gds_stack.sh` becomes the repo-local helper for the documented hosted + GDS flow and prints the exact matching `fprime-gds -n -g none --framing-selection fprime ...` command.

## Verification plan

- Rebuild the deployment after the topology upgrade.
- Launch headless `fprime-gds` in no-app mode.
- Launch `scripts/run_gds_stack.sh`.
- Verify that:
  - the hosted OBC stack stays up
  - the OBC deployment reports a successful TCP client connection to the GDS adapter
  - OS-level inspection shows an `ESTABLISHED` TCP connection on the documented port
