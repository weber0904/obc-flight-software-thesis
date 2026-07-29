## Context

The project already documents what verification evidence must exist and which gates matter before delivery, but it still lacks a repo-local implementation of those rules. That gap shows up in two places: every verification run still depends on the operator remembering the exact command sequence, and there is no repository-owned CI workflow that future changes can inherit without restating the same steps.

This change turns the documented delivery and evidence rules into project assets. The goal is not to build a complicated CI matrix. The goal is to make the existing baseline gate reproducible with one shared script locally and in GitHub Actions, and to give later changes a consistent evidence template instead of free-form records.

## Goals / Non-Goals

**Goals:**

- Provide one repo-local script that runs the current formal verification gate end to end.
- Provide one repository-owned GitHub Actions workflow that delegates to the script rather than duplicating commands inline.
- Provide a reusable evidence template under the documentation tree.
- Record the initial CI/evidence implementation result in the repository.

**Non-Goals:**

- Do not add deployment-specific HIL jobs or Raspberry Pi runners.
- Do not add release packaging, publication, or scheduled jobs in this change.
- Do not replace the per-change evidence records; this change standardizes them.

## Decisions

### Decision: Keep the CI gate script repository-local and shell-based

The project will add `scripts/run_verification_ci.sh` as the single entrypoint for the current verification gate. It will run:

- `fprime-util generate -f`
- `fprime-util build`
- `fprime-util generate --ut -f`
- `fprime-util build --ut`
- `fprime-util check --all`
- `openspec validate --specs`

The script will also write a simple markdown summary plus per-step logs to an artifact directory.

Alternative considered:

- Put all logic directly inside the GitHub workflow YAML. Rejected because local and CI behavior would drift immediately.

### Decision: Use one GitHub Actions workflow for the baseline gate

The repository will add one workflow that provisions Python, Node, `zeromq`, and the project dependencies, initializes submodules, and then calls the shared script.

Alternative considered:

- Split the build/test/spec validation into separate workflows first. Rejected because the current project still benefits more from one clear baseline gate than from workflow fan-out.

### Decision: Standardize evidence through a checked-in template

The project will add a markdown template under `docs/test-records/templates/` so later changes can capture environment, commands, results, constrained statuses, and notes in a uniform structure.

Alternative considered:

- Continue letting each change invent its own evidence shape. Rejected because reviews are already comparing multiple capability records.

## Risks / Trade-offs

- **GitHub Actions setup is host-dependent** -> Install required packages explicitly in the workflow and keep the script itself dependency-light.
- **CI logs can become noisy** -> Write per-step logs to artifacts and keep the workflow step list short.
- **The baseline gate still does not cover Raspberry Pi hardware** -> Preserve `Blocked-HW` and `Deferred-RPi` handling in the evidence model rather than pretending CI replaces target validation.

## Migration Plan

1. Create the OpenSpec proposal, design notes, tasks, and spec deltas for `verification-ci-v1`.
2. Add the shared verification gate script under `scripts/`.
3. Add the GitHub Actions workflow under `.github/workflows/`.
4. Add the reusable evidence template and record the initial CI evidence.
5. Update the narrative verification and delivery documents.
6. Run the verification script locally, validate the OpenSpec change, and archive it.

## Open Questions

None that block the baseline CI/evidence slice. Matrix expansion and target-specific runners can wait for later changes if the project needs them.
