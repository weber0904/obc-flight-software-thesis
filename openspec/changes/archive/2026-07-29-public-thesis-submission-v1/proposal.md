## Why

The thesis submission repository must present the implemented CubeSat OBC
flight-software baseline as a clean, reproducible, legally attributable public
release without publishing stale operator entrypoints, duplicate planning
documents, local credentials, or bulky raw evidence in the Git tree. The
private development repository remains the full working source; this change
defines the curated public distribution and the immutable
`thesis-submission-v1` release boundary.

## What Changes

- Create a deterministic public snapshot from development commit
  `142683f20ba46f59f894f594f2caf71dfeddf16f` without importing its Git history
  or untracked workspace content.
- Preserve every existing OpenSpec main spec and archived change while adding
  a release-profile contract that identifies active public requirements and
  retained historical provenance.
- Publish only current product code, transitive support tooling, maintained
  verification entrypoints, and release-aligned documentation.
- **BREAKING** Remove historical, deprecated, fail-closed, duplicate, and
  alias-only validation entrypoints from the public script surface.
- **BREAKING** Replace the tracked command-auth keystore with a tracked example,
  an ignored local runtime copy, and an explicit packaging-time target
  keystore input.
- Consolidate stale architecture, operator, roadmap, reporting, thesis-writing,
  and agent-specific documents into canonical public documentation.
- Preserve all test-record summaries in Git while externalizing raw artifacts
  to a checksummed, redacted GitHub Release asset.
- Add Apache-2.0 project licensing, third-party notices, SBOM, citation,
  security, contribution, provenance, and release manifests.
- Add CI gates for OpenSpec, build/UT, hosted verification, documentation,
  script inventory, evidence integrity, secrets, licensing, and clean-clone
  reproducibility.
- Reuse existing Raspberry Pi and lab evidence only as commit-scoped historical
  evidence; do not claim fresh target verification on the public tag.

## Capabilities

### New Capabilities

- `public-release-profile`: Defines deterministic snapshot provenance, public
  content disposition, evidence externalization, credential safety, licensing,
  validation, and immutable tag requirements.

### Modified Capabilities

- `delivery-workflow`: Adds the separate public-repository PR, CI, tag, release,
  and post-publication immutability gates.
- `documentation-governance`: Replaces parallel agent/reporting/thesis-writing
  truth sources with a compact English-canonical public document set.
- `interface-contract-index`: Replaces the Mission Console planning-handoff
  dependency and tracked-keystore wording with current public interfaces.
- `verification-evidence`: Defines in-tree summaries plus checksummed external
  raw evidence and commit-scoped target claim wording.
- `verification-path-registry`: Requires stable path status and a manifest
  relationship for every published verification entrypoint.
- `planning-docs`: Replaces implementation handoff notes with release-aligned
  current status and future work.
- `project-reporting`: Retires point-in-time reporting packages from the public
  tree after current contribution material is consolidated.
- `architecture-review`: Retires stale `current` review packages from the
  public tree while retaining formal OpenSpec provenance.
- `codex-skills`: Classifies repo-local agent skills as development-source
  tooling that is replaced by standard public contribution guidance.

## Impact

- Public repository: `weber0904/obc-flight-software-thesis`
- Source baseline: development repository commit `142683f2`
- Product surfaces: OBC deployment, simulators, Raspberry Pi packaging,
  Mission Console, current hosted/target validation tooling
- Public interfaces: publication, verification, and evidence JSON manifests;
  `scripts/bootstrap_dev_config.sh`; packaging-only
  `OBC_PACKAGE_KEYSTORE_PATH`
- Dependencies: pinned public F Prime and libcsp/csp-es forks, TooJPEG
- Release assets: sanitized raw evidence archive, checksums, and SPDX SBOM
- No FPP, CCSDS, CSP, command, telemetry, event, or flight-wire contract change
