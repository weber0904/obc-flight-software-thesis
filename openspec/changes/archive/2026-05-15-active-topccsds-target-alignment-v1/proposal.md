## Why

The repository already treats `OBC` / `TopCcsds` as the active hosted baseline,
but the Raspberry Pi package and some target-facing helpers still silently pull
`OBC_ComFprimeLegacy` artifacts while presenting the result as `OBC`. That
breaks the source-of-truth rule for architecture follow-up 02 and makes any
target boot-trust or reboot-safe command-freshness claim ambiguous.

This change narrows the target-facing path back to one truthful active source:
the governed Raspberry Pi package/install/autostart boundary must ship the
active `OBC` / `TopCcsds` binary and dictionary, while legacy artifacts remain
available only as explicit regression or fallback paths.

## What Changes

- Make the governed Raspberry Pi package payload pull `build-artifacts/.../OBC`
  instead of `.../OBC_ComFprimeLegacy`.
- Make source-workspace target launch helpers default to the active `OBC`
  binary instead of silently defaulting to legacy.
- Keep installed-release launch and systemd templates on `bin/OBC`, but make
  that name truthful by correcting the packaged payload behind it.
- Keep legacy probes and wrappers explicit; do not add new legacy dependencies
  or retire legacy in this wave.
- Record fresh Raspberry Pi package/install/autostart evidence on the corrected
  active path.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: make the governed Raspberry Pi target-facing release path
  explicitly active-`OBC` / `TopCcsds`.
- `verification-evidence`: require reviewable evidence that the Raspberry Pi
  package/install/autostart path now ships and boots the active `OBC` payload.

## Impact

- Affected code:
  - `scripts/package_rpi_bundle.sh`
  - `scripts/run_rpi_stack.sh`
  - target package/install/autostart helpers that rely on the governed bundle
  - Raspberry Pi evidence and active-baseline docs
- Public/operator impact:
  - governed Raspberry Pi `bin/OBC` now truthfully points at the active
    `TopCcsds` deployment
  - target package manifests and installed dictionaries now describe the active
    `OBCApp` topology rather than a renamed legacy payload
- Non-goals:
  - no legacy retirement
  - no new COMM CSP lab-operational closure
  - no new external-comm or RF path proof
