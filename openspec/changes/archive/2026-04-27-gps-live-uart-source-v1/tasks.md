## 1. Change Artifacts

- [x] 1.1 Add delta specs for GPS live UART, platform baseline, comm ownership realignment, evidence, and path registration

## 2. GPS Runtime And Source Support

- [x] 2.1 Extend `GpsSourceMode` and runtime helpers to include `LIVE_UART`
- [x] 2.2 Implement a dedicated serial-backed `IGpsSentenceSource` with bounded line-oriented UART ingest
- [x] 2.3 Extend `GpsBridge` runtime configuration and source activation to support fake, replay, and live-uart consistently

## 3. Tests

- [x] 3.1 Add PTY-backed L1 tests for the new serial GPS source
- [x] 3.2 Extend `GpsBridge` classic component and contract coverage for live-uart mode selection and failure behavior
- [x] 3.3 Re-run the existing hosted GPS regression without behavior drift

## 4. Target Probe And Evidence

- [x] 4.1 Add a governed `run_rpi_gps_live_probe.sh` for `obc.local:/dev/serial0`
- [x] 4.2 Record evidence for the first hardware-backed GPS live UART path and explicitly mark old OBC-side serial comm evidence as historical

## 5. Closeout Validation

- [x] 5.1 Run `python3 scripts/check_repo_consistency.py`
- [x] 5.2 Run `python3 scripts/check_agent_entrypoint.py`
- [x] 5.3 Run `openspec validate gps-live-uart-source-v1`
- [x] 5.4 Run `openspec validate --specs`
