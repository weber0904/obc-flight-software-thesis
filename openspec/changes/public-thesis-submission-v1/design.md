## Context

The development repository contains the complete implementation history, 34
main OpenSpec capabilities, 187 archived changes, 149 executable scripts, 158
test-record directories, and about 124 MB of checked-in raw evidence. It also
contains point-in-time reporting, architecture-review, thesis-writing,
agent-specific, historical operator, and fail-closed verification surfaces
that are useful during development but make a public portfolio repository
ambiguous and unnecessarily large.

The target public repository already exists with one placeholder commit. The
public release must preserve reviewable engineering provenance without
importing old Git objects that could retain deleted files, credentials, local
paths, or large blobs. The final public tag is an academic/research software
release, not a claim of flight certification.

## Goals / Non-Goals

**Goals:**

- Produce a clean, deterministic snapshot of the declared development commit.
- Keep every OpenSpec spec and archived change reviewable.
- Make current product, operator, verification, and evidence entrypoints
  unambiguous.
- Keep all evidence summaries while moving raw artifacts to a checksummed
  release asset.
- Publish a complete Apache-2.0 and third-party attribution boundary.
- Prevent tracked deployment credentials and personal environment defaults.
- Prove the public snapshot from a clean clone with full local and hosted gates.
- Preserve prior target evidence with exact provenance and conservative wording.

**Non-Goals:**

- Rewriting or deleting content in the development repository.
- Importing development Git history.
- Changing FPP, topology, commands, telemetry, events, CCSDS, CSP, or flight
  behavior.
- Upgrading F Prime or libcsp.
- Re-running Raspberry Pi, serial, SocketCAN, or watchdog target proofs.
- Claiming RF closure, flight certification, or fresh hardware verification for
  the public tag.

## Decisions

### Curated history instead of history filtering

The public branch starts from the existing public repository and adds only
already-curated files in logical commits. The development repository is read
through Git objects at the fixed source commit; its working directory and Git
history are never copied.

Rejected alternatives:

- `git filter-repo` keeps a large and hard-to-audit ancestry.
- A single snapshot commit hides the organization and review sequence.
- Committing a raw import and deleting later leaves excluded content in public
  Git objects.

### Exhaustive disposition manifest

`release/publication-manifest.json` classifies every tracked source path through
non-overlapping include, transform, externalize, or exclude rules. A checker
fails when a source path has zero or multiple decisions. Explicit exceptions
record the successor and reason for removed high-value documents and scripts.

### OpenSpec is retained but the public profile defines applicability

All existing main specs and archived changes are copied. Existing archived
changes remain byte-identical unless an actual secret is discovered. Main
specs are reconciled through this governed change, and a new
`public-release-profile` spec defines which source-only governance and
point-in-time packages are not public distribution requirements.

### Script publication uses a dependency closure

Maintained CI, baseline, package/install, current proof, manual-ops, and Mission
Console entrypoints form the retained seed set. Their transitive helpers are
retained as `support/internal`. Historical, deprecated, fail-closed,
alias-only, and unreachable wrappers are excluded. Any remaining executable
without a current registry/runbook owner is excluded rather than promoted.

### Raw evidence is a versioned release asset

Every test-record summary remains in Git. Everything below a test record's
`artifacts/` directory is copied into one deterministic tar archive after
textual redaction. Each externalized record receives `ARTIFACTS.json`; a global
catalog and checksum file bind the in-tree summary to the release asset.

The public manifest records both the original source digest and sanitized
digest. Binary evidence is not rewritten. This preserves traceability while
keeping clones small.

### Credentials are provisioned before runtime, not injected at runtime

The repository tracks a documented example keystore only. A bootstrap command
creates the ignored fixed-path local file for hosted development. The runtime
continues to reject command-auth CLI/environment overrides.

Target packaging accepts `OBC_PACKAGE_KEYSTORE_PATH`, validates that it is not
the public example, copies it to the fixed installed path, and records its
digest in the release manifest. This is packaging-time provisioning, not a
runtime authority bypass.

### Hardware evidence is explicitly commit-scoped

No target probe is rerun. Published target records are labeled `previously
demonstrated` or `historical target evidence`. A release-delta ledger identifies
whether public changes touch each claim. No current badge or summary may say
that target behavior was verified on `thesis-submission-v1`.

### English is canonical

README, architecture, interface, contribution, verification, security, and
general operator documentation use English. Traditional Chinese is limited to
a concise repository summary, thesis claim/evidence map, and integrated
Route 1/2/3 demonstration guide.

### Dependency licensing remains separated

Original project code uses Apache-2.0. F Prime remains Apache-2.0 with its
NOTICE and modification notices. libcsp remains MIT. TooJPEG remains under its
bundled zlib-style terms. The root license does not relicense third-party code.

## Risks / Trade-offs

- [Historical OpenSpec links can name excluded files] → The publication
  manifest maps excluded paths to a current successor or an intentional
  exclusion; current-doc link checks ignore only declared archived references.
- [Redaction changes raw text evidence bytes] → Publish original and sanitized
  digests plus deterministic rules; never call sanitized bytes the original.
- [Public example credentials could be deployed accidentally] → Ignore the
  runtime keystore, require explicit target package input, and reject the
  example fingerprint.
- [F Prime license notices may require a new dependency commit] → Complete the
  fork audit before freezing final dependency SHA and rerun the full build.
- [No fresh target run means the release cannot claim current hardware closure]
  → Use conservative commit-scoped wording and make hosted/full-build gates the
  actual release acceptance boundary.
- [Large documentation removal can break references] → Run current-doc link
  checks, OpenSpec validation, manifest coverage, and clean-clone verification.
- [Release assets can be replaced on GitHub] → Commit their SHA-256 in the tag;
  a replaced asset is detectable.

## Migration Plan

1. Create the public release branch and OpenSpec change.
2. Generate the source disposition inventory before staging imported content.
3. Import sanitized product code and exact submodule gitlinks.
4. Apply credential and licensing changes.
5. Compute the retained script dependency closure and write its catalog.
6. Rewrite current docs and externalize raw evidence.
7. Add CI and run clean-clone/full-hosted verification.
8. Sync and archive this OpenSpec change.
9. Commit logical publication boundaries locally.
10. After explicit approval, push, open a ready PR, wait for green CI, merge,
    create the annotated tag, and upload the checksummed release assets.

Rollback before publication is branch deletion. After publication the tag is
immutable; normal corrections use `thesis-submission-v1.1`.

## Open Questions

None. Product, history, evidence, language, licensing, and hardware-gate choices
are fixed by the approved implementation plan.
