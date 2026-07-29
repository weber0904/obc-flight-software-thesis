# Change Proposal: deployment-runtime-v1

## Why

The initial capability queue built the project slices required by the first-version architecture, but it stopped short of the user-facing acceptance bar defined in the platform baseline: a runnable OBC Flight Software project. The repository already contains hosted EPS / ADCS simulators, comm transports, bridges, boot/update behavior, and the verification gate, but it still needs an integrated runtime path that an operator can launch and exercise end to end.

## What Changes

- Add a minimal hosted OBC deployment/runtime executable that instantiates the existing F' components and can run as the software-only `dev-macos` profile.
- Add a repo-local dev-stack launch path that starts the OBC runtime together with EPS simulator, ADCS simulator, and the external comm mock.
- Add a standalone hosted radio mock server so the external comm path can be exercised through the same `RadioController` / `UartDriver` abstractions used by the OBC runtime.
- Capture integrated runtime verification evidence and update the narrative + formal specs to reflect that the first-version repository now includes a runnable hosted stack, not only isolated capability slices.

## Impact

- Affected specs: `platform-baseline`, `comm-subsystem`, `delivery-workflow`, `verification-evidence`
- Affected code: `OBC/Top/`, `OBC/Main.cpp`, component runtime helpers, `simulators/comm/`, `scripts/`
- Affected docs: `obc-dev-spec/`, `docs/test-records/`
