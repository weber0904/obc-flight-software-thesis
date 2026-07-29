## Context

The workspace originally contained only narrative drafts in `obc-dev-spec/` and no formal main specs. The drafts mixed architecture, subsystem behavior, testing, Git policy, and OpenSpec guidance across multiple files, with some duplicated ownership of public contracts and one index file referencing attachments that do not exist. The repo also had no Git metadata until this bootstrap change.

This bootstrap must solve two problems at once:

1. Preserve the original design reasoning so future work still has source context.
2. Establish a clean main-spec baseline that later implementation changes can modify without re-litigating the same scope and ownership questions.

## Goals / Non-Goals

**Goals:**

- Create a stable dual-layer documentation model: narrative source docs plus formal OpenSpec specs.
- Collapse 13 legacy narrative drafts into 9 source documents with clear ownership.
- Create 9 OpenSpec capabilities aligned one-to-one with those source documents.
- Normalize currently ambiguous defaults that later code work depends on.
- Archive the bootstrap change so `openspec/specs/` becomes the v1 baseline.
- Leave a concrete queue of follow-on changes for actual implementation.

**Non-Goals:**

- Do not bootstrap the F' project yet.
- Do not implement any simulator, FPP, topology, or runtime code in this change.
- Do not choose vendor-specific radio protocols, bootloader internals, or MCU-level memory layouts.
- Do not create a standalone implementation change for `resource-storage`; that capability remains shared across later changes.

## Decisions

### Decision: Keep narrative docs, but reduce and reframe them

The original drafts contain real design context, so they should not be discarded. Instead, archive the 13 legacy files verbatim under `obc-dev-spec/archive/legacy-v0/` and replace the top-level narrative layer with 9 rewritten source documents.

Alternative considered:

- Keep all 13 files and add more cross-links. Rejected because duplicated contract ownership and dead-link drift would remain.

### Decision: Use one capability per rewritten narrative document

Each rewritten narrative document maps directly to one OpenSpec capability:

- `00_platform_baseline.md` -> `platform-baseline`
- `01_core_system_contracts.md` -> `core-system-contracts`
- `02_resource_storage.md` -> `resource-storage`
- `03_eps_subsystem.md` -> `eps-subsystem`
- `04_adcs_subsystem.md` -> `adcs-subsystem`
- `05_comm_subsystem.md` -> `comm-subsystem`
- `06_boot_update.md` -> `boot-update`
- `07_verification_evidence.md` -> `verification-evidence`
- `08_delivery_workflow.md` -> `delivery-workflow`

Alternative considered:

- Fewer capabilities with broad, mixed ownership. Rejected because later implementation changes would become overly cross-cutting and harder to validate.

### Decision: Enforce single ownership for public symbol families

`core-system-contracts` owns shared types plus `MODE_*`, `HEALTH_*`, `CSP_*`, and `SYS_*`. Subsystem files own their own public symbol families. This removes the old pattern where one central file and one subsystem file both described the same contracts.

Alternative considered:

- Retain a single master dictionary file plus subsystem reiterations. Rejected because it encourages drift and contradictory edits.

### Decision: Record ambiguous defaults now, in the baseline

Three previously soft decisions become formal defaults in this bootstrap:

- `BOOT_CONFIRM` default timeout is 60 seconds; valid config range remains 30-180 seconds.
- Boot metadata v1 is file-backed in persistent storage, not database-backed.
- ADCS acceptance thresholds are mission constants with defaults `< 0.05 rad/s` and `< 5 deg`.

Alternative considered:

- Leave them unresolved until implementation. Rejected because follow-on design and validation tasks need concrete defaults.

### Decision: Use the real OpenSpec CLI lifecycle as the authoritative workflow

The official delivery workflow uses `openspec new change`, `status`, `instructions`, `validate`, and `archive`. Manual archive by moving directories is explicitly removed from the formal process.

Alternative considered:

- Keep the old manual archive guidance because it is simpler to describe. Rejected because it bypasses main-spec synchronization and weakens repeatability.

### Decision: Seed the follow-on queue immediately after archive

The repo should not archive this bootstrap and then leave the next steps implicit. Create lightweight follow-on change containers so the next implementation cycle is already named and staged.

Alternative considered:

- Document the queue only in prose. Rejected because actual change scaffolds make the next steps operational and discoverable through OpenSpec tooling.

## Risks / Trade-offs

- **Narrative/formal drift can reappear** -> Narrative docs explicitly declare that formal specs win, and later changes must update both layers.
- **This bootstrap is docs-heavy without code** -> It is intentionally the only governance-only change; later changes must include implementation and verification tasks.
- **Follow-on changes may refine defaults later** -> Defaults are narrow and intentionally configurable; changes to them must go through OpenSpec deltas.
- **No F' tree exists yet** -> The platform baseline documents the future bootstrap path and defers actual project generation to `bootstrap-fprime-platform`.

## Migration Plan

1. Initialize Git and OpenSpec scaffolding in the workspace.
2. Archive the 13 legacy narrative drafts under `obc-dev-spec/archive/legacy-v0/`.
3. Rewrite `obc-dev-spec/` into 9 source documents with source maps and ownership boundaries.
4. Create the `formalize-obc-baseline` proposal, design, specs, and tasks.
5. Run consistency checks and `openspec validate`.
6. Archive the bootstrap change so `openspec/specs/` becomes the formal v1 baseline.
7. Create the follow-on change queue for platform bootstrap and subsystem implementation.

## Open Questions

None that block this bootstrap. Implementation-specific questions are intentionally deferred to the follow-on changes.
