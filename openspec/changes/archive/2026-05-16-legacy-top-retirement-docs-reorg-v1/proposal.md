## Why

The active baseline has converged on `OBC/TopCcsds` plus the `OBC` deployment, while the legacy `OBC/Top` / `OBC_ComFprimeLegacy` path still exists in build targets, scripts, policy catalogs, and narrative documents. This keeps obsolete paths alive enough to confuse operators, reviewers, thesis material, and future agents.

## What Changes

- **BREAKING**: Remove the maintained legacy `OBC/Top` / `OBC_ComFprimeLegacy` runtime and build surface.
- Retire ComFprime-only launch and probe scripts whose only active purpose was legacy regression.
- Port current lab/operator scripts that accidentally still use legacy dictionary names or command prefixes onto the active `OBC` / `OBCApp.*` path.
- Remove legacy command-policy entries and verification-inventory references.
- Reorganize `docs/` so active architecture, roadmap, evidence, thesis, historical reporting, and retired material have distinct homes.
- Move repo-tracked thesis material from `docs/reporting/thesis/` to `docs/thesis/` and refresh it against the current mainline state.
- Compress roadmap into current baseline, next-work, and historical-completed handoff files.
- Preserve old test records as historical evidence while making current indexes and docs clear that legacy Top is retired.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: remove the legacy Top/ComFprime deployment from the maintained build baseline.
- `comm-subsystem`: remove active legacy ComFprime regression expectations and keep ComFprime evidence historical only.
- `core-system-contracts`: remove legacy command namespace coverage from current command-policy expectations.
- `verification-evidence`: update current evidence expectations after retiring buildable legacy Top surfaces.
- `verification-path-registry`: update registry semantics for active CCSDS paths versus historical ComFprime evidence.
- `planning-docs`: make the documentation information architecture explicit and remove retired redirect layers.

## Impact

- Affected build/runtime surfaces: root deployment registration, `OBC/CMakeLists.txt`, `OBC/Top/`, `OBC/MainComFprimeLegacy.cpp`, command authority policy, verification inventory helper paths.
- Affected scripts: legacy ComFprime wrappers/probes are removed; active lab/target scripts use active dictionary and `OBCApp.*` command names.
- Affected docs: `README.md`, `AGENTS.md` if needed, `docs/architecture/`, `docs/roadmap/`, `docs/thesis/`, `docs/reporting/`, `docs/interfaces.md`, `evidence/verification-path-registry.md`, and related indexes.
- Verification requires full CI because build targets and scripts change.
