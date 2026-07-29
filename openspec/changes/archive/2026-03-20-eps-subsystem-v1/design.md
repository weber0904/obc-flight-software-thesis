## Context

The narrative EPS baseline already fixes the public contract boundary: an EPS simulator operating as CSP node `2`, `EpsBridge` as the OBC-owned interface, and `EPS_*` commands/telemetry/events as the subsystem-owned public symbols. The codebase, however, still has no subsystem-level implementation, no shared simulator protocol, and no hosted verification path beyond the core component skeletons.

This change is the first subsystem implementation slice. It needs to validate three things at once: the shared request/response protocol shape, a hosted simulator workflow, and an F' bridge component that maps transport results into stable command, telemetry, and event behavior.

## Goals / Non-Goals

**Goals:**

- Define a project-local EPS protocol shared by the hosted simulator and the OBC bridge.
- Implement a deterministic hosted EPS simulator with ZMQ request/response transport.
- Implement `EpsBridge` with the required `EPS_*` command, telemetry, and event families.
- Verify bridge behavior with focused F' unit tests and verify transport interoperability with a host integration test.
- Record evidence in a way that later changes can reuse.

**Non-Goals:**

- Do not implement real EPS hardware drivers or physical power electronics behavior.
- Do not require a full deployment or GDS session in this change.
- Do not introduce CAN, UART, or real CSP stack bindings yet.
- Do not solve hardware-in-the-loop verification; those remain `Blocked-HW`.

## Decisions

### Decision: Use a shared binary protocol header plus ZMQ transport

The simulator and client will share a single project-local protocol definition in `simulators/common/protocol.h`. Transport will use ZeroMQ REQ/REP over a configurable endpoint, while the message semantics continue to model the formal EPS node/port contracts.

Alternative considered:

- Encode payloads ad hoc in separate simulator and bridge code. Rejected because drift between the two sides would become likely immediately.

### Decision: Split the host path into protocol client and simulator model/server

The host-side implementation will separate deterministic state handling from the ZMQ server loop:

- `EpsSimModel`: owns EPS state and request handling
- `ZmqEpsTransport`: OBC-facing client for request/response calls
- `EpsSimServer`: ZMQ server wrapper around the model

Alternative considered:

- Put all simulator logic in a single executable `main`. Rejected because it would make transport testing and direct model validation much harder.

### Decision: Keep `EpsBridge` transport-pluggable for tests

`EpsBridge` will depend on a small transport interface and use a ZMQ-backed implementation by default. Unit tests will inject a fake transport so bridge behavior can be exercised without sockets or timing sensitivity.

Alternative considered:

- Make unit tests talk to the live ZMQ server directly. Rejected because the bridge behavior and the transport layer would fail together, obscuring root cause.

### Decision: Verify the subsystem in two layers

This change will use:

- F' unit tests for `EpsBridge` command/event/telemetry behavior
- A host integration executable that launches the simulator server and validates the real ZMQ client/server exchange

Alternative considered:

- Build-only evidence plus manual simulator testing. Rejected because the EPS subsystem contract already includes actionable command and fallback behavior that should be automated now.

## Risks / Trade-offs

- **Homebrew `zeromq` is a host dependency** -> Detect the library path explicitly in CMake and fail fast if it is unavailable.
- **No deployment exists yet for scheduler-driven runtime polling** -> Expose a test helper that drives the same poll path as the future scheduler input.
- **Hosted simulator behavior is intentionally simplified** -> Keep the state model deterministic and document that detailed power chemistry remains out of scope for v1.
- **Hardware parity is incomplete** -> Mark real EPS hardware and true power/thermal behavior as `Blocked-HW` in the evidence record.

## Migration Plan

1. Create the OpenSpec proposal, design, tasks, and delta specs for `eps-subsystem-v1`.
2. Add the shared EPS protocol and ZMQ transport/client implementation under `simulators/`.
3. Add the hosted EPS simulator executable and integration test.
4. Implement `EpsBridge` and its focused unit tests.
5. Run the normal build, UT build, the bridge unit tests, and the host integration executable.
6. Record evidence under `docs/test-records/eps-subsystem-v1/`.
7. Validate and archive the change.

## Open Questions

None that block this first implementation slice. Full deployment/GDS wiring and true hardware-backed EPS integration are intentionally left to later changes.
