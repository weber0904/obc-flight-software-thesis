## Context

The first script reduction replaced the development inventory with an
allowlist, but retained cohesive directories without proving the necessity of
each file. The remaining tree contains operator entrypoints, application code,
route orchestration, tests, static assets, and internal support modules; these
different roles must not be judged by filename alone.

Both hosted and target/laboratory workflows are public deliverables. Current
operator guides, CI/CMake commands, the verification-path registry, governing
evidence records, and Chapter 5 demonstrations define the supported entrypoint
set. A historical mention alone does not require a script to remain, but a
current evidence-production or evidence-integrity path does.

## Goals / Non-Goals

**Goals:**

- Read every retained file and record its actual behavior and dependencies.
- Preserve complete host and target operator workflows.
- Prefer a small set of integrated verification paths over incremental probes
  that exercise only one development milestone.
- Make every retained file discoverable through a file-level catalog.
- Enforce consistency between the catalog, allowlist, filesystem, docs, and
  executable manifest.

**Non-Goals:**

- Delete historical evidence or OpenSpec changes.
- Treat every historical script name as a maintained current entrypoint.
- Reorganize F Prime component source or change flight-software behavior.
- Publish every lab diagnostic used during development.

## Decisions

1. Build a full-content audit containing path, type, purpose, parent workflow,
   inbound dependencies, and disposition for every pre-audit file.
2. Define roots from current operator manuals, root onboarding, CI/CMake, the
   verification-path registry, governing evidence records, and the three
   integrated thesis routes. Recursively retain their shell calls, Python
   imports, templates, static files, and required test fixtures.
3. Keep representative end-to-end verification for hosted and target
   environments. Remove narrower probes when the integrated route or retained
   end-to-end probe covers the same public story.
4. Treat Mission Console as an application: retain its imported Python modules,
   templates, static assets, manual-surface backend, and focused tests.
5. Retain target deployment/install/status and A/B baseline owners because they
   are required by the target operator guide, even when they are not runnable
   on a generic host.
6. Retain the target Chapter 5 runners, their A/B/C stages, and the formal
   Route 1 campaign/provenance tools because they are the maintained producers
   and validators for current evidence-registry paths.
7. Publish `scripts/CATALOG.md` as the concise retained-file reference and keep
   the larger audit in the archived OpenSpec change.
8. Make governance compare the catalog paths, allowlist expansion, tracked
   script paths, executable manifest, and current documentation commands.

## Risks / Trade-offs

- [Dynamic dependency is missed] → Combine full-text review, static call/import
  extraction, unit tests, shell syntax checks, hosted probes, and the complete
  F Prime gate.
- [A useful low-level diagnostic is removed] → Preserve its evidence and Git
  history; reintroduce it only when it becomes a supported public workflow.
- [Catalog becomes maintenance overhead] → Generate or validate its path index
  automatically and fail CI on drift.
