## Why

The repository needs a concise portfolio-facing documentation layer that lets
reviewers understand the system, architecture, operation, and verification
without navigating internal release-governance material or a large evidence
archive.

## What Changes

- Reduce `docs/` to a small set of canonical reader documents.
- Consolidate overlapping hosted, target/lab, Mission Console, simulator, and
  thesis-demonstration runbooks.
- Move test records and the detailed verification-path ledger into a dedicated
  top-level `evidence/` archive with a curated index.
- Move the baseline reconciliation source and generated review surface under
  `openspec/reconciliation/`.
- Rewrite repository, architecture, verification, thesis, security, and
  release prose around implemented capabilities and reproducible workflows.
- Update evidence generators, publication checks, documentation checks, links,
  and manifests for the new structure.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `documentation-governance`: define a compact reader-facing documentation
  surface and separate it from engineering evidence.
- `verification-evidence`: publish test-record summaries under the dedicated
  evidence archive.
- `verification-path-registry`: separate the concise verification overview
  from the detailed path ledger.
- `delivery-workflow`: place reconciliation data with OpenSpec governance
  rather than reader documentation.
- `planning-docs`: express future engineering work in the architecture overview
  instead of a separate public roadmap family.
- `public-release-profile`: require portfolio prose to present the system
  directly without publication-process commentary.

## Impact

- Documentation paths and internal links change.
- Evidence generation and release-boundary checks change paths but preserve
  record contents, digests, and release-asset coverage.
- No FPP, flight-software, simulator, packaging, protocol, or runtime behavior
  changes.
