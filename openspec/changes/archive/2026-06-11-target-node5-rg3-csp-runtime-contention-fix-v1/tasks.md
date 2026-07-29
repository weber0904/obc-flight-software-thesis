## 1. Salvage The Bounded Product Fix

- [x] 1.1 Port the `LibCspRuntime` cached-metrics fix onto a clean branch
  without carrying the dirty vendored F' timing patch.
- [x] 1.2 Port the `CommController` cached runtime-state fix onto the same
  clean branch without carrying temporary cadence or diagnostics knobs.

## 2. Re-verify On A Clean Baseline

- [x] 2.1 Run fresh local generate/build and focused verification for the
  salvaged code path on the clean branch.
- [x] 2.2 Rerun the maintained target node-`5` secure-auth path and confirm the
  reproduced RG3 slip is gone on the clean branch, while keeping any RG1
  residual separate.

## 3. Close The Formal Workflow

- [x] 3.1 Rewrite the evidence record and OpenSpec artifacts so they describe
  only the salvaged mainline-worthy fix and its clean-branch verification.
- [x] 3.2 Validate and archive
  `target-node5-rg3-csp-runtime-contention-fix-v1` before starting the
  follow-up runtime-owner architecture change.
