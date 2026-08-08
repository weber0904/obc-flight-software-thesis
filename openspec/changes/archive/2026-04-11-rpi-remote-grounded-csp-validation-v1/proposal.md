## Why

The repository already proves hosted-local CSP, Pi-local CSP, and distinct ground-side GDS paths, but it does not yet prove the topology that originally motivated the internal-network design: `Pi OBC` talking to remote macOS-hosted EPS and ADCS simulators while the same macOS host also provides the ground-side GDS command path. The next governed slice should register that remote topology explicitly instead of treating it as an informal combination of existing paths.

## What Changes

- Add a repo-owned host-side stack launcher for `csp_zmqproxy`, remote EPS/ADCS simulators, and headless GDS on macOS.
- Add a repo-owned Pi-side launcher that starts only the OBC process against remote CSP and GDS endpoints.
- Add a bounded probe that proves two distinct paths in one governed flow:
  - Pi OBC to remote macOS EPS/ADCS simulator CSP reachability
  - `fprime-cli -> GDS -> Pi OBC -> remote EPS/ADCS simulator` command dispatch
- Update specs, registry, README, and evidence wording so the remote CSP path, GDS command path, external comm path, and future hardware-bus claims remain distinct.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: add the governed remote `Pi OBC + macOS simulators + macOS GDS` topology as an allowed validation configuration.
- `verification-evidence`: record remote internal CSP reachability separately from the ground-driven subsystem command path.
- `verification-path-registry`: register the new remote internal CSP path and the new target-side GDS-driven subsystem command path.

## Impact

- Affected scripts: new host-side stack launcher, Pi-side remote launcher, and combined probe.
- Affected docs/evidence: README, verification matrix, verification-path registry, and new test-record documentation.
- Affected runtime behavior: none intended for existing hosted-local or Pi-local flows; the change adds a new governed topology and probe.
