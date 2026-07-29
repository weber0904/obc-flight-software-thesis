# active-topccsds-target-alignment-v1 Evidence

Date: 2026-05-16.

Branch: `feature/persistent-command-freshness-v1`.

Base commit: `9cadc693ba3b0571f76285b039f2e4834c7b601c` at evidence capture time.

OpenSpec change: `active-topccsds-target-alignment-v1`.

## Scope

This record closes the Raspberry Pi active-path alignment required by architecture-review follow-up 02:

- governed Raspberry Pi packaging now takes the active `build-artifacts/.../OBC` binary and active `dict/AppTopologyDictionary.json` from `TopCcsds`
- the installed release still launches through `bin/OBC`, but that path now truthfully resolves to the active deployment rather than silently packaging `OBC_ComFprimeLegacy`
- source-workspace Raspberry Pi helpers used by 02 now default to active `OBC`
- installed-release smoke, autostart, and reboot evidence are refreshed on the corrected active path

This record does not claim legacy retirement, COMM CSP lab operational closure, target-side authenticated replay defense, or any new validation for legacy `Top` / `OBC_ComFprimeLegacy`. Legacy paths remain only as fallback or regression references in this wave.

## Build And Packaging Evidence

Commands run:

```text
bash scripts/package_rpi_bundle.sh
env FORCE_INSTALL=1 bash scripts/install_rpi_bundle.sh build-artifacts/packages/rpi/v0.1.0-150-g9cadc69-dirty/obc-rpi-v0.1.0-150-g9cadc69-dirty.tar.gz
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-persistent-command-freshness-v1d
openspec validate active-topccsds-target-alignment-v1
openspec validate --specs
```

Result summary:

- `bash scripts/package_rpi_bundle.sh`: PASS.
- `env FORCE_INSTALL=1 bash scripts/install_rpi_bundle.sh ...`: PASS.
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-persistent-command-freshness-v1d`: PASS.
- `openspec validate active-topccsds-target-alignment-v1`: PASS.
- `openspec validate --specs`: PASS.
- Installed release: `v0.1.0-150-g9cadc69-dirty`.

Produced artifacts:

- tarball: `build-artifacts/packages/rpi/v0.1.0-150-g9cadc69-dirty/obc-rpi-v0.1.0-150-g9cadc69-dirty.tar.gz`
- manifest: `build-artifacts/packages/rpi/v0.1.0-150-g9cadc69-dirty/manifest.json`

Manifest excerpts proving active-path payload selection:

```text
release_id=v0.1.0-150-g9cadc69-dirty
project_version=v0.1.0-150-g9cadc69-dirty
framework_version=v4.1.0
bin/OBC 4217784 1e0746ef95923906db9fb0d32ee74b4de43a8e6a86bbc71ede3c69d6e55c3c3e
dict/AppTopologyDictionary.json 482099 76a316c0ef232f0980682acc221c025abd848304b4c2c8442eaba7110fb009be
launch/run_stack.sh 4990 aae4149c5829e1c0ca35129d0d8382fca1a26c3d264efe2d9331e4c0ef8f7fb3
```

Tarball listing excerpt:

```text
./dict/AppTopologyDictionary.json
./launch/run_stack.sh
./bin/eps_simulator
./bin/OBC
./bin/radio_mock_server
./bin/adcs_simulator
./bin/csp_zmqproxy
```

Interpretation: the governed Raspberry Pi bundle now packages the active `OBC` deployment and its active dictionary, not `OBC_ComFprimeLegacy`.

## Installed Release Smoke

Command run:

```text
RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_installed_probe.sh
```

Result summary:

- `RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_installed_probe.sh`: PASS.

Key observations:

```text
Framework Version: [v4.1.0]
Project Version: [v0.1.0-150-g9cadc69-dirty]
OBC CCSDS S-band runtime started. Type 'help' for commands.
Storage roots: persistent=$OBC_HOME/obc-deploy/runtime/integ-rpi/persistent-data staging=$OBC_HOME/obc-deploy/runtime/integ-rpi/staging
groundLink mode=disabled
boot active=SLOT_A pending=NONE confirmed=yes ... trustStatus=4 ...
```

Interpretation: the installed release runs the active packaged `OBC` payload from `$OBC_HOME/obc-deploy/current`, accepts the current runtime-root arguments, and exposes the expected boot/storage metadata on the corrected package path.

## Autostart And Reboot Evidence

Commands run:

```text
RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/install_rpi_autostart.sh
RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_autostart_probe.sh
```

Result summary:

- `RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/install_rpi_autostart.sh`: PASS.
- `RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_autostart_probe.sh`: PASS after fixing the helper to wait for SSH drop and SSH return across the reboot edge.

Key observations after reboot:

```text
Loaded: loaded (/etc/systemd/system/obc-installed-stack.service; enabled; preset: enabled)
Active: active (running) since Sat 2026-05-16 01:03:57 CST
bash $OBC_HOME/obc-deploy/current/launch/run_stack.sh
$OBC_HOME/obc-deploy/current/bin/OBC --comm tcp --comm-host 127.0.0.1 --comm-port 7000 ... --command-authority-profile sband-primary ... --headless
Project Version: [v0.1.0-150-g9cadc69-dirty]
Storage roots: persistent=$OBC_HOME/obc-deploy/runtime/integ-rpi/persistent-data staging=$OBC_HOME/obc-deploy/runtime/integ-rpi/staging
```

Interpretation: the governed installed `current` release is the rebooted systemd startup path, and reboot returns to the active `OBC` deployment rather than a legacy binary hidden behind the `bin/OBC` name.

## Notes

- `scripts/run_rpi_autostart_probe.sh` needed a bounded helper fix in this wave: it now waits for SSH to drop before waiting for the target to return. That change is probe cleanup required to make 02 evidence trustworthy on the active path.
- During reboot evidence capture the process listing also showed an unrelated `$OBC_HOME/obc-deploy/current/bin/OBC` instance using a separate runtime root (`comm-csp-lab-obc`). The formal verdict here is still bounded to the governed `integ-rpi` installed stack shown above; this record does not claim cleanup of unrelated lab processes.

## Reused And Updated Verification Paths

Reused baseline:

- Raspberry Pi direct `OBC -> GDS` connectivity evidence from `rpi-target-integration-v1`
- Raspberry Pi packaging/install path from `rpi-packaging-v1`
- Raspberry Pi installed-release autostart path from `rpi-autostart-v1`

Updated by this change:

- the governed Raspberry Pi package/install/autostart path now proves that `bin/OBC` is sourced from the active `TopCcsds` deployment payload
- source-workspace helpers required by 02 now default to active `OBC`

## Deferred Work

- retirement of `Top` / `OBC_ComFprimeLegacy`
- COMM CSP lab operational path cleanup beyond what 02 needs
- target-side persistent replay defense proofs beyond the bounded persistence record captured separately in `persistent-command-freshness-v1`
