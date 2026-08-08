## Context

The project has already proven two Raspberry Pi target steps:

- a governed workspace can be synced to the Pi and built natively
- the target stack can launch from that workspace and reconnect to the documented ground path

That is enough for integration, but it is not yet a real delivery artifact. The target currently depends on the entire synced source tree remaining present under `$OBC_HOME/lab/fprime/v0`, and there is no installed release layout, no bundle manifest, and no operator path for relaunching from a fixed install root.

The next slice should introduce packaging without overstating what is complete. It should create a reviewable installed bundle, preserve mutable runtime data outside the versioned release payload, and stay user-writable so it does not require root-owned system paths or service managers.

## Goals / Non-Goals

**Goals:**

- create a governed Raspberry Pi bundle from the already-built Linux target artifacts
- install that bundle into a fixed user-writable target root with a `current` release pointer
- launch the integrated target stack from the installed release path instead of the synced source workspace
- preserve runtime state in shared roots outside the release payload
- record reviewable evidence for package creation, installation, and installed-stack launch

**Non-Goals:**

- introducing systemd units, boot-time autostart, or root-owned `/opt` installation
- implementing signature verification or secure software-update authenticity
- replacing the existing workspace sync/build path as the authoritative development flow
- claiming the package is already a full A/B flight deployment image

## Decisions

### 1. Build the package from Raspberry Pi-produced Linux artifacts

The package will be assembled from the Raspberry Pi's native Linux build outputs rather than from host-generated Darwin artifacts. The host-side helper will fetch the required target artifacts and stage the bundle locally under `build-artifacts/packages/`.

Alternatives considered:

- Package Darwin artifacts on the host: rejected because they are not deployable to Raspberry Pi.
- Build a cross-compilation pipeline first: rejected because the project already has a working native target build path and packaging can build on that baseline.

### 2. Use a user-writable install root with versioned releases and a `current` pointer

The install flow will place releases under a fixed user-writable root, keep each payload in a versioned release directory, and update a `current` pointer to the selected release. Mutable runtime state will live under sibling runtime directories instead of inside the release tree.

Alternatives considered:

- Install directly into the synced workspace: rejected because that keeps treating source as deployment.
- Install into `/opt` or another root-owned path: rejected for this first slice because it introduces privilege and service-management concerns that are not required yet.

### 3. Package a minimal but runnable installed stack

The bundle will include the OBC runtime, EPS / ADCS / radio mock executables used by the integrated target flow, the deployment dictionary, launcher assets, and a manifest that records packaged versions and file digests. This keeps the installed stack runnable without the full workspace while still matching the current integrated target behavior.

Alternatives considered:

- Package only the OBC binary: rejected because the current integrated target flow also depends on the companion simulator and radio mock processes.
- Package the full build tree: rejected because it is larger than necessary and blurs source/build/runtime boundaries.

### 4. Keep installed launchers relative and repo-independent

The installed release will include its own launcher assets so it can start from the install root without sourcing repo-local helper files. Host-side convenience scripts may still SSH into the install root, but the installed payload itself should be self-contained enough to run after extraction.

Alternatives considered:

- Reuse repo-local scripts directly from the installed flow: rejected because that would keep the installed release dependent on the source workspace remaining present.

## Risks / Trade-offs

- [Packaging depends on a live Raspberry Pi build] -> Reuse the governed target bootstrap path as the prerequisite and document that package creation is target-backed.
- [User-writable install root is less production-like than a system path] -> Accept this for the first slice to keep installation and validation straightforward; system integration can be a later change.
- [Bundle manifest could drift from actual payload contents] -> Generate digests from the staged bundle payload during package creation and record them in evidence.
- [Installed stack still uses software mocks] -> Keep that explicit; packaging changes deployment shape, not the underlying hardware-validation status.

## Migration Plan

1. Add packaging templates/assets and host-side helpers for bundle creation and installation.
2. Assemble a Raspberry Pi bundle from the governed target build outputs.
3. Install the bundle into the fixed target install root and update the `current` release pointer.
4. Launch the installed stack from the install root and capture evidence.
5. Update docs, validate the change, and archive it.

## Open Questions

- Whether a later change should add systemd integration on top of this install root.
- Whether a later boot/update change should consume the same bundle manifest format as an update artifact.
