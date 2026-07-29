## 1. OpenSpec And Scope

- [x] 1.1 Create `legacy-zmq-retirement-v1` artifacts for proposal, design, specs, and tasks.
- [x] 1.2 Keep the change scoped to EPS/ADCS internal hosted communication cleanup only.

## 2. DTO And Protocol Refactor

- [x] 2.1 Add EPS-owned runtime DTO/result header and update EPS includes.
- [x] 2.2 Add ADCS-owned runtime DTO/result header and update ADCS includes.
- [x] 2.3 Delete `simulators/common/protocol.h` and remove active shared wire-protocol usage.

## 3. Direct-ZMQ Removal

- [x] 3.1 Remove `ZmqEpsTransport`, endpoint defaults, and EPS direct-ZMQ request/reply code.
- [x] 3.2 Remove `ZmqAdcsTransport`, endpoint defaults, and ADCS direct-ZMQ request/reply code.
- [x] 3.3 Remove stale ZMQ integration source and direct-ZMQ CMake assumptions.
- [x] 3.4 Update `EpsBridge` and `AdcsBridge` constructors/factories to remove endpoint compatibility while preserving F' behavior.

## 4. Regression Guardrail

- [x] 4.1 Add `scripts/check_legacy_zmq_retired.py` with allowlisted libcsp ZMQHUB usage and historical evidence paths.
- [x] 4.2 Wire the checker into `scripts/run_verification_ci.sh`.

## 5. Docs And Evidence

- [x] 5.1 Update README, simulator README, narrative docs, verification matrix, and registry.
- [x] 5.2 Add `evidence/records/legacy-zmq-retirement-v1/README.md`.
- [x] 5.3 Update reporting-package wording where direct-ZMQ fallback could be inferred.

## 6. Validation

- [x] 6.1 Run focused build and UT build.
- [x] 6.2 Run EPS CSP, ADCS CSP, CSP runtime, and scenario bridge checks.
- [x] 6.3 Run legacy-ZMQ checker, repo consistency, verification inventory, and OpenSpec validation.
- [x] 6.4 Run the full baseline gate before push.
