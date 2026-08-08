## Why

The repository's current secure baseline already runs through challenge-auth
plus secure command v2 for the maintained hosted and target secure-command
paths, but current docs, registry wording, and several older proof wrappers
still describe legacy command envelope v1 and wire `SESSION_OPEN(seq0)` as if
they were part of the preferred active baseline.

That wording now misleads later work in two ways:

- it encourages developers to treat retained legacy compatibility as the
  default command-session model instead of the new secure baseline
- it hides the exact remaining follow-up cleanup surfaces by mixing historical legacy proof
  families with still-maintained secure or payload/operator paths

This change retires legacy v1 as a current-baseline crutch without pretending
that every adjacent proof surface must be migrated in the same slice. It
starts from an explicit dependency audit, removes current wording pollution,
demotes superseded legacy proof families, strips the active secure baseline off
legacy keystore tuple semantics, and records only the narrower remaining
follow-up surfaces where migration is not yet complete.

## What Changes

- Add a governed OpenSpec audit for legacy command-envelope retirement with a
  checked-in retirement matrix covering docs, specs, scripts, and active proof
  surfaces.
- Rewrite current docs/specs/registry so secure auth success, not wire
  `SESSION_OPEN`, is the preferred active operator-facing command-session
  boundary.
- Keep repo-internal synthesized opened-session state intact as a runtime
  implementation detail needed by `CommController` and adjacent runtime owners.
- Demote legacy proof families such as command-envelope metadata, session
  sequence/lifecycle, and persistent freshness from current-baseline authority
  into historical compatibility evidence.
- Record the exact remaining supplemental proof gaps for any later cleanup
  slices, limited to non-core proof wrappers that are not shallow to migrate
  in this slice.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: legacy command envelope v1 remains a historical
  code/test/archive surface, but no longer defines the preferred current
  command-session model or the tracked comm-managed runtime contract.
- `comm-subsystem`: secure auth completion remains the preferred UHF command
  boundary; legacy `SESSION_OPEN(seq0)` becomes explicit compatibility-only
  wording.
- `interface-contract-index`: `docs/interfaces.md` must distinguish current
  secure-baseline semantics from compatibility-only legacy keystore/session
  fields.
- `live-beacon-broadcast`: current beacon suppress wording must use
  auth-success session ownership as the preferred baseline and keep any legacy
  mention compatibility-scoped.
- `verification-path-registry`: superseded legacy proof families become
  historical compatibility citations rather than current preferred authority.
- `verification-evidence`: evidence rules must distinguish historical legacy
  proof families from current secure-baseline proof requirements.

## Impact

- Affected docs/specs:
  - `README.md` and current branch docs where legacy wording still appears as
    active baseline truth
  - `docs/interfaces.md`
  - `docs/architecture/current-development-architecture.md`
  - `evidence/verification-path-registry.md`
  - relevant operator runbooks
  - relevant `openspec/specs/*`
- Affected runtime/config surfaces:
  - secure-auth runtime key provisioning is decoupled from legacy
    `source_id` / `key_slot` tuple application
  - repo-tracked keystore assets now carry only `module_serial` plus
    per-service root-key material for the current secure baseline
- Non-goals:
  - no redesign of secure auth or secure command v2
  - no encryption
  - no hardware-backed key storage
  - no broad observability redesign
  - no forced migration of official sequencing or target timing unless the
    dependency is truly shallow to remove
