## Why

The active `TopCcsds` baseline already has software watchdog ownership through
`WatchdogSupervisor`, shared recovery ownership through `RecoveryExecutor`, and
service-managed target restart evidence for `R2` process restart. The remaining
target-recovery gap is that the Raspberry Pi baseline still does not prove
hardware watchdog stroking or watchdog-caused board reset.

The Raspberry Pi target already exposes `bcm2835-wdt` as `/dev/watchdog0` with
the governed lab baseline, but the active OBC service does not own that device,
does not stroke it from `WatchdogSupervisor`, and does not distinguish
watchdog-caused board reboot from existing process-restart or runtime-exit
paths. This change closes that gap with a bounded target-only hardware watchdog
integration and proof boundary.

## What Changes

- Add a capability gate and governed target integration for Raspberry Pi
  `bcm2835-wdt` under the active `obc-comm-csp-stack.service` baseline.
- Connect existing `WatchdogSupervisor.watchdogFeedOut` to a target-only Linux
  watchdog sink instead of inventing a second watchdog policy owner.
- Add target/runtime configuration for hardware watchdog mode, device path, and
  timeout while keeping hosted baselines disabled by default.
- Add a governed target permission/install path so the non-root OBC service can
  access `/dev/watchdog0` through repo-owned udev and systemd configuration.
- Split watchdog-source target hardware-reset behavior from existing `R2`
  process restart and non-watchdog `R6` runtime exit paths.
- Add a bounded proof-only watchdog suppression trigger and a Raspberry Pi probe
  that proves watchdog-caused board reboot with boot metadata and persistent
  fault readback.
- Add a probe-owned quiet diagnostic path that temporarily suppresses
  unsolicited packet/file egress on the active ground path so the target
  hardware-watchdog proof can run on a quiet serial link without changing the
  default runtime behavior.
- Update active architecture, interface/runbook, verification registry, and
  evidence docs to reflect the new closure and keep power-loss recovery out of
  scope.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `mission-autonomy`: watchdog recovery shall support a hardware-watchdog-backed
  target reboot path distinct from service-managed `R2` restart and non-watchdog
  `R6` runtime exit.
- `platform-baseline`: the governed Raspberry Pi baseline shall support
  repo-owned hardware watchdog access, service integration, and board-reset
  proof through the active `TopCcsds` deployment.
- `verification-evidence`: evidence requirements shall include capability-gate
  results, target watchdog integration proof, and explicit separation from
  process restart and power-loss claims.
- `verification-path-registry`: the registry shall record the Raspberry Pi
  hardware watchdog reset path as distinct from hosted watchdog supervision,
  target service-managed `R2` restart, Linux reboot, and power-loss recovery.

## Impact

- Affected code includes hosted/runtime configuration parsing, the active target
  launch path, Raspberry Pi install/systemd helpers, target-only watchdog sink
  ownership, and `RecoveryExecutor` target hardware-watchdog exit policy.
- Affected probes include a new target hardware watchdog reset proof plus
  regression on the existing target `R2` restart path.
- Affected docs include active architecture truth, target operator guidance,
  `docs/interfaces.md`, the verification-path registry, and a new
  `evidence/records/target-hardware-watchdog-reset-proof-v1/` evidence record.
