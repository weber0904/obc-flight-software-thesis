## Why

The Raspberry Pi target can now build and run the governed workspace, but it still depends on a full synced source tree living at a workspace path. The project needs a first installable target package so the `integ-rpi` profile can be delivered and relaunched from a fixed install root instead of treating the development workspace itself as the deployable artifact.

## What Changes

- Add a governed Raspberry Pi packaging flow that collects the Linux target binaries, dictionary, version metadata, and launcher assets into a reviewable bundle artifact.
- Add a governed install flow that unpacks a selected bundle into a fixed user-writable install root on the Raspberry Pi and maintains a `current` release pointer for relaunching the installed stack.
- Add an installed-stack launcher path that runs the OBC runtime and simulators from the installed release while keeping runtime data under a shared install-root runtime tree rather than inside the release payload.
- Record evidence for package creation, installation, installed-stack launch, and installed bundle metadata on the Raspberry Pi target.
- Update operator-facing documentation to explain the packaged target workflow and the limits of this first installable slice.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `platform-baseline`: the governed `integ-rpi` profile now needs a formal package/install path in addition to the workspace sync/build path.
- `resource-storage`: the Raspberry Pi target now needs an installed release layout that separates immutable release payloads from mutable runtime roots.
- `verification-evidence`: the evidence baseline now needs reviewable package/install/run records for the installed Raspberry Pi target flow.

## Impact

- Affected code: `scripts/`, shared shell helpers, and packaging templates/assets.
- Affected docs: `README.md`, `docs/README.md`, `obc-dev-spec/`, and `docs/test-records/`.
- Affected systems: local host packaging workflow, Raspberry Pi install root, and installed target launch operations over SSH.
