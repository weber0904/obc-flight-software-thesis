## Context

The main boot/update specification already fixes the high-level contract: updates arrive through a staging file, metadata is file-backed, the inactive slot becomes pending, confirmation defaults to 60 seconds, and rollback is required when confirmation or health conditions fail. The repository, however, has no implementation for that contract yet, and no executable artifact that proves the file-backed metadata path or the confirm/rollback logic.

This change turns that contract into a first hosted implementation slice. It intentionally stops short of Raspberry Pi bootloader wiring, but it does need to make the public `BOOT_*` surface concrete and preserve enough state on disk that later target-specific work can build on it rather than redefining it.

## Goals / Non-Goals

**Goals:**

- Implement `BootManager` with the owned `BOOT_*` command, telemetry, and event families.
- Persist boot metadata v1 as a structured file under persistent storage.
- Verify staged images using the project hash backend and recorded expected image size.
- Simulate the pending-slot boot window closely enough to exercise confirm and timeout rollback behavior on the host.
- Record reviewable build and verification evidence for the slice.

**Non-Goals:**

- Do not implement Raspberry Pi bootloader, partition, or firmware integration.
- Do not add chunk-upload commands or any command-payload image transfer path.
- Do not introduce secure-boot or hardware-root-of-trust features in this change.
- Do not replace the future real reboot path; this slice models the boot transition in-process for host validation.

## Decisions

### Decision: Keep metadata as a single structured text file

Boot metadata v1 will live under the persistent-data tree as a single key/value text file. The file records the formal minimum fields and a few implementation-support fields needed by the hosted state machine:

- `active_slot`
- `pending_slot`
- `last_known_good_slot`
- `confirmed`
- `expected_digest`
- `expected_size`
- `last_boot_attempt_time`
- `last_error_code`
- `stage_verified`
- `staged_path`

Alternative considered:

- Store metadata in JSON or SQLite. Rejected because the baseline explicitly avoids a database and the hosted slice does not need a heavier parser.

### Decision: Reuse the active F' hash backend instead of hard-coding SHA-256

The first implementation will verify staged files using `Utils::Hash`, which currently resolves to the project-configured F' hash backend. In this repository today that backend is CRC32, rendered as lowercase hex for the operator-facing digest string.

Alternative considered:

- Add a separate OpenSSL or custom SHA-256 path in the project code. Rejected because it would diverge from the active F' hash configuration and add an unnecessary dependency before the platform slice needs it.

### Decision: Model the pending-slot boot window in-process

`BOOT_ACTIVATE_STAGED_IMAGE` will switch the modeled active slot to the inactive slot, mark that slot as pending, and start the confirm timeout window. `BOOT_CONFIRM` finalizes the new slot as last-known-good, while scheduler ticks without confirmation trigger rollback to the previous good slot.

Alternative considered:

- Keep activation as metadata-only and wait for a true reboot implementation. Rejected because it would leave the formal confirm/rollback behavior untestable in the current host-only project stage.

### Decision: Treat invalid metadata as an immediate rollback condition

When persisted metadata cannot be parsed or is missing required fields, `BootManager` will restore a safe in-memory baseline rooted at the last-known-good slot, emit the rollback path, and rewrite metadata in the supported format.

Alternative considered:

- Fail commands until the operator repairs metadata manually. Rejected because the main spec already treats invalid metadata as a rollback condition, not a dead-end fault state.

## Risks / Trade-offs

- **Hosted activation is not a real reboot** -> Document it explicitly as the first hosted execution model and leave true target reboot sequencing to `Deferred-RPi`.
- **The active hash backend is CRC32 today** -> Record this decision in both the narrative source and spec delta so reviewers do not assume SHA-256 semantics.
- **File-backed metadata adds host filesystem dependence to UTs** -> Keep test storage under temporary directories and clean up in the test harness destructor.
- **Real bootloader behavior remains uncovered** -> Mark Raspberry Pi boot-chain validation as `Deferred-RPi` and power-loss/SD-card switching as `Blocked-HW`.

## Migration Plan

1. Create the OpenSpec proposal, design, tasks, and delta specs for `boot-update-v1`.
2. Add `BootManager` and its file-backed metadata helper under `OBC/Components/BootManager/`.
3. Add focused F' unit tests covering the hosted lifecycle and metadata recovery paths.
4. Update the narrative boot/update source document with the first implementation details.
5. Run the normal build, UT build, and `fprime-util check --all`.
6. Record evidence under `docs/test-records/boot-update-v1/`.
7. Validate and archive the change.

## Open Questions

None that block this first hosted slice. True reboot integration and Raspberry Pi boot-chain ownership remain queued for later target-specific changes.
