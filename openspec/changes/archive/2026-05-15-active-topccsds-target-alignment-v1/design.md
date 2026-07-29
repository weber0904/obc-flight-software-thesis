## Context

The current active baseline is already `OBC` / `TopCcsds`:

- `OBC/Main.cpp` boots `TopCcsds`
- hosted command-security probes already target `OBCApp`
- installed Raspberry Pi launchers already invoke `bin/OBC`

The drift is in the artifact boundary that precedes those launchers:

- `scripts/package_rpi_bundle.sh` still copies
  `build-artifacts/Linux/OBC_ComFprimeLegacy/bin/OBC_ComFprimeLegacy`
  and its dictionary, then renames that binary to `bin/OBC`
- `scripts/run_rpi_stack.sh` still defaults `OBC_BINARY_NAME` to
  `OBC_ComFprimeLegacy`

That makes the target package/install/autostart path appear active while still
running legacy bits.

## Design

### Active release source

The governed Raspberry Pi release path SHALL use the active Linux deployment:

- binary: `build-artifacts/Linux/OBC/bin/OBC`
- dictionary: `build-artifacts/Linux/OBC/dict/AppTopologyDictionary.json`

The package continues to install those artifacts as:

- `bin/OBC`
- `dict/AppTopologyDictionary.json`

This preserves the current installed launcher and systemd template surface while
making the packaged payload truthful.

### Source-workspace target helper alignment

The source-workspace Raspberry Pi helper path SHALL default to `OBC`, not
`OBC_ComFprimeLegacy`.

This is a bounded alignment fix only for the helper surfaces needed by follow-up
02:

- `scripts/run_rpi_stack.sh`
- package/install/autostart helpers that depend on the produced release

Legacy-only operational or lab probes may remain legacy if they are not part of
the formal 02 evidence path, but they must stay explicit rather than pretending
to be the active `OBC` path.

### Legacy boundary

`Top` / `OBC_ComFprimeLegacy` remain buildable and available for:

- regression comparison
- explicitly legacy probes
- temporary fallback during future cleanup work

This change does not:

- remove legacy binaries
- rename legacy scripts into active truth
- expand legacy package ownership

### Evidence boundary

This change closes only the active target artifact alignment needed by 02. Fresh
evidence must show:

1. the produced package manifest and staged bundle contain the active `OBC`
   payload and active dictionary
2. installed current-release launch still works on the corrected package
3. autostart or reboot relaunch still works on the corrected package path

The evidence is packaging and target-startup truth, not new COMM CSP
lab-operational closure.

## Risks And Boundaries

- Target-side scripts that still intentionally target legacy may remain out of
  scope for this wave.
- The change must not introduce any new dependency from active package/install
  flow back onto `OBC_ComFprimeLegacy`.
- The active path proof remains limited to the governed Raspberry Pi package,
  install, and autostart boundaries needed by follow-up 02.
