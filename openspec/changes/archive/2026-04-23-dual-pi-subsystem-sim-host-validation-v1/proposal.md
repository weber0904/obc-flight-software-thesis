## Why

The repository now has named `obc.local` and `subsystem.local` roles, but the governed remote CSP/GDS workflow still assumes a two-host layout where macOS runs both the ground services and the subsystem simulators. The next slice needs to prove the actual near-term three-host topology so hostname changes, split-host subsystem simulation, and target-side GDS command flow become reviewable baselines instead of ad hoc operator combinations.

## What Changes

- Add a governed three-host topology where `macOS` runs `csp_zmqproxy` plus headless `fprime-gds`, `subsystem.local` runs `eps_simulator` and `adcs_simulator`, and `obc.local` runs only the OBC process.
- Add repo-owned subsystem-host workspace sync/bootstrap scripts plus launchers that start the subsystem simulator stack over SSH without introducing a subsystem install/autostart lifecycle.
- Add two bounded probes:
  - a main dual-Pi split-host probe for internal CSP reachability plus `fprime-cli -> GDS -> obc.local -> subsystem.local` EPS and ADCS commands
  - a separate dual-Pi coexistence probe that proves the split-host subsystem path can coexist with external comm on `/dev/serial0`
- Update specs, verification-path registry, operator docs, and evidence so the new three-host path stays distinct from the older two-host remote path, direct GDS connectivity, and future physical-link claims.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: add the governed three-host `macOS + obc.local + subsystem.local` topology and subsystem-host workspace orchestration surface.
- `comm-subsystem`: add a governed split-host coexistence path where external comm on `obc.local` remains separate from the remote subsystem simulator path.
- `verification-evidence`: record the three-host internal CSP path, the three-host GDS-driven subsystem command path, and the separate comm coexistence path as reviewable evidence.
- `verification-path-registry`: register the three-host internal CSP path, the three-host GDS-driven subsystem command path, and the split-host CSP plus external-comm coexistence path.

## Impact

- Affected scripts: new subsystem-host sync/bootstrap/launcher scripts, new ground-host launcher, new dual-Pi probes, and shared shell helper logic.
- Affected docs/evidence: README surfaces, verification matrix/registry, new evidence docs, and reconciliation inventory.
- Affected runtime behavior: none for existing hosted-local, single-Pi, or two-host remote baselines; this change adds a new governed topology and new bounded probes.
