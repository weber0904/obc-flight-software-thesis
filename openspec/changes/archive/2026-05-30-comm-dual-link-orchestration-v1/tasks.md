## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and delta specs for
  `ground-ttc-gateway`, `verification-evidence`,
  `verification-path-registry`, and `planning-docs`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate comm-dual-link-orchestration-v1`.

## 2. Hosted Layer-2 Owner

- [x] 2.1 Add a distinct hosted orchestration helper or launcher above the
  maintained per-band stock-stack baseline without changing `43B` semantics.
- [x] 2.2 Make the new owner write reviewable orchestration manifest and status
  artifacts covering owner type, lifecycle phase, owned transitions, owned
  process set, shared-runtime interaction, delegated adjacent state, and
  failure or cleanup summary.
- [x] 2.3 Keep the new owner limited to hosted lifecycle/state coordination and
  explicit non-claims; do not add command-path, session-policy, gateway, or
  COMM semantic ownership.

## 3. Hosted Orchestration Proof

- [x] 3.1 Add a dedicated hosted-first orchestration proof path that uses only
  orchestration-owned artifacts plus process/listener cleanup as its oracle.
- [x] 3.2 Prove the happy-path lifecycle transition sequence:
  `preflight -> starting -> ready -> stopping -> stopped`.
- [x] 3.3 Add one bounded startup-conflict case that yields explicit
  `startup_failed` state and cleanup summary without reusing older COMM
  semantic oracles.
- [x] 3.4 Rerun the layer-1 `43B` proof separately as non-regression rather
  than folding it into the new orchestration claim.

## 4. Evidence, Registry, And Docs

- [x] 4.1 Add a dedicated hosted orchestration evidence record that states what
  the new owner proves, what remains delegated to `43B`, and what stays
  deferred.
- [x] 4.2 Add a distinct hosted orchestration-owner entry to
  `evidence/verification-path-registry.md` that is explicitly separate from `43B`.
- [x] 4.3 Add a dedicated hosted orchestration runbook and update current docs
  and script inventory so they consistently distinguish:
  - layer-1 maintained per-band stock stacks
  - layer-2 hosted thin lifecycle owner
  - deferred future target-bearing simultaneous work

## 5. Verification

- [x] 5.1 Run touched helper syntax checks with `python3 -m py_compile`.
- [x] 5.2 Run touched launcher and probe shell syntax checks with `bash -n`.
- [x] 5.3 Run `bash scripts/run_dual_link_orchestration_hosted_probe.sh`.
- [x] 5.4 Run `bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh`.
- [x] 5.5 Run
  `PATH="$PWD/fprime-venv/bin:$PATH" bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`.
- [x] 5.6 Run `openspec validate comm-dual-link-orchestration-v1` and
  `openspec validate --specs`.
