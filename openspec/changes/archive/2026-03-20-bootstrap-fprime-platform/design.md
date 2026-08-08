## Context

The repository now has formal OpenSpec governance and a narrative source layer, but it still lacks the actual F' project scaffold that future subsystem work depends on. The platform baseline already committed to a single official bootstrap path using F' v4.1.0, so this change is the first implementation-oriented execution of that commitment.

This workspace is unusual because it is not an empty directory: it already contains `openspec/`, `obc-dev-spec/`, and project-local `.codex/skills/`. The bootstrap therefore needs to populate the current directory instead of creating a fresh nested project elsewhere.

## Goals / Non-Goals

**Goals:**

- Populate the current workspace with the F' v4.1.0 project skeleton in place.
- Preserve the existing OpenSpec and narrative-doc structures.
- Establish the generated virtual environment and baseline files needed for later `fprime-util` usage.
- Ensure the workspace contains or explicitly adds the directories referenced by the formal baseline (`OBC/`, `simulators/`, `scripts/`, `docs/`, `.github/` where applicable).
- Produce concrete bootstrap evidence through generate/build checks.

**Non-Goals:**

- Do not implement subsystem code, FPP types, topology wiring, simulators, or drivers yet.
- Do not complete Raspberry Pi cross-compilation or target-specific hardware setup.
- Do not archive this change until bootstrap artifacts, generated structure, and validation evidence are in place.

## Decisions

### Decision: Populate the existing root directory

Use `fprime-bootstrap project --populate --path . --tag v4.1.0` directly in the current repository root instead of generating a nested project and copying files back.

Alternative considered:

- Bootstrap into a temporary or nested directory and merge manually. Rejected because it would create more drift risk and complicate later OpenSpec-tracked edits.

### Decision: Preserve OpenSpec-governed content as first-class project content

The repository already has governance-critical content. Bootstrap output must coexist with `openspec/`, `obc-dev-spec/`, and `.codex/skills/` instead of replacing them.

Alternative considered:

- Treat OpenSpec/doc files as external to the project tree. Rejected because the formal baseline explicitly makes them part of the governed workspace.

### Decision: Patch around bootstrap omissions locally

If the F' bootstrap output does not create every directory expected by the formal baseline, add the missing project-local directories or placeholders in this change rather than weakening the baseline immediately.

Alternative considered:

- Change the baseline first to match the bootstrap tool's minimal output. Rejected because the formal baseline intentionally includes simulator, docs, and scripts locations for near-term work.

### Decision: Treat generate/build as the bootstrap gate

After bootstrap, run the generated `fprime-util` from the project virtual environment for `generate` and `build`, and record the outcome as the minimum bootstrap evidence.

Alternative considered:

- Stop after project generation only. Rejected because the formal verification baseline already calls for bootstrap-phase gate evidence.

### Decision: Use a Python interpreter supported by F' v4.1.0

The generated `fprime-venv/` must use a Python interpreter supported by the `v4.1.0` dependency set. In this workspace, `python3` resolved to 3.14 and caused the `pydantic_core` build to fail, so the validated bootstrap path uses `python3.13 -m venv fprime-venv`.

Alternative considered:

- Keep the `python3.14` virtual environment and document partial incompatibility. Rejected because it would make the bootstrap baseline non-reproducible for `v4.1.0`.

## Risks / Trade-offs

- **Bootstrap may require network or extra permissions** -> Retry with escalated approval if sandboxed network or global-cache access blocks the command.
- **Bootstrap output may differ from the formal target tree** -> Add minimal supplementary directories or placeholders and document the outcome in evidence.
- **Generated files may overlap with existing root files** -> Inspect resulting changes carefully and preserve OpenSpec/narrative content.
- **`fprime-util` is not currently available outside the generated venv** -> Run it via the virtual-environment path once bootstrap succeeds.
- **Host default Python may be newer than the supported dependency set** -> Recreate `fprime-venv/` with an explicitly compatible interpreter such as `python3.13`.

## Migration Plan

1. Create the change artifacts for the bootstrap work.
2. Run `fprime-bootstrap project --populate --path . --tag v4.1.0`.
3. Inspect generated files and reconcile them with the existing governance files.
4. Add any missing baseline directories or placeholders required for the near-term project layout.
5. Run `fprime-venv/bin/fprime-util generate`.
6. Run `fprime-venv/bin/fprime-util build`.
7. Record the bootstrap evidence under `evidence/records/bootstrap-fprime-platform/`.
8. Validate the change and leave it ready for archive once tasks are complete.

## Open Questions

None that block execution. If bootstrap fails because of sandboxed network or permission limits, escalate and retry rather than redesigning the change.
