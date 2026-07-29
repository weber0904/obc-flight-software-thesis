## Context

The current strongest target path is still established through bounded probes. Those probes start and prepare the pieces explicitly:

```text
macOS fprime-gds + ground_ttc_gateway
  -> physical serial ingress
  -> subsystem.local COMM node 4 on can1
  -> shared SocketCAN bus
  -> obc.local OBC node 1 on can0
```

The new baseline keeps the same logical path but moves operational ownership into service-managed lab workflows. It is a lab target operational baseline, not a final flight deployment baseline.

## Decisions

- Rename the change and evidence boundary to `target-obc-comm-csp-lab-operational-baseline-v1`.
  - Use "lab operational" consistently.
  - State that subsystem workspace services and CAN oneshot provisioning are transitional mechanisms.
- Keep long-running runtime processes non-root.
  - OBC and subsystem runtime processes run as `operator` in this workspace-service stage because the workspace and native build outputs are owned by that user.
  - Root is used only by oneshot CAN provisioning services.
  - Future host-install work should move runtime ownership to dedicated non-login users such as `obc-runtime` and `subsystem-runtime`.
- Add a dedicated OBC service instead of changing the existing installed service in place.
  - `obc-comm-csp-stack.service` starts from `$OBC_HOME/obc-deploy/current` or the configured install root.
  - The helper that installs it disables/stops `obc-installed-stack.service` before enabling/starting the new service.
  - Rollback is documented as disabling/stopping the new service and re-enabling the older installed service.
- Keep OBC package responsibility narrow.
  - The OBC installable bundle carries the OBC binary, dictionary, metadata, comm-csp launch profile, and OBC service template.
  - Subsystem services, CAN oneshot templates, ground launcher, and operator runbook are repo-governed lab deployment artifacts outside the OBC tarball boundary.
- Split subsystem service failure boundaries.
  - `subsystem-comm-csp-stack.target` aggregates three services.
  - `subsystem-eps-csp.service` runs EPS node `2` on `subsystem.local:can0`.
  - `subsystem-adcs-csp.service` runs ADCS node `3` on `subsystem.local:can0`.
  - `subsystem-comm-csp.service` runs COMM node `4` on `subsystem.local:can1` and the lab serial ingress.
  - Each service has its own journal and restart boundary.
- Treat CAN oneshots as lab provisioning helpers.
  - Default timing remains `bitrate 500000 dbitrate 2000000 fd on`.
  - Timing values are configurable through helper/template parameters.
  - Evidence and runbook wording must not call this final flight OS provisioning.
- Add a ground launcher as the reproducible operator surface.
  - It starts stock `fprime-gds` plus `ground_ttc_gateway`.
  - It prints GDS IP port, TTS port, file-storage directory, serial endpoint, baudrate, preamble count, and preamble delay.
- Make final validation service-managed.
  - The E2E proof must reboot `obc.local` and observe `obc-comm-csp-stack.service` coming back from the installed release.
  - It must use the service-managed subsystem units rather than probe-spawned subsystem processes.
  - It must prove bounded command/event/channel and housekeeping file/downlink through the same path.

## Risks / Trade-offs

- [Risk] `operator` as the runtime user can be mistaken for the final operational identity.
  - Mitigation: specs, runbook, and evidence state that this is a workspace transition and future host-install work should use dedicated non-login users.
- [Risk] CAN oneshot services can be mistaken for flight OS networking.
  - Mitigation: name and document them as lab provisioning helpers; keep timing configurable and final OS preconfiguration out of scope.
- [Risk] Splitting subsystem services increases service count and install complexity.
  - Mitigation: provide an aggregate target and status helper so operators can inspect the whole subsystem stack while retaining per-process failure boundaries.
- [Risk] Service-managed validation can fail because of stale installed release or stale subsystem build outputs.
  - Mitigation: package/install and sync/bootstrap steps remain explicit prerequisites in the runbook and probe.
- [Risk] The baseline name can be over-read as flight-like.
  - Mitigation: include "lab" in change name, evidence title, runbook title, and registry entry.
