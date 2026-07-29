## 1. ADCS CSP Protocol And Transport

- [x] 1.1 Add ADCS-owned CSP request/reply payload definitions and application service ports for state, mode, target, and calibration.
- [x] 1.2 Add `CspAdcsTransport` using `ICspRuntime::requestReply` and make `makeDefaultAdcsTransport()` select it by default.
- [x] 1.3 Preserve the legacy ZMQ transport as temporary migration compatibility, but keep it out of the default hosted path.

## 2. Hosted Simulator Migration

- [x] 2.1 Convert the hosted ADCS simulator startup path to run as libcsp node `3`.
- [x] 2.2 Add CSP service dispatch for ADCS state, mode, target, and calibration requests while preserving `AdcsSimModel` behavior.
- [x] 2.3 Update hosted stack scripts so ADCS uses the internal CSP hub configuration instead of direct ZMQ endpoint assumptions.

## 3. Verification And Evidence

- [x] 3.1 Add a focused hosted ADCS CSP integration smoke and CTest registration with a timeout.
- [x] 3.2 Update the verification path registry and evidence record for the hosted ADCS internal libcsp service path.
- [x] 3.3 Update verification matrix/inventory and narrative source docs to reflect ADCS CSP migration and legacy ZMQ status.

## 4. Validation

- [x] 4.1 Run focused ADCS CSP integration and `AdcsBridge` component tests.
- [x] 4.2 Run repo consistency checks and OpenSpec validation.
- [x] 4.3 Run the shared baseline gate before push.
