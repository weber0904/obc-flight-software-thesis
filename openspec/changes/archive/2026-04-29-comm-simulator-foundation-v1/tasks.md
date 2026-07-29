## 1. Formalize the simulator-foundation contract

- [x] 1.1 Create proposal, design, and delta spec artifacts for the `CommSimModel` foundation without expanding gateway, RF, file/downlink, or SocketCAN scope.
- [x] 1.2 Validate the active OpenSpec change before implementation closeout.

## 2. Implement the COMM model/server split

- [x] 2.1 Add `CommSimModel`, `CommSimConfig`, and `CommSimStatus` with bounded uplink queue, link state, downlink acceptance/backpressure, overflow counters, and model-only injection APIs.
- [x] 2.2 Refactor `CommNodeServer` so CSP service semantics delegate to `CommSimModel` while the server shell keeps libcsp and serial I/O ownership.
- [x] 2.3 Preserve node `4`, services `30-32`, packet sizes, and existing `CommCspProtocol` compatibility.

## 3. Add regression coverage

- [x] 3.1 Add `CommSimModelUnitTest` covering reset/default status, link flags, uplink poll ordering, `NO_CHUNK`, drop-new overflow, invalid requests, downlink acceptance/rejection, backpressure, and error counters.
- [x] 3.2 Keep existing `comm_groundlink_unit_test` coverage green for ports `30-32` compatibility.
- [x] 3.3 Register the new model source and test in the simulator CMake build.

## 4. Verify and record evidence

- [x] 4.1 Run the fresh local gate with `bash scripts/run_verification_ci.sh build-artifacts/comm-simulator-foundation-v1-closeout`.
- [x] 4.2 Run the focused gateway compatibility probe with `bash scripts/run_comm_csp_ground_gateway_probe.sh`.
- [x] 4.3 Add `docs/test-records/comm-simulator-foundation-v1/README.md` with clear proof boundaries.
- [x] 4.4 Run `openspec validate comm-simulator-foundation-v1` and `openspec validate --specs`, then archive the change through OpenSpec.
