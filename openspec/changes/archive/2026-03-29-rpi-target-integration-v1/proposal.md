## Why

The repository has already met the hosted `dev-macos` baseline, but the `integ-rpi` profile remains only partially realized: the Raspberry Pi target cannot yet use the same repo-local build and launch workflow, and several evidence items are still marked `Deferred-RPi` because the target hardware was previously unavailable. Now that a Raspberry Pi 3B+ is reachable, the project needs a governed target-integration slice that turns the documented target profile into a real, repeatable build/run/verify path.

## What Changes

- Generalize the repo-local launch and artifact-discovery scripts so the same governed workspace can build and run on Darwin or Linux instead of assuming Darwin-only output paths.
- Add repository-local Raspberry Pi helper flows for syncing the workspace, preparing the target build environment, and launching the integrated `integ-rpi` stack over SSH without inventing a second unmanaged process model.
- Make the OBC runtime and boot/update storage paths configurable enough to run on the Raspberry Pi target with target-specific staging and persistent-data roots while keeping the core application logic shared with the hosted profile.
- Record Raspberry Pi target evidence for native build, hosted stack launch on the Pi, GDS-visible target connectivity, and boot/update metadata persistence across target-side process restarts.
- Reclassify only the target-integration items that are truly cleared by Raspberry Pi validation; leave real radio, extra cabling, and power-loss scenarios explicitly constrained when the required hardware path is still absent.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `platform-baseline`: the formal platform baseline now needs repo-local `integ-rpi` entrypoints and portable artifact discovery for both hosted and Raspberry Pi execution.
- `resource-storage`: the storage baseline now needs target-specific runtime roots and evidence/log locations that can be exercised on Raspberry Pi instead of remaining purely conceptual.
- `comm-subsystem`: the comm subsystem now needs Raspberry Pi target integration evidence and a governed path for target-side TCP mock and UART-oriented validation without changing controller-layer logic.
- `boot-update`: the boot/update capability now needs Raspberry Pi target execution paths for staging and persistent metadata plus restart-based validation of the confirm/rollback state machine on target hardware.
- `verification-evidence`: the verification baseline now needs explicit Raspberry Pi target scenarios and evidence rules for clearing `Deferred-RPi` items.

## Impact

- Affected code: `scripts/`, `OBC/Main.cpp`, `OBC/Components/BootManager/`, and any shared helpers needed for platform-portable build artifact discovery.
- Affected docs: `README.md`, `obc-dev-spec/`, and `docs/test-records/`.
- Affected systems: local developer host, Raspberry Pi 3B+ target over SSH, and the repo-local GDS / verification workflow.
