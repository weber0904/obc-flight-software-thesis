## Context

`run_uart_stack.sh` already starts the libcsp hub, EPS simulator, ADCS simulator, and OBC while using a serial device for the external comm link. The missing piece is a bounded target probe that makes both parts of that run explicit and reviewable.

## Decisions

1. **Add a wrapper probe instead of changing flight behavior.**
   - `scripts/run_rpi_csp_comm_baseline_probe.sh` drives existing target binaries.
   - It asserts CSP ping success to EPS and ADCS nodes, EPS/ADCS status visibility, radio enable success, and raw UART `STATUS` response.

2. **Reserve `/dev/serial0` for external comm in this validation.**
   - The target UART path is comm-owned for this run.
   - GPS live UART remains out of scope and is handled by serial allocation governance.

3. **Keep optional comm probes optional.**
   - Transparent, framed, and restart/recovery probes provide stronger evidence when hardware is connected.
   - The minimum acceptance path is the combined CSP + mock-text UART baseline probe.

## Risks / Mitigations

- **Risk:** Raspberry Pi or serial hardware is not reachable during development.
  - **Mitigation:** Evidence records the blocked condition and replacement hosted/local checks; the script remains checked in for the formal rerun.
- **Risk:** The combined probe is misread as a GDS or RF proof.
  - **Mitigation:** Registry and evidence name it as target-side internal CSP plus external UART/RS485 only.
