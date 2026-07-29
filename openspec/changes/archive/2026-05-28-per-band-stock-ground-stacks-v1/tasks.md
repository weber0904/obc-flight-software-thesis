## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and delta specs for
  `ground-ttc-gateway`, `verification-evidence`, and
  `verification-path-registry`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate per-band-stock-ground-stacks-v1`.

## 2. Maintained Hosted Launcher Surface

- [x] 2.1 Extract the shared hosted dual-surface process-management logic from
  `run_comm_session_and_downlink_qos_probe.sh` into a dedicated maintained
  helper that can start one shared hosted runtime with distinct S-band and UHF
  stock stacks.
- [x] 2.2 Add maintained launcher entrypoints for the hosted S-band stock stack,
  hosted UHF stock stack, and the combined composition-only hosted wrapper.
- [x] 2.3 Make the maintained launcher output reviewable by printing stack
  roles, GDS and TTS ports, southbound endpoints, file-storage directories,
  runtime roots, process logs, startup order, cleanup order, and explicit
  non-claims.

## 3. Hosted Proof And Reused Baselines

- [x] 3.1 Add one hosted-first repository-owned proof for the maintained
  per-band stock-stack operator baseline.
- [x] 3.2 Keep the existing hosted probe-owned surfaces as proof dependencies,
  not as the new operator surface, and update them only as needed to reuse the
  shared helper cleanly.
- [x] 3.3 Record hosted-first evidence that states what is newly proven, what
  per-band S-band and UHF paths are reused, and what remains deferred to
  orchestration and target-bearing simultaneous work.

## 4. Docs, Runbook, And Registry

- [x] 4.1 Add a dedicated hosted operator runbook under `docs/operator/` for
  the maintained per-band stock-stack baseline.
- [x] 4.2 Update current-facing docs and inventory surfaces so they reflect the
  2026-05-28 UHF baseline and the maintained separate per-band stock-stack
  operator truth.
- [x] 4.3 Add a dedicated hosted per-band stock-stack operator-baseline entry
  to `docs/verification-path-registry.md` and keep future
  `comm-dual-link-orchestration-v1` wording clearly deferred.

## 5. Verification

- [x] 5.1 Run touched script syntax and helper checks for the maintained
  launcher and proof surfaces.
- [x] 5.2 Run the maintained launcher proof and rerun the adjacent hosted S-band
  and UHF dependency proofs affected by the helper extraction.
- [x] 5.3 Run `openspec validate per-band-stock-ground-stacks-v1` and
  `openspec validate --specs`.
