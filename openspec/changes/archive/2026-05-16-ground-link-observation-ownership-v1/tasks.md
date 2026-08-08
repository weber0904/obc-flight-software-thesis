## 1. Formalize the change

- [x] 1.1 Add proposal, design, tasks, and `comm-subsystem` delta artifacts for `ground-link-observation-ownership-v1`.
- [x] 1.2 Run `openspec validate ground-link-observation-ownership-v1`.

## 2. Move observation ownership

- [x] 2.1 Add the OBC-owned ground-link observation runtime header.
- [x] 2.2 Move provider-facing observation types out of `simulators/comm` and update includes.
- [x] 2.3 Remove `cspTargetNode` from the provider-facing observation contract.

## 3. Express health semantics explicitly

- [x] 3.1 Publish `ACTIVE_COMM_CSP` for node `5` and node `6`, `CONNECTED_ONLY_FALLBACK` for node `4` and direct TCP, and `DISABLED` for no backend.
- [x] 3.2 Update `GroundLinkHealthProvider` to use `GroundLinkHealthSemantics` instead of raw node IDs.
- [x] 3.3 Preserve active COMM CSP stale/activity and transport-growth behavior while suppressing stale/transport-growth policy for connected-only fallback.

## 4. Remove controller driver coupling

- [x] 4.1 Remove `GroundLinkDriver*` parameters and members from `CommController`.
- [x] 4.2 Update active `TopCcsds`, legacy `Top`, and unit-test wiring for the new controller runtime configuration.

## 5. Refresh tests

- [x] 5.1 Update `GroundLinkDriver` unit coverage for mode, semantics, and observation counters.
- [x] 5.2 Update `GroundLinkHealthProvider` coverage for active COMM CSP, node `4` connected-only fallback, direct TCP fallback, stale recovery, and transport growth.
- [x] 5.3 Update `CommController` coverage to prove FDIR/failover/authority logic remains provider-driven after driver pointer removal.
- [x] 5.4 Update simulator/backend coverage for node `4` versus node `5` / node `6` health semantics.

## 6. Refresh docs and verify

- [x] 6.1 Update `docs/interfaces.md`, current architecture, and relevant roadmap wording.
- [x] 6.2 Run fresh local verification: F' generate/build, UT generate/build, `fprime-util check --all`, component baseline check, and OpenSpec validation.
- [x] 6.3 Run focused probes for active S-band CCSDS primary, active UHF node `6`, and generic node `4` compatibility after the fresh build.
- [x] 6.4 If a focused probe script is touched, keep the cleanup change scoped and verify rerun safety plus no owned-helper orphan or high-CPU residual process.
