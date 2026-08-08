## Context

The repository currently validates the external comm stack through three main paths: TCP mock, PTY-backed serial-like paths, and Raspberry Pi to host UART/RS485 hardware validation. The host-side peer for these flows is `radio_mock_server`, whose protocol is command/status oriented and currently implemented as the `mock-text` radio protocol adapter.

That peer is good enough for validating `RadioController` semantics, but it is not a close approximation of the legacy EnduroSat S-band transceiver described in the user's hardware manual. The legacy hardware exposes UART in transparent mode and specifies a fixed UART baudrate, while its deeper configuration and management behavior belongs to a different control plane. The current OBC repo does not yet implement that control plane, and it should not pretend to do so in this change.

## Goals / Non-Goals

**Goals:**

- Add a governed validation path that exercises legacy EnduroSat-style transparent UART payload exchange over the existing Raspberry Pi to host serial link.
- Improve the host-side simulator so it can behave as a transparent serial peer rather than only as a command/status mock.
- Reuse the existing byte-stream transport and `UartDriver` raw exchange path instead of redesigning the controller layer.
- Make the evidence explicit about what is validated and what remains out of scope.

**Non-Goals:**

- Full ESPS control-plane integration for the legacy EnduroSat transceiver
- Support for the newer EnduroSat `csp-es` ecosystem
- Replacing the current `mock-text` comm baseline
- Claiming complete real-radio support for power, configuration, or management flows that require hardware-specific control semantics

## Decisions

### Decision: Treat legacy EnduroSat integration as a data-plane-first change

The first slice will target transparent UART payload exchange only. This aligns with the hardware manual, avoids over-claiming unsupported management behavior, and lets the repository validate a real hardware-adjacent path immediately.

Alternative considered:

- Extend `RadioController` to model the full legacy control plane now. Rejected because the available manual evidence and existing code do not yet define a complete, trustworthy control/configuration implementation.

### Decision: Add a dedicated transparent peer behavior instead of mutating the existing `mock-text` baseline

The current host-side peer already serves an important regression role for the controller-oriented comm baseline. The transparent EnduroSat path should therefore be introduced as a separate mode or companion behavior so that the current baseline remains stable and reviewable.

Alternative considered:

- Replace `mock-text` entirely with a more general peer. Rejected because it would blur two different validation goals: controller regression and transparent data-path validation.

### Decision: Reuse `UartDriver` raw exchange for the transparent path

The repository already has a governed raw byte-stream path and a Raspberry Pi to host UART validation probe. Reusing that path minimizes architectural churn and keeps the transparent mode work below the existing comm contracts.

Alternative considered:

- Route transparent mode through `RadioController` as if it were still a status-oriented radio protocol. Rejected because transparent mode is fundamentally a data path, not the same request/response control contract used by `mock-text`.

### Decision: Keep the legacy hardware and future `csp-es` generations explicitly separate

This change will document that the legacy transparent-UART path and the future `csp-es` path are separate integration branches. This avoids mixing hardware generations and protects the current repo from premature migration decisions.

Alternative considered:

- Design the first change around both generations at once. Rejected because it would mix legacy hardware validation with a not-yet-adopted future stack and make verification less clear.

## Risks / Trade-offs

- [Risk] The transparent peer may still be simpler than the real EnduroSat hardware. → Mitigation: document the exact scope of the peer and keep remaining radio-specific behavior explicitly out of scope.
- [Risk] The existing comm architecture could tempt reviewers to assume `RadioController` is fully applicable to transparent mode. → Mitigation: validate transparent exchange through `UartDriver` raw flows and state clearly that control-plane work remains future scope.
- [Risk] Fixed legacy UART settings such as baudrate may not match future hardware revisions. → Mitigation: keep these settings runtime-configurable and document that they are tied to the legacy hardware path only.

## Migration Plan

1. Add the transparent peer mode and supporting script entrypoints.
2. Add a governed probe that runs the Raspberry Pi target against the host-side transparent peer with the legacy UART settings.
3. Capture evidence and update specs to separate validated legacy transparent behavior from future control-plane work.
4. Leave rollback simple: the repository can continue using the already-validated `mock-text` flow as the default comm baseline if the transparent path is not selected.

## Open Questions

- Whether the legacy hardware path should later gain a lightweight config helper before a full ESPS control-plane implementation
- Whether a future change should expose transparent-mode framing behavior through a named adapter or keep it strictly as a raw `UartDriver` path
