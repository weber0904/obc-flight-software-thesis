## 1. OpenSpec And Scope

- [x] 1.1 Add proposal, design, spec deltas, and tasks for the remote Pi+macOS topology and GDS-driven subsystem validation.
- [x] 1.2 Keep external comm, GPS, boot/update, and real subsystem hardware out of scope for this change.

## 2. Launchers And Probe

- [x] 2.1 Add a host-side stack launcher for remote CSP simulators plus headless GDS.
- [x] 2.2 Add a Pi-side launcher that starts only OBC against remote CSP and GDS endpoints.
- [x] 2.3 Add a bounded probe that proves remote CSP reachability plus ground-driven EPS and ADCS commands.

## 3. Documentation And Validation

- [x] 3.1 Update README, verification matrix, verification-path registry, and new evidence docs for the remote topology.
- [x] 3.2 Record the remote internal CSP path and the remote GDS-driven subsystem command path as distinct entries and evidence sections.
- [x] 3.3 Run focused probe validation, `openspec validate rpi-remote-grounded-csp-validation-v1`, and `openspec validate --specs`.
