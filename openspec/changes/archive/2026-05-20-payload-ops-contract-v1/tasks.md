## 1. OpenSpec Artifacts

- [x] 1.1 Create `proposal.md`, `design.md`, `tasks.md`, and delta specs for
  `payload-operations`, `core-system-contracts`, `platform-baseline`,
  `verification-evidence`, and `resource-storage`.

## 2. Payload Runtime And Public Contract

- [x] 2.1 Add `PayloadOpsController` as the only public payload owner with the
  `PAYLOAD_*` command family, payload events/telemetry, deferred completion,
  and bounded payload state machine.
- [x] 2.2 Add `PiCameraManager`, `IPiCameraDriver`, target-only
  `LibcameraPiCameraDriver`, and hosted/test `StubPiCameraDriver` under the
  controller without exposing backend details through the public command
  contract.
- [x] 2.3 Implement payload-local mode admission and forced-exit cleanup using
  existing runtime mode truth rather than a second mode owner.
- [x] 2.4 Implement payload-local EPS proxy orchestration using reserved proxy
  channel `3`, power-settle timing, init timeout handling, and explicit lab
  proxy truth that does not claim a physically switched EPS rail.
- [x] 2.5 Implement governed payload capture storage under
  `<runtime-root>/persistent-data/payload/camera/` with deterministic capture
  naming and last-result readback.

## 3. Topology And Authority Integration

- [x] 3.1 Instantiate and wire `PayloadOpsController` into active `TopCcsds`
  with the needed scheduler/runtime configuration and support-layer setup.
- [x] 3.2 Extend command authority catalog, policy, and shared enums with
  payload resource labeling and the new `PAYLOAD_*` commands.
- [x] 3.3 Keep official sequencing as the payload consumer by ensuring payload
  commands work through the existing sequence-admission path without adding a
  parallel scheduler or stock backend bypass.

## 4. Tests And Probes

- [x] 4.1 Add a classic F' component UT harness for `PayloadOpsController`.
- [x] 4.2 Add focused helper or manager tests for explicit-prepare
  requirements, mode gating, busy rejection, proxy power behavior, settle/init
  failure handling, abort semantics, forced mode exit, and last-result/path
  status.
- [x] 4.3 Add or update authority tests covering payload resource labeling and
  policy entries.
- [x] 4.4 Add a repository-owned hosted probe that proves the governed payload
  contract path and official sequencing use with the stub driver.
- [x] 4.5 Add a repository-owned Raspberry Pi target probe and evidence
  workflow for the real `libcamera` path, proxy power notification, and local
  JPEG capture.

## 5. Docs, Evidence, And Validation

- [x] 5.1 Update `docs/interfaces.md`,
  `docs/architecture/current-development-architecture.md`,
  `docs/verification-path-registry.md`, and add
  `docs/test-records/payload-ops-contract-v1/README.md`.
- [x] 5.2 Run focused tests and probes after a fresh build, recording the
  hosted-versus-target evidence split honestly.
- [x] 5.3 Run `openspec validate payload-ops-contract-v1`.
- [x] 5.4 Run `openspec validate --specs`.
