## 1. Change Artifacts

- [x] 1.1 Create proposal, design, and a `platform-baseline` delta spec for host-role alignment.
- [x] 1.2 Define canonical host-target variables and compatibility behavior.

## 2. Script And Helper Alignment

- [x] 2.1 Add shared helper logic for canonical OBC and subsystem-simulator SSH target defaults.
- [x] 2.2 Update all current OBC-target helper scripts to resolve the target through `OBC_SSH_TARGET`, with `RPI_SSH_TARGET` kept as a compatibility alias.
- [x] 2.3 Introduce `SUBSYSTEM_SIM_SSH_TARGET` in the shared configuration surface without forcing existing remote-CSP scripts to become subsystem-host SSH launchers.

## 3. Docs And Spec Alignment

- [x] 3.1 Update the repo README and scripts README so the default target host is `operator@obc.local` and the second Pi is named `operator@subsystem.local`.
- [x] 3.2 Update the relevant platform-baseline docs/spec wording so the near-term topology is expressed as `macOS` ground host, `obc.local` OBC target, and `subsystem.local` subsystem simulator host.
- [x] 3.3 Update the architecture-review package where it discusses near-term topology or target-role naming.

## 4. Validation And Finalization

- [x] 4.1 Run `bash -n` on every modified shell script.
- [x] 4.2 Run `python3 scripts/check_repo_consistency.py`.
- [x] 4.3 Run `python3 scripts/check_agent_entrypoint.py`.
- [x] 4.4 Run `openspec validate dual-pi-hostname-and-target-role-alignment-v1` and `openspec validate --specs`.
- [x] 4.5 Add or update a test/evidence record for this change.
- [x] 4.6 Archive the change and update reconciliation surfaces.
- [x] 4.7 Collapse the work into one Conventional Commit and stop at local-ready without pushing.
