## Context

`transparent-link-framing-v1` introduced a repository-owned frame v1 above the existing serial byte-stream transport. That change proved binary-safe framing, CRC validation, and host-side deframing for a small set of representative payloads, but it did not yet prove that the path remains healthy when exchanges repeat for longer than a few frames or when the host-side peer disappears and comes back.

The project still needs a layered progression:

- keep the direct TCP to `fprime-gds` baseline untouched
- keep the controller-oriented `mock-text` radio baseline untouched
- harden the transparent framed UART path before attempting any ground gateway or vendor-specific control plane

This change therefore focuses on robustness of the existing framed transparent path rather than inventing a new framing format or introducing a higher-layer gateway.

## Goals / Non-Goals

**Goals:**

- prove the framed transparent UART path can carry repeated binary-safe frames over the governed Raspberry Pi to host serial link
- prove the path can recover after the host-side framed peer disconnects and is restarted
- add governed probe tooling and integration tests that make these behaviors reviewable
- preserve frame v1 and the current transport split so later work can build on a stable baseline

**Non-Goals:**

- redesign frame v1, replace CRC-32, or add scrambling
- replace or modify the direct TCP to `fprime-gds` baseline
- implement a ground gateway or RF simulation
- implement legacy EnduroSat control/configuration commands or newer `csp-es` integration

## Decisions

### Decision: Keep frame v1 unchanged and harden behavior around it

The framing format already covers the immediate data-plane need: explicit delimiter, escaping, length, and CRC-32. This change will stress and recover that format rather than revise it again. Revising the format now would mix link-definition work with robustness work and make later evidence harder to compare.

Alternative considered:

- change frame structure before robustness testing
  - rejected because the current frame is already sufficient for repeated and reconnect scenarios

### Decision: Exercise robustness through governed host-peer modes instead of modifying the controller layer

The transparent framed path should stay transport-oriented and data-plane-oriented. Robustness behavior will therefore be driven by host-peer launch options and probe scripts, not by new controller-layer semantics. This keeps the existing `RadioController`, `CommController`, and `mock-text` paths isolated from transparent-link hardening.

Alternative considered:

- add robustness logic into `RadioController`
  - rejected because it would blur the boundary between controller semantics and transparent link validation

### Decision: Cover reconnect by restarting the host peer, not by adding background auto-retry loops first

The first robustness slice should prove that a fresh framed exchange can succeed after a clean interruption and restart of the host-side peer. This is enough to validate link-layer recovery behavior without committing yet to a richer retry state machine inside the runtime.

Alternative considered:

- implement automatic retry or persistent reconnect state in the runtime first
  - rejected because it introduces policy decisions before the basic recovery case is fully evidenced

### Decision: Make evidence explicit about what still remains out of scope

The more robust framed path may look closer to a gateway-ready link, so the evidence must keep clear boundaries. Reviewers should still see that direct GDS integration, RF effects, vendor control plane, and future-generation hardware paths are separate later work items.

## Risks / Trade-offs

- [Risk] Reconnect scenarios may behave differently on the Raspberry Pi than in hosted PTY tests. → Mitigation: keep a governed Pi to host serial probe as the primary acceptance path and treat local tests as regression support.
- [Risk] Longer framed exchanges may expose buffering or delimiter handling bugs in the serial transport. → Mitigation: add integration tests that cover repeated frames and representative binary payloads before relying only on the Pi probe.
- [Risk] Host-peer restart timing could produce flaky results. → Mitigation: make the probe script explicitly control peer lifetime and verify observable log markers rather than assuming a restart succeeded.
- [Risk] Readers may confuse framed-link robustness with full ground-station integration. → Mitigation: keep evidence and specs explicit about the remaining boundaries.
