## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and a `verification-evidence` delta spec for `active-probe-cleanup-hardening-v1`.
- [x] 1.2 Validate the change artifacts before implementation with `openspec validate active-probe-cleanup-hardening-v1`.

## 2. Python Probe Cleanup Hardening

- [x] 2.1 Add a repo-internal Python helper under `scripts/` for managed subprocess launch, reverse-order process-group teardown, and narrowly-scoped stale-helper reaping.
- [x] 2.2 Refactor the active command envelope/auth/session/freshness probes to use the helper for local `fprime-gds`, stack-wrapper, and auxiliary subprocess management.
- [x] 2.3 Refactor the active Raspberry Pi command-persistence probe and the selected CCSDS hosted probes in scope to use the same helper contract.

## 3. Shell Launcher Cleanup Hardening

- [x] 3.1 Add targeted owned-process sweep helpers to `scripts/_common.sh` that operate on exact command fragments, owned ports, and runtime-root paths.
- [x] 3.2 Harden `scripts/run_dev_stack.sh` and `scripts/run_remote_csp_gds_stack.sh` so interrupted reruns self-heal without manual cleanup.
- [x] 3.3 Harden `scripts/run_rpi_stack.sh` and `scripts/run_rpi_installed_stack.sh` so the remote active stack path receives and uses the cleanup ownership tokens.

## 4. Verification And Evidence

- [x] 4.1 Add a repository-owned cleanup-hardening verification script that interrupts representative hosted, shell, and Raspberry Pi probe flows, verifies rerun success, and confirms no owned helper leftovers remain.
- [x] 4.2 Rerun the affected active-path hosted and Raspberry Pi probes as regressions after the helper refactor.
- [x] 4.3 Add `docs/test-records/active-probe-cleanup-hardening-v1/README.md` with the final commands, interruption scenarios, rerun results, and bounded claims.

## 5. Closeout

- [x] 5.1 Run the fresh local verification gate and focused cleanup-hardening probes or checks required for touched scripts.
- [x] 5.2 Run `openspec validate active-probe-cleanup-hardening-v1` and `openspec validate --specs`.
- [x] 5.3 Archive the OpenSpec change before opening the PR.
