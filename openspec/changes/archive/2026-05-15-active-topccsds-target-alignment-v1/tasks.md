## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `active-topccsds-target-alignment-v1`.
- [x] 1.2 Validate the change artifacts before implementation with `openspec validate active-topccsds-target-alignment-v1`.

## 2. Active OBC Packaging And Launch Alignment

- [x] 2.1 Update the governed Raspberry Pi bundle flow to package the active `build-artifacts/.../OBC` binary and dictionary rather than the legacy deployment.
- [x] 2.2 Keep the installed release layout and launch templates on `bin/OBC`, but make that path truthfully resolve to the active `TopCcsds` deployment.
- [x] 2.3 Update source-workspace Raspberry Pi launch helpers needed by follow-up 02 so they default to the active `OBC` binary instead of silently defaulting to `OBC_ComFprimeLegacy`.
- [x] 2.4 Keep legacy-only wrappers explicit and avoid adding any new legacy dependency to the active package/install/probe path.

## 3. Evidence And Docs

- [x] 3.1 Refresh Raspberry Pi package evidence to show the active package manifest, active binary payload, and active dictionary contents.
- [x] 3.2 Refresh installed-release smoke evidence on the corrected package path.
- [x] 3.3 Refresh autostart or reboot evidence on the corrected package path.
- [x] 3.4 Update active-baseline docs where target-facing source-of-truth wording changes.

## 4. Verification And Closeout

- [x] 4.1 Run the fresh local verification gate and any focused checks needed for touched package/helper logic.
- [x] 4.2 Run `openspec validate active-topccsds-target-alignment-v1` and `openspec validate --specs`.
- [x] 4.3 Keep the worktree review-ready without retiring legacy in this wave.
