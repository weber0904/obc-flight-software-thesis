## Context

The repository currently proves the first-version OBC stack on a hosted developer machine, but the `integ-rpi` profile still relies on implicit assumptions that do not hold on a Raspberry Pi target. The existing helper scripts hard-code Darwin output paths, the target workflow is not yet captured as a repo-local launch/build flow, and the boot/update slice persists metadata correctly but is still only evidenced on the hosted profile.

The Raspberry Pi 3B+ was reachable over SSH as `operator@youjun.local` during this historical change window. That hostname is now deprecated; current governed references use `operator@obc.local` or `operator@<private-lab-host>`. The immediate constraints at the time were:

- the target currently has Python 3.11, `git`, `g++`, and `ninja`, but not `cmake`
- UART is not yet enabled in `/boot/firmware/config.txt`, so true hardware UART validation may still require a later target configuration step or extra cabling
- the project must keep the shared OBC logic intact and express profile differences through runtime configuration, paths, and launch helpers

## Goals / Non-Goals

**Goals:**

- make the repository-local launch flow portable across Darwin and Linux build outputs
- provide a governed Raspberry Pi sync/build/run path that reuses the same checked-in workspace structure
- let the runtime configure target-specific boot metadata and staging roots without forking `BootManager`
- capture Raspberry Pi evidence for native build, integrated stack launch, GDS connectivity, and restart-based boot state persistence
- clear the `Deferred-RPi` items that are actually exercised on the target

**Non-Goals:**

- replacing the first-version TCP mock radio backend with a vendor-specific radio protocol
- declaring real radio GPIO, I2C, power sequencing, or vendor framing fully verified without the required hardware path
- implementing Raspberry Pi firmware / bootloader partition ownership or secure boot chain behavior
- silently auto-modifying target boot configuration without an explicit operator checkpoint if a reboot or serial-console change is required

## Decisions

### 1. Use portable artifact discovery instead of Darwin-only paths

The current scripts assume `build-fprime-automatic-native/bin/Darwin` and `build-artifacts/Darwin/...`, which blocks direct reuse on Linux. This change will add a shared shell helper that locates the active deployment binary and dictionary dynamically from the repo-local build outputs.

Alternatives considered:

- Keep separate Darwin and Linux scripts: rejected because it duplicates behavior and violates the existing profile-sharing rule.
- Force a fixed Linux-specific output path: rejected because the current local workflow already relies on F' generated paths.

### 2. Keep the target workflow repo-local and SSH-driven

The Raspberry Pi flow will be driven by checked-in scripts that sync a reduced workspace, bootstrap the target virtual environment, and run the same OBC stack on the Pi over SSH. This keeps the target flow reviewable and avoids ad hoc shell history as the only record of how the target was exercised.

Alternatives considered:

- Manual copy/paste commands only: rejected because the target workflow would not be repeatable.
- A separate unmanaged target repo: rejected because it would drift from the governed workspace.

### 3. Make boot storage roots runtime-configurable

`BootManager` and `BootMetadataStore` already implement file-backed state, but their default paths point to hosted-relative directories. This change will expose configurable staging and persistent roots through the runtime entrypoint and target launch scripts so hosted and target profiles can share the same component logic.

Alternatives considered:

- Fork a Raspberry Pi-specific `BootManager`: rejected because the specs require shared core logic across profiles.
- Keep hard-coded hosted roots and rely on the current working directory: rejected because it makes target operation brittle and hides actual storage layout choices.

### 4. Validate target boot behavior through restart persistence before true boot-chain ownership

The first target integration slice will verify that boot metadata survives a Raspberry Pi process restart and that the confirm/rollback window resumes from persisted state on target hardware. This gives a real target-backed validation step without overstating that the Linux bootloader, SD slot ownership, or power-loss behavior are complete.

Alternatives considered:

- Claim full Raspberry Pi boot validation after one hosted-equivalent run: rejected because it would blur the line between target execution and actual boot-chain ownership.
- Block all target boot validation until a full systemd / reboot flow is automated: rejected because it would postpone useful target evidence that is already achievable now.

## Risks / Trade-offs

- [Target dependency drift] -> Keep the Raspberry Pi bootstrap steps in checked-in scripts and record the observed toolchain state in evidence.
- [UART still unavailable after target integration] -> Treat the target TCP mock and restart-based validation as the first slice, and keep missing serial-cable or radio-specific checks explicitly labeled `Blocked-HW`.
- [Remote execution changes are harder to debug] -> Prefer small repo-local helpers with logged commands and capture the target output under `docs/test-records/`.
- [Portable artifact discovery can pick the wrong build tree] -> Scope the helper to the known OBC deployment and dictionary layout and fail loudly when no unique match exists.

## Migration Plan

1. Add portable script helpers and target-oriented launch/build wrappers in the governed repository.
2. Extend the runtime / boot configuration surfaces needed for target roots.
3. Sync the workspace to the Raspberry Pi and build it natively there.
4. Run the integrated target stack on the Pi, including a GDS-connected run and a restart-based boot-state persistence check.
5. Record evidence, update narrative docs, validate the OpenSpec change, and archive it.

## Open Questions

- Whether the target UART should be enabled and rebooted as part of this change or split into a follow-up hardware validation change if extra cabling is still required.
- Whether the long-term Raspberry Pi run path should stay workspace-relative or move to a fixed target install root in a future packaging change.
