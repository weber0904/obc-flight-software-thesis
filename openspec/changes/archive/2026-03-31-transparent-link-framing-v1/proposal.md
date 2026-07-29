## Why

The repository now has three useful but separate validation baselines: the direct TCP path to `fprime-gds`, the controller-oriented `mock-text` radio path, and the legacy EnduroSat transparent-UART data path. What it still lacks is a governed link-layer framing step above transparent UART and below any future ground-station or vendor-specific control semantics.

Without that framing layer, the current transparent path can prove byte transport, but it cannot yet demonstrate binary-safe packet boundaries, CRC-backed integrity, or host-side deframing behavior that could later feed richer ground tooling. The next incremental step should therefore add a minimal, self-contained transparent link frame format while keeping the current TCP/GDS baseline untouched.

## What Changes

- Add a governed transparent link framing format for the legacy transparent-UART path, including frame encoding, deframing, escaping, and CRC-32 integrity checks.
- Extend the host-side transparent peer so it can deframe and echo framed payloads instead of only echoing text lines.
- Add a runtime/operator path and governed probes that exercise framed payload transfer over the existing Raspberry Pi to host UART/RS485 link.
- Capture reviewable evidence showing what this framed link validates and what still remains outside scope, especially true RF behavior and vendor-specific control/configuration.

## Capabilities

### Modified Capabilities

- `comm-subsystem`: add a binary-safe transparent link framing layer above the current transparent UART data path while preserving the existing TCP/GDS and `mock-text` baselines.
- `verification-evidence`: require reviewable evidence for framed transparent-UART payload transfer and explicit scope boundaries for still-missing ground-station and control-plane work.

## Impact

- Affected code: `simulators/comm/`, `OBC/Main.cpp`, `OBC/Components/UartDriver/`, and UART-related probe scripts.
- Affected behavior: adds a framed transparent-UART validation mode without replacing the current raw transparent or `mock-text` paths.
- Dependencies: reuses existing UART/RS485 hardware path and current CRC-32 policy; no new external library is required.
