# payload-sensor-register-controls-v1 Test Record

## Scope

- Branch: `feature/payload-ops-v2`
- OpenSpec change: `payload-sensor-register-controls-v1`
- Date: 2026-05-21

This record captures the first in-branch verification state for governed raw
sensor-register controls on the OV5647 payload path.

The current code scope covered here is:

- public `PAYLOAD_SENSOR_REG_READ(address)` command
- public `PAYLOAD_SENSOR_REG_WRITE(address, value, verifyReadback)` command
- `RAW_SENSOR`-session-only gating for raw register access
- new `PAYLOAD_CAPTURE_RAW(tag)` surface
- hosted fake register adapter inside `StubPiCameraDriver`
- explicit target non-closure hooks inside `LibcameraPiCameraDriver`

## Commands

Commands run from `$REPO_ROOT`:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build -j 8
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native-ut --target OBC_Components_PayloadOpsController_ut_exe OBC_Components_CommandIngressAuthority_ut_exe -j 8
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_PayloadOpsController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
bash -n scripts/run_payload_sensor_register_controls_v1_hosted_probe.sh
bash scripts/run_payload_sensor_register_controls_v1_hosted_probe.sh
PATH="$PWD/fprime-venv/bin:$PATH" openspec validate payload-sensor-register-controls-v1
```

## Current Results

- Fresh native build: PASS
- Focused UT target build: PASS
- `OBC_Components_PayloadOpsController_ut_exe`: PASS
- `OBC_Components_CommandIngressAuthority_ut_exe`: PASS
- `bash -n scripts/run_payload_sensor_register_controls_v1_hosted_probe.sh`: PASS
- `bash scripts/run_payload_sensor_register_controls_v1_hosted_probe.sh`: PASS
  - canonical root: `/tmp/payload-sensor-register-controls-v1-hosted.LUk9ax`
- `openspec validate payload-sensor-register-controls-v1`: PASS

## Focused Coverage

`OBC_Components_PayloadOpsController_ut_exe` currently proves:

- raw register access is rejected before prepare
- raw register access is rejected while prepared in non-`RAW_SENSOR` sessions
- raw register write + read succeeds in a prepared `RAW_SENSOR` session on the
  hosted stub backend
- `RAW_SENSOR` prepare uses the prepared raw session kind rather than silently
  falling back to the deterministic session kind

`OBC_Components_CommandIngressAuthority_ut_exe` currently proves:

- `PAYLOAD_SENSOR_REG_READ` stays a governed read/status payload surface
- `PAYLOAD_SENSOR_REG_WRITE` and `PAYLOAD_CAPTURE_RAW` stay governed
  payload-control surfaces and are not granted to restricted backup ingress

## Hosted And Target Closure Status

Hosted truth today:

- the branch has a fake hosted adapter that exposes a deterministic in-memory
  OV5647-like register map through `StubPiCameraDriver`
- raw register behavior is covered both at component level through UT and by a
  fresh hosted CCSDS node-`5` PASS at
  `/tmp/payload-sensor-register-controls-v1-hosted.LUk9ax`
- the hosted proof reuses the freshly re-stabilized shared sequencing/file-
  ingress spine re-proven at `/tmp/official-sequencing-system-resources-v1.oUHfai`
- official wrapper sequencing is proven on the hosted path for
  `.sequence-staging/psrc1.bin`: `SEQ_VALIDATE` is proven as an observed
  wrapper envelope plus bounded no-reject preflight, `SEQ_RUN(..., WAIT)`
  executes the `RAW_SENSOR` sequence, and the same run produces governed raw-
  register read/write events, `PAYLOAD_CAPTURE_RAW`, metadata sidecars, and
  clean shutdown
- as with the capture-modes hosted proof, canonical cleanup truth for this
  probe requires running the repository-owned script without sandbox
  process-list restrictions so the probe can reap `fprime_gds` children
- the hosted proof remains explicitly fake-backend-only; it proves contract,
  authority, sequencing, and observability, not real OV5647 register writes

Target truth today:

- `LibcameraPiCameraDriver` contains explicit raw-register hook methods
- those hooks currently return a bounded unsupported/error response rather than
  claiming a real OV5647 register access path
- there is therefore **no** target raw-register proof claim yet

This record is an explicit bounded non-claim for target raw-register closure at
the current branch state.

## Non-Claims

- no real OV5647 register round-trip proof yet
- no claim that every OV5647 exposure/gain/frame-timing register is safely
  writable while streaming
