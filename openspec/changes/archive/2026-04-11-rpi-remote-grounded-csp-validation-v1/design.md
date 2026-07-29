## Context

The existing repository can already separate internal CSP settings from GDS settings, and the libcsp runtime can already point at a non-local hub host through environment variables. What is missing is a governed topology and probe that make the original `Pi OBC + remote macOS simulators` architecture explicit and reviewable, including one real ground-driven subsystem command path over GDS.

## Decisions

1. **Treat remote internal CSP and GDS-driven subsystem commands as two distinct proven paths.**
   - The probe runs both in one session, but the evidence and registry entries stay separate.
   - Internal CSP reachability does not imply GDS command dispatch, and GDS command dispatch does not imply every other ground path.

2. **Run the host-side support services on macOS and only the OBC process on Pi.**
   - macOS starts `csp_zmqproxy`, `eps_simulator`, `adcs_simulator`, and headless `fprime-gds`.
   - Pi starts only `OBC`, pointing both `CSP_HUB_HOST` and `GDS_HOST` at the macOS LAN host.

3. **Keep external comm optional and out of scope for acceptance.**
   - The Pi-side launcher defaults to `comm=tcp` to minimize variables.
   - If the user chooses `COMM_MODE=serial` / `/dev/serial0`, the launcher uses the existing wiring directly and does not start a fake loopback peer.
   - The resulting evidence still scopes acceptance to remote internal CSP and the GDS-driven subsystem command path.

4. **Use one EPS and one ADCS command as the ground-driven subsystem proof.**
   - `EPS_SET_PDU(channel=2, enabled=true)`
   - `ADCS_SET_MODE(POINTING)`
   - The probe waits long enough for these GDS commands to land, then asks the Pi-side REPL to print `eps get` and `adcs get`.

## Risks / Mitigations

- **Risk:** Timing between Pi REPL input and host-side `fprime-cli` commands is flaky.
  - **Mitigation:** the probe uses delayed stdin injection on Pi and bounded sleeps around the GDS command dispatch.
- **Risk:** Reviewers misread the remote TCP/IP carrier as proof of a future physical bus.
  - **Mitigation:** specs, registry, and evidence explicitly label the result as a remote development-carrier topology, not a physical-wire proof.
