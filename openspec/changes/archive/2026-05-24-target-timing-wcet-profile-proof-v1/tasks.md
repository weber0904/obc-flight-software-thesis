## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and delta specs for
  `interface-contract-index`, `platform-baseline`, and
  `verification-evidence`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate target-timing-wcet-profile-proof-v1`.

## 2. Target Timing Proof Plumbing

- [x] 2.1 Package `payload_camera_backend_helper` into the Raspberry Pi release
  surface and manifest so the installed `obc-comm-csp-stack.service` path can
  execute the representative payload workload.
- [x] 2.2 Add a repository-owned
  `scripts/run_target_timing_wcet_profile_proof_v1_probe.sh` entrypoint plus
  any bounded helper script needed to measure the service-managed target path.
- [x] 2.3 Keep the probe rerun-safe and scoped to the canonical
  service-managed target baseline instead of widening into direct-control or
  broader COMM-matrix revalidation.

## 3. Documentation And Evidence

- [x] 3.1 Add `evidence/records/target-timing-wcet-profile-proof-v1/README.md`
  and record the declared workload windows, timing observations, slip verdict,
  and residual gaps.
- [x] 3.2 Update `evidence/verification-path-registry.md` with the new target
  timing proof path.
- [x] 3.3 Refresh stale current-baseline docs so they reflect payload v2 /
  helper-backed target payload convergence and the current COMM verification
  matrix closure, and so target timing no longer stays as broad `TBD` prose.
- [x] 3.4 Update `docs/interfaces.md` so target-flightlike timing surfaces show
  proven facts plus explicit residual gaps instead of all-`TBD` rows.

## 4. Verification And Closeout

- [x] 4.1 Run the fresh local verification/build work needed for this change,
  including maintained targets touched by the packaging/probe path.
- [x] 4.2 Repackage and reinstall the Raspberry Pi release, rerun the relevant
  service-managed target baseline proof, and either capture passing timing/WCET
  evidence or record the narrower residual-gap evidence honestly on the
  canonical target path.
- [x] 4.3 Run `openspec validate target-timing-wcet-profile-proof-v1` and
  `openspec validate --specs`.
- [x] 4.4 Sync specs, archive the change, update the reconciliation matrix, and
  keep the branch reviewable for PR creation.
