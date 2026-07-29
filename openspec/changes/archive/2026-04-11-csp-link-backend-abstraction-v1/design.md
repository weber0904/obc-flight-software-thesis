## Context

`LibCspRuntime` currently owns both the libcsp runtime lifecycle and the hosted ZMQHUB binding details. That is workable for the current local baselines, but it leaves the carrier choice mixed into the runtime core and makes future remote or physical-bus carrier work look like a larger business-layer change than it should be.

## Decisions

1. **Introduce a carrier/backend layer below `ICspRuntime`, not above subsystem bridges.**
   - `ICspRuntime`, `IEpsTransport`, `IAdcsTransport`, and F' command/tlm/event contracts stay unchanged.
   - The new abstraction sits inside `simulators/csp/` and only owns carrier binding/config validation.

2. **Keep `zmqhub` as the only implemented carrier in this change.**
   - Add `CSP_TRANSPORT=zmqhub` and a generic transport selector in `RuntimeConfig`.
   - Preserve `CSP_HUB_HOST`, `CSP_HUB_SUB_PORT`, `CSP_HUB_PUB_PORT`, and `CSP_INTERFACE_NAME` compatibility.

3. **Keep future physical buses explicit rather than implied.**
   - Docs and specs must state that `TCP/IP + ZMQHUB` is the current development carrier.
   - Future `UART / CAN / RS485 / Ethernet` backends remain later work and are not silently implied by this refactor.

## Risks / Mitigations

- **Risk:** The refactor changes the existing hosted or Pi-local CSP behavior.
  - **Mitigation:** keep `zmqhub` the default, preserve current environment variables, and rerun all existing CSP/EPS/ADCS local checks.
- **Risk:** Reviewers misread the refactor as physical-bus validation.
  - **Mitigation:** update specs and evidence wording to separate carrier abstraction from future hardware-bus proof.
