# Contributing

This repository is a curated public thesis release of a larger development
workspace. Contributions must preserve its distinction between current
behavior, historical evidence, and unproven future scope.

## Workflow

1. Create a dedicated `feature/`, `fix/`, `docs/`, or `hotfix/` branch.
2. Use an OpenSpec change for product, interface, verification, packaging, or
   governance changes.
3. Update code, current documentation, and evidence in the same branch.
4. Run `bash scripts/run_verification_ci.sh`.
5. Run the focused repository-owned hosted probe for the claim being changed.
6. Validate and archive the OpenSpec change before release closeout.
7. Open a ready-for-review pull request and wait for required CI.

## Verification Rules

- Start from `docs/verification-path-registry.md`; do not select a nearby old
  wrapper by filename.
- Target/lab work follows A/B/C ownership:
  `ensure_target_comm_lab_baseline.sh`,
  `ensure_ground_dual_gds_baseline.sh`, then the probe-owned functional test.
- A real component under `OBC/Components/` must retain its classic F Prime
  TesterBase/GTestBase unit-test harness.
- Hosted probes use fresh builds, isolated runtime roots and ports, bounded
  assertions, and managed cleanup.
- New executable scripts must be registered in
  `scripts/verification-manifest.json`.

## Documentation And Evidence

- English current documents are canonical.
- Traditional Chinese is retained for the thesis claim map and demo workflow.
- Preserve exact environment and non-claim boundaries.
- Keep test-record summaries in Git; raw artifacts belong in the checksummed
  release evidence asset.
- Never commit `config/security/command-auth.ini` or another real credential.
