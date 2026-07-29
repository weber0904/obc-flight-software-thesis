# Tasks: rpi-target-integration-v1

## 1. Portable platform entrypoints

- [x] 1.1 Replace Darwin-only build artifact assumptions with portable repository-local discovery helpers
- [x] 1.2 Update the hosted launch scripts so they run unchanged against Linux build outputs as well as Darwin outputs
- [x] 1.3 Add repository-local Raspberry Pi sync/build/run helpers for the governed `integ-rpi` workflow

## 2. Target runtime configuration

- [x] 2.1 Make boot metadata and staging roots configurable from the runtime / launch path
- [x] 2.2 Keep target transport selection profile-driven so the same OBC logic can run with TCP mock or UART device-path settings

## 3. Raspberry Pi validation

- [x] 3.1 Sync the governed workspace to the Raspberry Pi target and bootstrap the native target build environment
- [x] 3.2 Build the project natively on the Raspberry Pi and verify the integrated target stack launches there
- [x] 3.3 Run the target stack against the documented GDS path and capture the observed target connectivity
- [x] 3.4 Validate that boot metadata survives a target-side process restart and that the confirm / rollback window resumes correctly

## 4. Evidence and spec sync

- [x] 4.1 Record Raspberry Pi target evidence under `evidence/records/`
- [x] 4.2 Update the affected narrative source documents and top-level README for the target workflow
- [x] 4.3 Run OpenSpec validation for the change and archive it after the implementation and evidence are complete
