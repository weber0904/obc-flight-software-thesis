## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `ground-ttc-gateway`, `comm-subsystem`, `interface-contract-index`, and `documentation-governance`.
- [x] 1.2 Validate the change artifacts before closeout with `openspec validate manual-dual-gds-secure-ops-surface-v1`.

## 2. Manual Operator Surface

- [x] 2.1 Create the dedicated `scripts/manual_ops/` subtree with hosted, target, and shared helper structure.
- [x] 2.2 Add the hosted manual dual-GDS lifecycle wrappers and unified manifest/status output.
- [x] 2.3 Add the target manual baseline wrappers and target local ground wrappers with explicit ownership boundaries.

## 3. Shared Helper And Launcher Plumbing

- [x] 3.1 Add `manual_secure_ops.py` plus shared library code for manifest loading, session-state persistence, secure auth, secure-v2 command send, and governed sequence/file actions.
- [x] 3.2 Extend the maintained GDS launchers so manual surfaces support `GDS_UI_MODE=ui|headless` without changing existing headless proof defaults.
- [x] 3.3 Reuse tracked secure keystore material directly for hosted auth and add the target provenance gate before target auth establishment.

## 4. Documentation

- [x] 4.1 Add hosted and target manual dual-GDS operator runbooks with manifest, auth, secure command, file upload, `SEQ_*`, re-auth, and cleanup guidance.
- [x] 4.2 Update `README.md`, `scripts/README.md`, and other current entrypoints to route readers toward the new manual operator subtree.
- [x] 4.3 Update `docs/interfaces.md` as needed to reflect the manual manifest/session boundary and explicit invalidation semantics.

## 5. Validation

- [x] 5.1 Run `bash -n` for the new shell wrappers and `python -m py_compile` for the new manual helper modules.
- [x] 5.2 Run `openspec validate manual-dual-gds-secure-ops-surface-v1` and `openspec validate --specs`.
- [x] 5.3 Update this task list with the completed results.
