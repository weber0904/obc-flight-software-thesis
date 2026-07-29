## Why

The current repository truth for command ingress, CCSDS framing, timing,
official data products, recovery surfaces, and sequencing is spread across
code, topology, evidence records, and roadmap prose. The deep architecture
review follow-ups for interface contracts and hosted-versus-target timing are
still open, while several current narrative layers also drift behind the now
merged `canonical-state-hk-fallback-v1`, `ground-link-boundary-split-v1`, and
`persistent-fault-ring-v1` baselines.

## What Changes

- Add `docs/interfaces.md` as a non-normative current interface contract index
  for the active baseline.
- Record the current hosted CCSDS framing identifiers, APID mapping,
  command-envelope shape, COMM role ownership, official `.fdp` history
  boundary, persistent fault readback surface, and governed sequencing
  wrapper surface in one checked-in location.
- Absorb the hosted-versus-target rate-group timing follow-up into the same
  document by recording current hosted timing truth and keeping target
  flightlike timing values explicitly `TBD`.
- Refresh architecture-review companion docs, roadmap follow-up notes, and
  current architecture/roadmap prose so completed 01/03/06 work is no longer
  described as pending or active.

## Capabilities

### New Capabilities

- `interface-contract-index`: define the checked-in non-normative
  `docs/interfaces.md` contract index and the truth/status rules it follows.

### Modified Capabilities

None.

## Impact

- Affected docs: `docs/interfaces.md`, `docs/architecture-review/current/`,
  `docs/roadmap/architecture-review-followups/`, and current architecture /
  roadmap narrative layers.
- Affected formal baseline: one new documentation/governance capability under
  `openspec/specs/`.
- No change to runtime code, topology wiring, timing constants, or wire
  formats.
