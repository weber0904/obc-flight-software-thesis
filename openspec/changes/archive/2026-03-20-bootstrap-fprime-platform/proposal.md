## Why

The formal spec baseline now exists, but the repository still does not contain an actual F' project tree. This change turns the spec-only workspace into the first real implementation workspace by populating it with the F' v4.1.0 project skeleton while preserving the OpenSpec and narrative-doc layers that now govern future work.

## What Changes

- Populate the current repository in place using `fprime-bootstrap project --populate --path . --tag v4.1.0`.
- Preserve the existing `openspec/`, `obc-dev-spec/`, and `.codex/skills/` structures while adding the F' project files needed for implementation.
- Establish the baseline project tree, virtual environment, and settings files that future subsystem changes will build on.
- Add any missing project-local directories or placeholders required by the formal baseline when the bootstrap output does not create them directly.
- Run the bootstrap-phase gate checks for `fprime-util generate` and `fprime-util build`.
- Capture bootstrap verification evidence inside the repo so later changes can reference a concrete starting point.

## Capabilities

### New Capabilities

None.

### Modified Capabilities
- `platform-baseline`: Add requirements for in-place F' population of the existing OpenSpec-governed workspace and for the minimum generated project structure.
- `verification-evidence`: Add bootstrap-specific gate requirements for `fprime-util generate` and `fprime-util build`, plus evidence capture for this phase.

## Impact

- Affected files: project root bootstrap outputs, `openspec/changes/bootstrap-fprime-platform/`, `.gitignore`, `docs/`, and any generated F' baseline files
- Affected systems: local Python virtual environment, F' project skeleton, future topology and component work
- Dependencies: `fprime-bootstrap`, `fprime-util` inside the generated virtual environment, network/package access if bootstrap needs remote content
