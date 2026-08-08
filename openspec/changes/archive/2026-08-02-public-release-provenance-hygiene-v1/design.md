## Context

The release records a fixed F Prime Gitlink so reviewers can reproduce the
project with `git clone --recurse-submodules`. The audit established that the
selected F Prime commit contains only Apache-2.0 modification notices but is
not reachable from the configured public fork. Route 1 provenance also carries
an obsolete allowlist for a local thesis drafting directory.

## Goals / Non-Goals

**Goals:**

- Make the F Prime Gitlink reachable through the configured public remote.
- Remove the obsolete workspace exception without weakening campaign identity
  checks or target synchronization.
- Add a release-level assertion that a clean recursive checkout resolves the
  declared submodules.

**Non-Goals:**

- Change F Prime runtime behavior or its existing project integration changes.
- Re-run target/laboratory hardware validation.
- Alter the runtime-created `.adm-*` and `.stg-*` aliases.

## Decisions

- Publish the existing F Prime notice-only commit to the configured fork rather
  than repinning the project to its parent. The parent is publicly available,
  but lacks notices needed for the already-modified F Prime files.
- Remove the thesis-workspace exception at every public call site and its test.
  The public repository no longer carries that directory, so silently allowing
  it would weaken provenance hygiene without supporting a release workflow.
- Treat a recursive clean clone as the release reachability oracle. Local
  object presence is insufficient because a reviewer must obtain every pinned
  submodule from public remotes.

## Risks / Trade-offs

- [Fork branch is not public or is changed later] → publish the exact commit on
  a named public branch and verify a fresh recursive clone before tag approval.
- [Removing an allowlist rejects a local drafting directory] → intentional;
  keep private drafting outside the public release worktree.
- [Network access is unavailable during normal static checks] → keep the
  recursive clone as a release gate, not an unconditional offline checker.

## Migration Plan

1. Publish the existing F Prime commit to the configured public fork.
2. Remove obsolete public exceptions and update the regression test.
3. Run the focused provenance test, public checks, fresh recursive clone, and
   affected hosted Route 1 verification.
4. Archive this change, regenerate release manifests/checksums if tracked
   content changes, and create a local review commit. Do not tag or push the
   public release branch until separately approved.
