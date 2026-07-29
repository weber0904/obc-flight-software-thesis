## 1. Define the shared ADCS host protocol

- [x] 1.1 Extend the shared hosted protocol and add ADCS transport/client code under `simulators/common/` and `simulators/adcs/`
- [x] 1.2 Reconcile the narrative ADCS source document with any newly explicit protocol or behavior decisions

## 2. Implement the first ADCS subsystem slice

- [x] 2.1 Add the hosted ADCS simulator executable with deterministic quaternion, rate, pointing-error, and sensor-validity behavior
- [x] 2.2 Add `AdcsBridge` with the owned `ADCS_*` command, telemetry, and event families plus fallback-on-invalid-reply behavior

## 3. Add automated verification

- [x] 3.1 Add focused F' unit tests for `AdcsBridge`
- [x] 3.2 Add a host-side ZMQ integration test for the ADCS simulator and client path
- [x] 3.3 Run the normal build, unit-test build, F' unit tests, and the host integration executable

## 4. Capture evidence and close the change

- [x] 4.1 Record the implementation and verification results under `evidence/records/adcs-subsystem-v1/`
- [x] 4.2 Run `openspec validate adcs-subsystem-v1` and archive the change after the implementation and evidence are complete
