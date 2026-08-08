## Context

The comm subsystem is already layered so `CommController`, `RadioController`, and `UartDriver` operate above a shared byte-stream transport abstraction. That transport path is now validated across TCP mock, PTY-backed serial, and a real Raspberry Pi to host UART/RS485 hardware chain. What remains tightly coupled is the radio framing itself: the current `RadioTransport` implementation directly encodes and parses the first hosted text protocol, even though the long-term spec explicitly leaves room for either KISS or a vendor-specific adapter once a real radio protocol is selected.

This change therefore targets the protocol layer, not the serial transport layer. The goal is to make the current hosted text protocol an explicit adapter so future radio integrations can swap framing without forcing controller or transport rewrites.

## Goals / Non-Goals

**Goals:**
- introduce an explicit client-side radio protocol adapter interface below `RadioController`
- preserve the existing hosted text protocol as the default named adapter
- let runtime configuration select the adapter kind without changing controller APIs
- keep the current TCP / PTY / Raspberry Pi UART validation paths working with the default adapter
- document the extension point for a later KISS or vendor-specific adapter

**Non-Goals:**
- implementing a real vendor radio adapter in this change
- declaring KISS mandatory for the project
- changing the chosen byte-stream transport strategy
- claiming real-radio hardware validation

## Decisions

### 1. Add a protocol adapter layer below `RadioController`

`RadioTransport` will stop owning the request/response grammar directly. Instead, it will hold a protocol adapter object responsible for:

- formatting status, enable, power, and frequency requests
- parsing the returned response into `RadioStatus`
- reporting adapter-level invalid-response failures without changing controller semantics

This keeps `RadioController` focused on command and telemetry behavior while making wire framing replaceable.

### 2. Keep the current hosted text protocol as the first named default adapter

The existing text grammar based on `STATUS`, `ENABLE`, `POWER`, and `FREQ` requests will be preserved as `mock-text`. This ensures current hosted tooling and the Raspberry Pi UART hardware probe continue to work unchanged by default.

### 3. Expose adapter selection as runtime configuration

The runtime will gain an explicit adapter-selection setting so hosted and target launches can say which radio protocol adapter is in use. The default remains `mock-text`, but the runtime contract will no longer imply that the first hosted text grammar is the only supported future protocol.

### 4. Treat KISS and vendor framing as future adapter implementations

The existing narrative already says KISS is not a first-version mandatory choice. This change keeps that stance and formalizes the extension point instead of pre-committing the codebase to KISS. A later change may add:

- a KISS adapter
- a transparent-UART vendor adapter
- a direct device-specific radio adapter

without having to redesign `RadioController` or the byte-stream transport classes.

## Risks / Trade-offs

- [Adding an adapter layer could feel premature] -> The serial hardware path is now stable enough that protocol coupling is the next real integration risk, and this slice is intentionally small.
- [Runtime configuration could drift from available adapters] -> Keep a closed set of named adapters and fail fast on unknown selections.
- [Regression risk on the current mock-radio path] -> Preserve `mock-text` as the default and rerun the shared verification gate plus comm integration tests.

## Migration Plan

1. Extract the current hosted text request/response logic into a named adapter interface and implementation.
2. Update the runtime and helper configuration to select `mock-text` explicitly or by default.
3. Extend tests so the named adapter path is exercised through the shared byte-stream transport.
4. Record the regression evidence and keep future KISS / vendor work out of scope for this slice.

## Open Questions

- Whether a later real-radio change should add both `kiss` and `transparent-uart` adapters, or pick one based on the chosen radio hardware.
- Whether installed Raspberry Pi release configuration should eventually persist the selected radio adapter in a profile-specific settings file.
