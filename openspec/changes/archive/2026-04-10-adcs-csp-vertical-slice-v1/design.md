## Context

`feature/libcsp-internal-network-base` has established the hosted internal libcsp runtime and migrated EPS business traffic from direct ZMQ request/response to a libcsp node/service model. ADCS still has the same architectural drift that EPS previously had: the formal node allocation says ADCS is CSP node `3`, but the runtime path is still a project-local ZMQ REQ/REP transport using `simulators/common/protocol.h` as the effective wire authority.

This change is the ADCS vertical slice on top of the libcsp base branch. It does not revisit the ground path, external comm path, GPS path, mission logic, or ADCS dynamics model. It changes the hosted internal transport substrate for ADCS and records a new verification path for that internal business traffic.

## Goals / Non-Goals

**Goals:**
- Run the hosted ADCS simulator as libcsp node `3` on the governed ZMQHUB-backed internal CSP substrate.
- Replace the default `AdcsBridge` transport with a libcsp-backed client while preserving the F' `ADCS_*` public interface and current state semantics.
- Define ADCS-owned CSP request/reply payloads and application service ports that avoid libcsp reserved ports.
- Prove state, mode, target, and calibration traffic from node `1` to node `3` through a repository-owned integration smoke.
- Keep the verification registry explicit that this proves only hosted ADCS internal CSP traffic, not ground, external comm, GPS, or real ADCS hardware paths.

**Non-Goals:**
- No GDS, `fprime-cli`, or ground transport changes.
- No external comm, radio, transparent UART, GPS, CAN, scheduler, or Pi/hardware bring-up changes.
- No ADCS control-law redesign or new flight dynamics features.
- No removal of legacy ADCS ZMQ code beyond what is necessary for the default hosted path; final legacy retirement remains a later slice.

## Decisions

1. **Mirror the EPS CSP migration pattern for ADCS.**
   - ADCS will get an `AdcsCspProtocol.hpp`, `CspAdcsTransport`, CSP-backed simulator server, and focused integration script.
   - Rationale: EPS has already proven this migration shape in the same repo and reduces architectural variance between internal subsystem bridges.
   - Alternative considered: route ADCS business traffic through `CspBridge`. Rejected because `CspBridge` is the runtime owner/diagnostic facade; subsystem business protocols should remain owned by their subsystem bridge.

2. **Use ADCS-owned application ports above libcsp reserved ports.**
   - The ADCS service set will use application ports starting at `20` for state, mode, target, and calibration.
   - Rationale: libcsp reserves low built-in service ports. EPS already moved to `10..13`; ADCS receives its own non-overlapping port range.
   - Alternative considered: keep the narrative `1..4` service ports. Rejected because those collide conceptually with libcsp management service space.
   - Implementation note: the hosted libcsp build must allow application binds through at least port `23`; this slice sets the build-time bind range high enough for ADCS ports `20..23`.

3. **Preserve `AdcsBridge` public contract and cached-state behavior.**
   - `AdcsBridge` commands, telemetry, events, scheduler polling, invalid-reply handling, and convergence signaling remain behaviorally stable.
   - Rationale: this slice changes the internal transport substrate, not the F' public subsystem contract or mission semantics.

4. **Keep legacy ZMQ transport temporarily available but not default.**
   - The existing ZMQ implementation may remain as compatibility code during the migration base, but `makeDefaultAdcsTransport()` will select `CspAdcsTransport`.
   - Rationale: this avoids a larger retirement patch while making the new architecture the path future runtime scripts use.

## Risks / Trade-offs

- **Risk:** ADCS CSP payloads drift from the legacy `StateData` model.
  - **Mitigation:** Reuse the existing `StateData` DTO for app/runtime state while moving the on-wire request/reply envelope into ADCS-owned CSP payload definitions.

- **Risk:** CSP integration tests can hang if background processes fail.
  - **Mitigation:** Add CTest `TIMEOUT` for the ADCS CSP integration test and keep script cleanup traps.

- **Risk:** Hidden coupling between default runtime initialization and transport tests.
  - **Mitigation:** Use the same runtime readiness cache pattern as the fixed EPS transport and allow injected runtimes for tests.

- **Risk:** Reviewers may confuse the ADCS CSP path with ground, external comm, or hardware coverage.
  - **Mitigation:** Add a dedicated verification registry entry and evidence record with explicit out-of-scope neighboring paths.
