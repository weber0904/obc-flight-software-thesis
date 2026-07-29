## 1. OpenSpec Artifacts And Qualification Inventory

- [x] 1.1 Write `proposal.md`, `design.md`, delta specs, and `tasks.md` for `legacy-command-envelope-retirement-v2`.
- [x] 1.2 Re-qualify candidate verification scripts into `current maintained secure-baseline gates`, `supplemental / historical wrappers`, and `retired-now surfaces` using current registry/runbook authority before implementation claims rely on them.
- [x] 1.3 Validate the change artifacts with `openspec validate legacy-command-envelope-retirement-v2`.

## 2. Runtime Retirement

- [x] 2.1 Remove public `OBCApp.commandIngressAuthority.SESSION_OPEN` from current command surfaces, authority catalog, policy JSON, and dictionary-facing runtime contract.
- [x] 2.2 Update `CommandIngressAuthority` so `authGranted` still synthesizes active secure session state and emits existing `COMMAND_SESSION_OPENED` / `SESSION_*` evidence.
- [x] 2.3 Make legacy command-envelope v1 and external lifecycle traffic fail closed before dispatch, session mutation, sequence mutation, or persistence mutation.
- [x] 2.4 Remove current runtime dependence on legacy reopen-floor / persistent freshness state where it only served the retired external v1 path.

## 3. Tests And Focused Code Verification

- [x] 3.1 Rewrite `CommandIngressAuthority` unit coverage to secure-auth-only current behavior and legacy fail-closed expectations.
- [x] 3.2 Rebuild and rerun affected focused unit targets for `CommandIngressAuthority` and adjacent secure-session owners after the retirement changes.
- [x] 3.3 Confirm current secure-baseline operator/runtime surfaces still work without public `SESSION_OPEN`.

## 4. Specs, Registry, And Current Docs

- [x] 4.1 Update current main specs so secure-auth-only is the maintained command baseline and legacy `SESSION_OPEN` is historical compatibility only.
- [x] 4.2 Update `docs/verification-path-registry.md`, roadmap docs, and relevant operator runbooks so retired wrappers no longer appear as maintained closeout authority.
- [x] 4.3 Keep archived evidence intact while clearly marking legacy v1 and timing families as historical or retired in current docs.

## 5. Local-Ready Verification And Closeout

- [x] 5.1 Run the fresh local gate `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`.
- [x] 5.2 Run only the re-qualified maintained secure-baseline focused probes needed for this change.
- [x] 5.3 Update test-record evidence for the qualification audit, runtime retirement, and branch-head verification results.
- [x] 5.4 Re-run `openspec validate legacy-command-envelope-retirement-v2` and `openspec validate --specs`.
- [x] 5.5 Sync specs, archive the change, update `docs/baseline-reconciliation-matrix.json`, regenerate `docs/baseline-reconciliation-matrix.md`, run `python3 scripts/check_repo_consistency.py`, and confirm the worktree is clean before declaring `local-ready`.
