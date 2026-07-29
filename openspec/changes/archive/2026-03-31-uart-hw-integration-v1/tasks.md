## 1. Change Scaffolding

- [x] 1.1 Sync the UART hardware-integration delta specs with the chosen host-peer and Raspberry Pi validation flow
- [x] 1.2 Identify the existing comm runtime and mock-radio entrypoints that must be generalized from PTY-only wording to serial-device wording

## 2. Host And Target Runtime Implementation

- [x] 2.1 Extend the hosted mock-radio backend to serve the existing protocol over an explicit serial device path in addition to TCP and PTY modes
- [x] 2.2 Update the comm runtime help text and transport helpers so explicit serial-device paths are documented and reusable for hardware UART launches
- [x] 2.3 Add repo-local helper scripts for launching the host serial peer and the Raspberry Pi hardware-UART stack with explicit device inputs

## 3. Verification And Evidence

- [x] 3.1 Re-run the shared local verification gate to confirm existing TCP and PTY comm validation still passes
- [x] 3.2 Execute the governed Raspberry Pi to host hardware-UART validation flow and capture reviewable evidence for device selection, launch commands, and observed serial-link behavior
- [x] 3.3 Update repo documentation and test records to reflect the cleared Raspberry Pi-to-host UART hardware path and any remaining `Blocked-HW` gaps

## 4. Finalize

- [x] 4.1 Validate the OpenSpec change and sync the resulting deltas into the main specs
- [x] 4.2 Archive the change and preserve a clean git state for the next hardware slice
