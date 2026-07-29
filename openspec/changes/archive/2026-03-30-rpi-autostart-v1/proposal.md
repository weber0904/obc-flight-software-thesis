## Why

The Raspberry Pi target can now build, package, install, and manually launch the governed OBC stack, but the installed release still depends on an operator SSH session to start it. The project needs a governed autostart path so the installed `current` release can come up after a target reboot and move closer to a real deployable OBC target flow without overstating full bootloader or partition-handoff support.

## What Changes

- Add a governed Raspberry Pi autostart flow that installs a systemd-managed service for the installed `current` release.
- Add a headless runtime mode so the installed OBC process can run under a service manager without depending on an interactive stdin-driven REPL session.
- Add repo-local helper scripts to install, inspect, and probe the governed Raspberry Pi autostart service from the repository.
- Record reviewable evidence for service installation, service status, target reboot, and post-reboot launch from the installed release.
- Update operator-facing documentation and narrative source documents to explain the governed service path and its remaining limits.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `platform-baseline`: the governed `integ-rpi` profile now needs a formal service-managed startup path for the installed `current` release.
- `boot-update`: the first target boot integration now needs a governed post-reboot startup path that relaunches the installed release and keeps boot state observable after OS reboot, without claiming full bootloader partition handoff.
- `verification-evidence`: the Raspberry Pi evidence baseline now needs reviewable autostart and post-reboot startup records.

## Impact

- Affected code: `OBC/Main.cpp`, Raspberry Pi packaging assets, and repo-local `scripts/`.
- Affected docs: `README.md`, `docs/README.md`, `obc-dev-spec/`, and `docs/test-records/`.
- Affected systems: Raspberry Pi install root, systemd service management, and target reboot validation.
