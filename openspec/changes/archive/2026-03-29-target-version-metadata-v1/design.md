## Context

The Raspberry Pi workflow intentionally syncs a governed workspace without `.git` so the target build tree stays lightweight and does not depend on a full repository clone. That keeps the target flow simple, but it also means the framework's stock version generator cannot run `git describe` inside the target workspace and therefore falls back to `v3.5.0`.

The project needs correct target-side version metadata for reviewable evidence and operator confidence, but it should not solve this by patching the `lib/fprime` submodule in place or by copying the full git history to the Raspberry Pi. The fix should stay repo-local, work through the governed sync/bootstrap scripts, and preserve the hosted workflow.

## Goals / Non-Goals

**Goals:**

- preserve correct framework and project version strings in the Raspberry Pi target build without syncing `.git`
- keep the solution repo-local so the project does not modify the checked-in F' submodule implementation
- make the governed Raspberry Pi bootstrap path the single supported way to inject target version metadata
- record reviewable evidence showing the target-generated version metadata after the fix

**Non-Goals:**

- designing a full target packaging or installation bundle
- introducing signature-based authenticity or changing the existing CRC-32 vs SHA-256 split
- teaching arbitrary manual Pi builds outside the governed bootstrap flow to reconstruct version metadata from scratch

## Decisions

### 1. Override the F' version target at the project layer

The project will prepend a repo-local `cmake/` directory before loading `FPrime.cmake` and provide a local `target/version.cmake` override. That lets the project redirect version generation behavior without editing `lib/fprime` and keeps the customization reviewable in the governed repository.

Alternatives considered:

- Patch `lib/fprime/cmake/target/version.cmake` directly: rejected because it dirties the submodule and makes the local policy harder to carry or review.
- Accept the fallback string on Pi: rejected because it produces misleading evidence and operator output.

### 2. Use explicit host-derived overrides for framework and project versions

The Raspberry Pi bootstrap script will capture the host project's `git describe` results before syncing/building and export those values into the target build environment. The repo-local version generator will prefer these explicit override values and fall back to the stock F' git lookup only when overrides are absent.

Alternatives considered:

- Sync `.git` to the Raspberry Pi: rejected because the current target workflow intentionally avoids pushing the full repository metadata.
- Generate and commit a permanent version seed file in the repository: rejected because it risks stale checked-in metadata and would blur generated state with source-controlled content.

### 3. Preserve hosted behavior by keeping fallback order narrow

Only the Raspberry Pi bootstrap path will inject override variables. Hosted builds in the normal local repository will continue to obtain versions from git as they do today, and the repo-local generator will call back into the framework logic when overrides are not provided.

Alternatives considered:

- Force all builds to use overrides once the project defines them: rejected because hosted builds already have direct git context and should remain simple.

## Risks / Trade-offs

- [Project-level target override can drift from upstream F'] -> Keep the override thin and delegate as much file-generation logic as possible back to the framework generator.
- [Host-derived metadata can become stale if the target is rebuilt later without rerunning bootstrap] -> Treat the governed bootstrap script as the supported target build entrypoint and document that expectation.
- [Version evidence could still be ambiguous] -> Record both the build command path and the observed target output in a dedicated evidence record.

## Migration Plan

1. Add the repo-local version-target override and the bootstrap-time host version exports.
2. Verify hosted generation still works unchanged.
3. Rebuild the synced workspace on the Raspberry Pi through the governed bootstrap script.
4. Capture the generated target version metadata and update evidence/docs.
5. Validate and archive the OpenSpec change.

## Open Questions

- Whether a later packaging change should persist the same host-derived version strings into a deployable manifest outside the build tree.
