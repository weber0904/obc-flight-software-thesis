## Context

The current public tree has a strong evidence base but exposes too many
development-oriented document families at the same level as the portfolio
narrative. `docs/` contains overlapping operator runbooks, roadmap material,
internal reconciliation data, a multi-thousand-line path ledger, and the full
test-record archive. The result is accurate but difficult to review.

The change must preserve OpenSpec history, evidence digests, executable
ownership, licenses, and reproducibility while improving the first-read
experience.

## Goals / Non-Goals

**Goals:**

- Make `README.md` and `docs/README.md` sufficient for a technical reviewer to
  understand the project and choose the next document.
- Keep no more than one current document per major topic.
- Place detailed engineering evidence under a clearly named top-level archive.
- Preserve test-record summaries, artifact descriptors, catalog entries, and
  checksums.
- Keep operator instructions executable and aligned with shipped scripts.
- Use direct technical prose instead of discussing the publication process.

**Non-Goals:**

- Change flight-software, simulator, protocol, topology, package, or security
  behavior.
- Rewrite archived OpenSpec decisions or individual evidence conclusions.
- Delete evidence records or weaken release-boundary checks.
- Publish, push, merge, tag, or upload release assets.

## Decisions

### Reader documentation and engineering evidence use separate roots

`docs/` will contain only current explanatory and operational material.
`evidence/` will contain the catalog, detailed verification-path ledger, and
test-record summaries. This makes the repository hierarchy communicate intent
without requiring explanatory disclaimers.

### Current documents are consolidated by reader task

The current layer will consist of:

- project architecture and contributions
- interface contracts
- verification overview
- hosted operations
- target/lab operations
- Mission Console
- simulator controls
- thesis demonstration and claim map

Each topic has one entrypoint. Detailed scripts remain documented in
`scripts/README.md`.

### Reconciliation stays with OpenSpec

The reconciliation JSON and generated Markdown move to
`openspec/reconciliation/`, beside the formal specs and archived changes they
audit. Repository checks and generators follow the new paths.

### Evidence records move mechanically

Every public `evidence/records/<id>/` path moves to
`evidence/records/<id>/`. Evidence generation, catalogs, descriptors, link
targets, publication classification, and checks update together. Record body
content remains unchanged except for path references needed by the move.

### Public-facing prose is capability-first

README, current docs, security, and release pages describe implemented
behavior, supported workflows, measured verification, and evidence locations.
Release-curation rationale remains in OpenSpec and machine-readable manifests,
not in the portfolio narrative.

## Risks / Trade-offs

- [Large path migration can leave broken links] → Rewrite repository-relative
  references mechanically, then run current-doc, OpenSpec, evidence, and clean
  clone checks.
- [Consolidation can omit an operator detail] → Map every shipped current
  runbook section to a destination before deleting old files.
- [Evidence tools can disagree on old and new paths] → Change generator,
  checker, catalog, and descriptors in one commit and require exact record and
  artifact counts.
- [Historical OpenSpec text can retain old paths] → Apply a deterministic path
  rewrite in the public copy while preserving the original source digest in
  the publication manifest.
