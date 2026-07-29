## Why

`RecoveryExecutor` currently treats `ADCS_POLL_TRANSPORT` and
`ADCS_POLL_FRESHNESS` as `R2_RESTART_SOFTWARE_COMPONENT` sources and responds
to the first ADCS scheduled-poll fault by persisting process-restart metadata.
That behavior skips the subsystem-owned ADCS recovery plane even though the EPS
path already proves a bounded `R3_RESET_SUBSYSTEM_INTERFACE` action through a
real CSP reset request/reply chain.

The current ADCS simulator stack already has internal reset/default-state logic
in `AdcsSimModel`, but the owned ADCS CSP protocol, transport, simulator
server, bridge runtime surface, and shared recovery wiring do not expose it.
This leaves the ADCS shared-recovery path behaviorally different from EPS and
prevents the first ADCS recovery step from being a true subsystem-interface
reset.

## What Changes

- Add an ADCS-owned CSP reset service on a new service port `24`.
- Expose ADCS reset through the transport, simulator server, simulator model,
  and `AdcsBridge` runtime surface.
- Add a minimal ADCS recovery-control interface for `RecoveryExecutor` and wire
  `AdcsBridge` into it.
- Change `ADCS_POLL_TRANSPORT` and `ADCS_POLL_FRESHNESS` to start at
  `R3_RESET_SUBSYSTEM_INTERFACE` and execute ADCS reset as the first recovery
  action.
- Preserve existing relatch escalation and existing `R3` /
  `SUBSYSTEM_INTERFACE_RESET` naming.
- Add focused unit, integration, and hosted-probe validation plus formal spec
  and verification-path updates for the new ADCS reset path.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `adcs-subsystem`: ADCS-owned CSP protocol and simulator behavior now include
  a reset service and a recovery-visible reset path.
- `mission-autonomy`: shared ADCS scheduled-poll recovery now starts at
  `R3_RESET_SUBSYSTEM_INTERFACE` and issues ADCS reset instead of OBC process
  restart.
- `verification-path-registry`: the active ADCS internal CSP path and shared
  recovery path descriptions must reflect the added reset service and the new
  ADCS first-fault recovery action.

## Impact

- Affected code includes ADCS protocol/transport/simulator sources,
  `AdcsBridge`, `RecoveryRuntime`, `RecoveryExecutor`, topology runtime wiring,
  and related tests.
- Affected verification includes ADCS simulator/model tests, ADCS bridge and
  RecoveryExecutor unit tests, the ADCS CSP integration client, and the hosted
  shared recovery probe.
- Affected documentation includes the ADCS subsystem OpenSpec spec and the
  verification-path registry entry describing the ADCS hosted internal CSP path
  and bounded shared recovery path.
