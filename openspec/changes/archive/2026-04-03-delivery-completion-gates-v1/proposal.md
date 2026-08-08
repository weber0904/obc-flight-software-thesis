## Why

The current delivery documents describe Git, CI, and OpenSpec at a high level, but they do not yet record the project's refined completion checkpoints clearly enough. That gap has already caused confusion about when a change is merely local-ready, when it may be pushed, and when it is formally complete after CI.

## What Changes

- Define a project-owned `local-ready` checkpoint that requires implementation, local verification, archive, clean worktree, and a Conventional Commit before a change is reported as ready to push.
- Define the push and CI completion gates explicitly so a change is not treated as formally complete until GitHub CI is green after push.
- Define the timing rule for releases and tags so they are created only after the relevant CI run has succeeded.
- Update the narrative delivery workflow document and the README summary so the documented operator workflow matches the repo's actual delivery practice.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `delivery-workflow`: add explicit local-ready, push-confirmation, CI-green, and release/tag completion gates.

## Impact

- Affected code: none.
- Affected systems: formal delivery workflow, change completion criteria, and operator-facing project guidance.
- No change to flight software runtime behavior, target integration behavior, or verification tooling semantics.
