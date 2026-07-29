## Why

The project has validated S-band and UHF hosted command paths, but it still lacks a formal vocabulary for describing command authority decisions. Existing UHF backup evidence proves bounded command ingress; it does not define which commands a backup link may execute or how future session, sequence, scheduler, and failover work should classify commands without coupling policy directly to individual subsystem components.

This change creates the shared authority vocabulary and policy source needed by the follow-on `command-ingress-authority-v1` implementation. It deliberately stops short of runtime enforcement, cryptographic authentication, session sequencing, replay protection, dynamic failover, file authority, or full link authority.

## What Changes

- Define link identity, configured link role, command class, resource label, authority decision, and rejection reason vocabulary.
- Define a policy source keyed by fully qualified FPP JSON dictionary command names rather than short names or hand-written global opcode values.
- Define the conservative UHF backup command allowlist:
  - `MODE_GET`
  - `EPS_GET_STATUS`
  - `ADCS_GET_ATTITUDE`
  - `GPS_GET_STATE`
  - `RADIO_GET_STATUS`
  - `STORAGE_GET_STATUS`
  - `BOOT_STATUS`
- Define dictionary coverage requirements so every active command in the default CCSDS and legacy ComFprime topology dictionaries must be classified.
- Define `UHF + PRIMARY_AFTER_FAILOVER` as vocabulary and policy-test scope only; runtime dynamic failover is deferred.
- Add traceability from OpenSpec requirements to policy behavior and verification evidence.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: add a command authority vocabulary and classification contract for routed F Prime command packets.
- `verification-evidence`: require dictionary coverage and traceability evidence for command authority vocabulary and policy classification.

## Impact

- Affected artifacts: OpenSpec changes, policy source files, generator inputs, unit tests, evidence records, and pending planning docs.
- Affected runtime behavior: none in this change alone. Runtime enforcement is introduced by `command-ingress-authority-v1`.
- Non-goals: gateway enforcement, full link authority, file/unknown uplink authority, crypto auth, session/sequence windows, replay protection, persistent config store, dynamic UHF primary failover, scheduler, FDIR, HK/data-product changes, and CCSDS route changes.
