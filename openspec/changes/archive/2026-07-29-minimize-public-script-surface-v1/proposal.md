## Why

The public repository still exposes a development-scale script inventory,
including compatibility implementations, narrowly scoped probes, redundant
wrappers, and reader-facing release commentary. A portfolio release needs a
small operational surface centered on reproducible build, demonstration, and
verification workflows.

## What Changes

- **BREAKING**: remove script entrypoints that are historical, diagnostic,
  superseded, duplicated, or not part of the public build and demonstration
  workflow.
- Retain only release governance, primary hosted operation, governed target
  baselines, integrated thesis routes, Mission Console, and their dependency
  closure.
- Replace the exhaustive development inventory with a concise public script
  catalog and machine-checked allowlist.
- Remove empty-directory placeholders and ignored local artifacts from the
  candidate checkout.
- Extend public-release validation to reject scripts outside the allowlist and
  release-process commentary in script documentation.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `public-release-profile`: require a minimal allowlisted public script
  surface rather than every source script classified as maintained.
- `documentation-governance`: apply capability-first prose checks to script
  documentation as well as the main reader-document layer.

## Impact

The change affects `scripts/`, its public manifest, documentation links,
release-boundary checks, and verification entrypoints. Development history and
formal evidence remain available under `openspec/` and `evidence/` without
keeping every historical implementation executable.
