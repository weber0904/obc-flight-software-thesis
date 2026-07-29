## 1. ADCS Reset Path

- [x] 1.1 Add ADCS CSP reset service port `24` and extend the ADCS transport
      and simulator server/model to serve reset request/reply traffic.
- [x] 1.2 Add `AdcsBridge` recovery reset runtime entry points and keep reset
      success/failure behavior aligned with the existing bridge apply/cache
      semantics.

## 2. Shared Recovery Integration

- [x] 2.1 Add the minimal ADCS recovery-control interface and wire
      `RecoveryExecutor` and topology/test callers to use it.
- [x] 2.2 Change ADCS shared scheduled-poll recovery mapping and first-action
      behavior from R2 process restart to R3 subsystem-interface reset while
      preserving relatch escalation.

## 3. Validation And Documentation

- [x] 3.1 Update focused unit/integration tests for ADCS reset request/reply,
      simulator reset state restoration, bridge reset behavior, and shared
      recovery mapping/action changes.
- [x] 3.2 Update and rerun the hosted shared recovery probe so ADCS first-fault
      expectations use R3 reset instead of R2 process restart.
- [x] 3.3 Update the active ADCS subsystem and verification-path OpenSpec delta
      specs plus repo-facing registry text for the new reset service and ADCS
      recovery behavior.
