## Why

The repository now proves bounded command/event/channel TT&C and housekeeping archive file/downlink through COMM node `4` on `subsystem.local:can1` to target OBC on `obc.local:can0`. That proof is still probe-driven: the target OBC service path, subsystem runtime management, CAN bring-up, ground launcher, operator runbook, and release packaging boundary do not yet make the path reusable as a normal lab target workflow.

The next step is to make the path operational for the lab target while keeping the claim narrower than a flight deployment baseline. The change must preserve non-root long-running runtime processes, keep transitional CAN provisioning separate from final OS configuration, and split subsystem process failure boundaries into separate systemd units.

## What Changes

- Add a lab target operational baseline for `obc.local` running installed OBC with `GROUND_LINK_MODE=comm-csp`, `CSP_TRANSPORT=socketcan`, COMM node `4`, and live GPS UART by default.
- Add an OBC-specific `obc-comm-csp-stack.service` that starts from the installed `current` release, and add helper behavior that enables/starts the new service while disabling/stopping the older `obc-installed-stack.service`.
- Add lab CAN bring-up oneshot service templates with configurable timing defaults matching the current evidence: `bitrate 500000 dbitrate 2000000 fd on`.
- Add workspace-based subsystem service management on `subsystem.local` using an aggregate `subsystem-comm-csp-stack.target` with separate EPS, ADCS, and COMM services.
- Add a formal ground launcher for macOS that starts stock `fprime-gds` plus `ground_ttc_gateway` with reviewable ports, file-storage directory, serial endpoint, and preamble settings.
- Add an operator runbook under `docs/operator/` describing package/install, service migration, subsystem services, ground startup, status/journal checks, rollback, and E2E verification.
- Add an operational E2E probe or focused helper that validates reboot/autostart, subsystem services, CAN state, ground path, command/event/channel flow, and housekeeping file/downlink over the service-managed lab path.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `platform-baseline`: define the lab target operational baseline, service identities, runtime user boundary, transitional CAN provisioning, and release/deployment responsibility split.
- `comm-subsystem`: require service-managed COMM SocketCAN operation with separate subsystem process failure boundaries.
- `ground-ttc-gateway`: add the operator launcher/runbook boundary for the lab target COMM CSP path.
- `verification-evidence`: require evidence for service-managed reboot/autostart, subsystem services, CAN state, ground observations, and file byte matches.
- `verification-path-registry`: register the lab target operational path separately from probe-only SocketCAN TT&C/file-downlink evidence.

## Impact

- Adds systemd templates, install/status helpers, a ground launcher, operator documentation, and an operational E2E probe.
- Updates the OBC package contents to include the OBC comm-csp profile and OBC service template only.
- Keeps subsystem service templates, CAN oneshot templates, ground launcher, and operator runbook as repo-governed lab deployment artifacts rather than files claimed inside the OBC install tarball.
- Does not add RF behavior, no-preamble first-byte-clean behavior, reliable retransmission, arbitrary onboard file downlink, ScenarioBridge/pass automation, a custom GDS plugin, final flight OS CAN provisioning, or final flight runtime identities.
