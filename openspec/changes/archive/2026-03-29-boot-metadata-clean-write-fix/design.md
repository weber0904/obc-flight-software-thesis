## Context

The `rpi-target-integration-v1` change validated that boot metadata persists across target-side process restarts, but the target evidence exposed a serialization bug: after rollback cleared `staged_path`, the persisted metadata file still contained leftover bytes from the previous longer line. A fresh reader could treat that file as malformed, which violates the intent of the file-backed metadata contract even though the in-memory state machine behaved correctly.

## Goals / Non-Goals

**Goals:**

- ensure each metadata save fully replaces the previous file contents
- add a regression test that fails if rollback leaves a malformed metadata file on disk
- keep the fix localized to the boot metadata persistence path

**Non-Goals:**

- changing the public boot/update commands, telemetry, or event contract
- changing the SHA-256 staged-image verification behavior
- expanding the Raspberry Pi flow beyond the already completed target evidence

## Decisions

### 1. Use truncate-safe file creation for metadata rewrites

`BootMetadataStore::writeFile_()` will open the metadata file in a mode that recreates or truncates the existing file before writing. This is the smallest fix that directly addresses the observed stale-tail behavior.

Alternatives considered:

- delete the file before each write: rejected because it adds an unnecessary extra filesystem step and a brief missing-file window
- pad rewritten metadata to the previous length: rejected because it would preserve an implementation quirk instead of fixing the underlying persistence behavior

### 2. Lock the bug with a disk-backed rollback regression test

The regression test will drive the existing prepare / verify / activate / rollback flow, then create a fresh `BootMetadataStore` reader and confirm that the on-disk file parses cleanly with an empty `stagedPath`.

Alternatives considered:

- assert only the in-memory rollback state: rejected because that would miss the original on-disk corruption
- rely only on the Raspberry Pi probe script: rejected because the fix should be covered by local automated tests as well

## Risks / Trade-offs

- [Filesystem mode differences] -> use F' file APIs rather than host-specific POSIX calls so the fix stays portable across hosted and target builds
- [Regression test could accidentally prove cached state instead of disk state] -> construct a fresh metadata store after rollback and read the file back from disk

## Migration Plan

1. Update the metadata write path to truncate or recreate the file on each save.
2. Add the rollback persistence regression test.
3. Re-run local unit/integration tests and the Raspberry Pi boot probe.
4. Append the observation to the target evidence, validate the change, and archive it.
