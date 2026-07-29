## 1. OpenSpec Artifacts

- [x] 1.1 Create proposal, design, tasks, and capability deltas for the lab operational baseline.
- [x] 1.2 Validate the change artifacts before implementation.
- [x] 1.3 Commit the OpenSpec proposal boundary.

## 2. OBC Installed Profile And Service

- [x] 2.1 Add an installed OBC comm-csp SocketCAN launch profile that starts only the OBC runtime from the installed release.
- [x] 2.2 Add `obc-comm-csp-stack.service` template with non-root runtime user, live GPS UART defaults, SocketCAN defaults, and installed-release working directory.
- [x] 2.3 Add install/status helpers that install the OBC comm-csp service, enable/start it, and disable/stop `obc-installed-stack.service`.
- [x] 2.4 Update OBC package creation and manifest so the bundle includes the comm-csp launch profile and OBC service template without claiming subsystem or ground artifacts inside the tarball.
- [x] 2.5 Run focused local validation for the OBC package/profile/service artifacts and commit.

## 3. Lab CAN Provisioning

- [x] 3.1 Add OBC and subsystem CAN oneshot service templates with configurable timing defaults.
- [x] 3.2 Add helpers to install and inspect the lab CAN provisioning services on `obc.local` and `subsystem.local`.
- [x] 3.3 Document that CAN oneshots are lab/development provisioning helpers, not the final flight OS configuration model.
- [x] 3.4 Run focused validation for template rendering and service-name/device validation and commit.

## 4. Subsystem Workspace Services

- [x] 4.1 Add workspace-based `subsystem-eps-csp.service`, `subsystem-adcs-csp.service`, and `subsystem-comm-csp.service` templates.
- [x] 4.2 Add `subsystem-comm-csp-stack.target` aggregate template.
- [x] 4.3 Add install/status helpers that install the target and three services, enable/start the aggregate, and expose per-service journal/status.
- [x] 4.4 Ensure subsystem runtime processes remain non-root and fail clearly if the workspace build outputs are missing.
- [x] 4.5 Run focused local/template validation and commit.

## 5. Ground Launcher And Operator Runbook

- [x] 5.1 Add a macOS ground launcher for stock `fprime-gds` plus `ground_ttc_gateway`.
- [x] 5.2 Add `docs/operator/target-obc-comm-csp-lab-runbook.md` covering package/install, service migration, subsystem services, ground startup, status/journal, rollback, and E2E verification.
- [x] 5.3 Update README/scripts index wording to point operators at the runbook.
- [x] 5.4 Run documentation/static checks and commit.

## 6. Operational E2E Probe

- [x] 6.1 Add a repository-owned operational probe that uses the installed OBC service, subsystem services, CAN oneshots, and ground launcher shape.
- [x] 6.2 Probe reboot/autostart of `obc-comm-csp-stack.service` from installed release.
- [x] 6.3 Probe subsystem aggregate and per-service health.
- [x] 6.4 Probe GDS/gateway command/event/channel and housekeeping archive file/downlink with byte-matched files.
- [x] 6.5 Record clear setup failures for missing builds, missing devices, CAN bring-up failure, or inactive services.
- [x] 6.6 Run focused operational validation and commit.

## 7. Closeout

- [x] 7.1 Run the shared local verification gate.
- [x] 7.2 Run package/install, CAN provisioning, subsystem service, ground launcher, reboot/autostart, and full E2E hardware validation.
- [x] 7.3 Add `docs/test-records/target-obc-comm-csp-lab-operational-baseline-v1/README.md` with exact commands, service states, CAN state, command/event/channel observations, file hashes, commit SHA, package release id, and exclusions.
- [x] 7.4 Update the verification path registry, reconciliation matrix, README/operator references, and planning docs as needed.
- [x] 7.5 Validate OpenSpec change and main specs, archive the change, regenerate reconciliation markdown, and commit final evidence/archive.
