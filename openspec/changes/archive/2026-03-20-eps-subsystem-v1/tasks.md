## 1. Define the shared EPS host protocol

- [x] 1.1 Add the shared EPS protocol definitions and host transport/client code under `simulators/common/` and `simulators/eps/`
- [x] 1.2 Reconcile the narrative EPS source document with any newly explicit protocol or behavior decisions

## 2. Implement the first EPS subsystem slice

- [x] 2.1 Add the hosted EPS simulator executable with deterministic battery, solar, heater, and PDU state handling
- [x] 2.2 Add `EpsBridge` with the owned `EPS_*` command, telemetry, and event families plus fallback-on-timeout behavior

## 3. Add automated verification

- [x] 3.1 Add focused F' unit tests for `EpsBridge`
- [x] 3.2 Add a host-side ZMQ integration test for the EPS simulator and client path
- [x] 3.3 Run the normal build, unit-test build, F' unit tests, and the host integration executable

## 4. Capture evidence and close the change

- [x] 4.1 Record the implementation and verification results under `evidence/records/eps-subsystem-v1/`
- [x] 4.2 Run `openspec validate eps-subsystem-v1` and archive the change after the implementation and evidence are complete
