## Why

During Raspberry Pi target verification, the boot metadata file was observed to retain stale trailing bytes after a rollback rewrote `staged_path` from a populated value to an empty value. The runtime behavior still reached the correct slot state, but the persisted `metadata-v1.txt` file became malformed because the shorter rewrite did not truncate the previous contents.

This needs a small governed bugfix slice so the file-backed boot contract remains reviewable and reloadable after rollback, especially on target hardware where the metadata file is part of the observable A/B state.

## What Changes

- Make boot metadata rewrites truncate-safe so a shorter persisted value cannot leave stale bytes in `metadata-v1.txt`.
- Add a regression test that exercises rollback persistence and reloads the metadata file from disk to prove the rewritten file stays valid.
- Record the observed bug and fix in the Raspberry Pi target evidence.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `boot-update`: the file-backed metadata contract now explicitly requires rewrites to replace the previous serialized contents cleanly instead of leaving stale trailing bytes.

## Impact

- Affected code: `OBC/Components/BootManager/`
- Affected docs: `evidence/records/rpi-target-integration-v1/`
- Affected systems: hosted unit-test environment and Raspberry Pi target boot metadata persistence
