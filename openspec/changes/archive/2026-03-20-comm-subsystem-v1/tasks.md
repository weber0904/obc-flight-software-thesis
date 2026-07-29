## 1. Define the first comm transport slice

- [x] 1.1 Add a shared host-side byte-stream transport layer with TCP mock and PTY-backed implementations under `simulators/comm/`
- [x] 1.2 Reconcile the narrative comm source document with any newly explicit transport or protocol decisions

## 2. Implement the first comm subsystem components

- [x] 2.1 Add `CommController` with `COMM_*` commands, pass-window state, and related telemetry/events
- [x] 2.2 Add `RadioController` with `RADIO_*` commands, status telemetry, and mock-radio transport integration
- [x] 2.3 Add `UartDriver` with `UART_*` telemetry/events and connectivity/error accounting

## 3. Add automated verification

- [x] 3.1 Add focused F' unit tests for `CommController`, `RadioController`, and `UartDriver`
- [x] 3.2 Add a host-side integration test that validates both TCP mock and PTY-backed transport paths
- [x] 3.3 Run the normal build, unit-test build, and registered test suite

## 4. Capture evidence and close the change

- [x] 4.1 Record the implementation and verification results under `evidence/records/comm-subsystem-v1/`
- [x] 4.2 Run `openspec validate comm-subsystem-v1` and archive the change after the implementation and evidence are complete
