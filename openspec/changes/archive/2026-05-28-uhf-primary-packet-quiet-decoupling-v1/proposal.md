## Why

The current UHF formal-path baseline already distinguishes beacon suppress from
official file/data-product downlink, but the runtime wiring and spec wording
still describe packet suppression and beacon suppress too closely as one active
UHF command-session quiet concept. That wording is no longer honest enough for
the current baseline.

This change is needed now because the active UHF baseline must state two
separate truths at the same time: formal UHF live packet suppression begins as
soon as UHF becomes the current primary band, while UHF beacon suppress still
begins only after an accepted authenticated `SESSION_OPEN(seq0)` and later
same-session accepted activity.

## What Changes

- Split formal UHF live packet suppression from UHF beacon suppress/runtime.
- Make formal UHF live `event/tlm` packet suppression depend directly on
  `currentPrimaryBand == UHF`.
- Keep UHF beacon suppress/runtime tied to accepted authenticated
  `SESSION_OPEN(seq0)` and later accepted same-session UHF activity.
- Preserve official file/data-product downlink as a formal capability while
  UHF primary packet quiet is active.
- Keep stronger diagnostic quiet semantics separate from the new UHF
  primary-driven packet quiet semantics.
- Add a dedicated hosted verification-path/evidence surface for UHF primary
  packet quiet, separate from the existing hosted beacon suppress/runtime path.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: redefine formal UHF live packet suppression as a
  primary-band-driven policy while keeping beacon suppress accepted-session
  driven and file/data-product downlink formal.
- `interface-contract-index`: record separate runtime/state semantics for UHF
  primary packet quiet versus UHF beacon suppress/runtime.
- `live-beacon-broadcast`: preserve COMM-owned accepted-session-driven beacon
  suppress/runtime semantics while separating them from packet quiet.
- `verification-evidence`: require evidence that separately proves UHF primary
  packet quiet, UHF beacon suppress/runtime, and formal file/data-product
  preservation.
- `verification-path-registry`: add a distinct hosted UHF primary packet quiet
  registry path rather than reusing the hosted beacon suppress/runtime path.

## Impact

- Affected code: `CommController` runtime policy, `CommEgressMux` packet quiet
  naming and enforcement, and focused unit coverage around both components.
- Affected docs/specs: current baseline, interface, follow-up, verification
  registry, and the modified OpenSpec main specs above.
- Affected verification: one new hosted proof path is needed to keep packet
  quiet separate from beacon suppress/runtime and from official file/downlink
  behavior.
