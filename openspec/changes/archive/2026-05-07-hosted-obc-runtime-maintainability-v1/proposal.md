## Why

The default hosted `OBC` runtime and the CCSDS spike runtime now carry long, closely mirrored command parsing and dispatch loops. This makes future command-heavy work harder to review because runtime-structure changes can become mixed with link-proof, parser, or behavior changes.

## What Changes

- Refactor shared hosted runtime command parsing and dispatch into a smaller table-driven or handler-based helper module.
- Keep the default `OBC` executable on the existing `ComFprime` topology and keep `OBC_CcsdsGroundLinkSpike` as a separate spike-only `ComCcsds` executable.
- Preserve existing operator command names, arguments, visible CLI output markers, F' command/event/telemetry behavior, housekeeping archive fields, COMM node identities, and COMM service layouts.
- Add focused L1 tests for extracted parser/dispatch helper behavior.
- Record evidence that the refactor reuses existing validation paths and does not create new S-band, UHF, CCSDS, direct-GDS, target/Pi, RF, reliable-transfer, command-authority, or failover claims.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: require hosted runtime variants to share maintainable command-dispatch/runtime helper logic while keeping topology and protocol variants adapter-owned.
- `verification-evidence`: require behavior-preservation evidence for runtime maintainability refactors and explicit separation from new validation-path claims.

## Impact

- Affects the hosted runtime entrypoints and build registration around `OBC/Main.cpp`, `OBC/MainCcsdsGroundLinkSpike.cpp`, and a new shared runtime helper module.
- Adds focused helper tests for command dispatch and argument parsing.
- Does not modify public F' component interfaces, command opcodes, telemetry channels, event definitions, HK data product fields, COMM CSP services, topology proof boundaries, or deployment defaults.
