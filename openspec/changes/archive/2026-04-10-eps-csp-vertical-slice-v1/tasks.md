## 1. Governance And Scope

- [x] 1.1 Add delta specs for `eps-subsystem`, `verification-evidence`, and `verification-path-registry`.
- [x] 1.2 Update narrative docs, verification-path registry, and evidence so EPS CSP is distinct from foundation CSP, ground, external comm, GPS, and still-legacy ADCS paths.

## 2. EPS CSP Payload And Transport

- [x] 2.1 Add EPS-owned CSP service payload definitions for status, PDU, heater/config, and reset.
- [x] 2.2 Implement `CspEpsTransport` behind `IEpsTransport`.
- [x] 2.3 Switch `makeDefaultEpsTransport()` to return the libcsp-backed EPS transport while keeping fake-transport UT injection intact.

## 3. EPS Simulator Migration

- [x] 3.1 Rework hosted `eps_simulator` into libcsp node `2` using the hosted ZMQHUB foundation.
- [x] 3.2 Preserve `EpsSimModel` deterministic state behavior.
- [x] 3.3 Keep ADCS simulator and transport unchanged for this phase.

## 4. Scripts And Verification

- [x] 4.1 Update hosted stack startup so EPS uses the internal CSP hub rather than direct ZMQ endpoint arguments.
- [x] 4.2 Replace `eps_zmq_integration_test` with `eps_csp_integration_test`.
- [x] 4.3 Capture governed evidence for the hosted EPS CSP vertical-slice path.

## 5. Validation

- [x] 5.1 Run `EpsBridge` classic component UT.
- [x] 5.2 Run `eps_csp_integration_test`.
- [x] 5.3 Run `bash scripts/run_verification_ci.sh <artifact-dir>`.
- [x] 5.4 Run `openspec validate eps-csp-vertical-slice-v1` and `openspec validate --specs`.
