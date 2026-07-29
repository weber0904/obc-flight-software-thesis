## 1. Define the UART preflight contract

- [x] 1.1 Add and validate OpenSpec artifacts for the subsystem-side COMM UART preflight path and its evidence boundaries.
- [x] 1.2 Update reviewer-facing docs that still describe hosted gateway-backed omitted-RF TT&C as future-only.

## 2. Add the subsystem-side serial probe

- [x] 2.1 Add a native-built serial probe executable that opens an explicit serial device and performs bounded `STATUS`, `ENABLE 1`, `STATUS` mock-text exchanges.
- [x] 2.2 Register the probe in CMake without changing the existing COMM node service contract or adding new CSP services.

## 3. Add governed probe orchestration

- [x] 3.1 Add a repository-owned script that starts one side as a mock-text serial peer, runs the other side as the requester, captures logs, and asserts the final enabled status.
- [x] 3.2 Ensure the script requires explicit host and subsystem serial device paths and keeps runtime logs under a bounded temporary directory.

## 4. Verify and record evidence

- [x] 4.1 Run the fresh local verification gate and OpenSpec validations.
- [x] 4.2 Run the subsystem native-build prep.
- [x] 4.3 Run the focused UART preflight probe to PASS.
- [x] 4.4 Record passing hardware evidence under `docs/test-records/subsystem-comm-uart-link-preflight-v1/`.
- [x] 4.5 Register the new verification path after the focused probe passes.

Final status: LIMITED PASS. The proven repo-owned preflight uses `mac-to-subsystem` direction with `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE`, `SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0`, and `COMM_BAUDRATE=115200`. `serial-getty@ttyS0` is inactive, serial console has been removed from `/boot/firmware/cmdline.txt`, and `/dev/ttyS0` is `root:dialout crw-rw----`. Follow-up diagnostics found that clean `subsystem.local` cold-first traffic into a passive macOS receiver is not proven by this change.
