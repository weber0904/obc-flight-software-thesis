## Context

`WatchdogSupervisor` already computes aggregate watchdog freshness and only
strobes `watchdogFeedOut` when feed remains eligible. In the active topology,
that output port is intentionally unconnected, so current watchdog behavior can
prove software supervision and shared recovery semantics but cannot prove target
hardware reset.

The Raspberry Pi target exposes `bcm2835-wdt` as `/dev/watchdog0` with fixed
15-second timeout, no `SETTIMEOUT` support, and `nowayout=0`. Non-destructive
smoke showed that the device can be opened, fed, cleanly closed without causing
board reboot, and reopened after release. The remaining target-side gaps are
service access permissions, explicit sink ownership, and recovery-path
separation between process restart and watchdog-caused board reboot.

## Goals / Non-Goals

**Goals:**

- Connect the active watchdog policy owner to Raspberry Pi hardware watchdog
  stroking on the governed target baseline.
- Keep the OBC service non-root while making `/dev/watchdog0` accessible through
  repo-owned target install/systemd configuration.
- Preserve existing `R2` process restart and non-watchdog `R6` runtime-exit
  semantics while making watchdog-source `R6` become hardware-reset-backed on
  target.
- Add target proof that shows watchdog-source stale suppression leads to board
  reboot and post-boot recovery evidence.
- Keep the target proof bounded by a probe-owned quiet diagnostic path rather
  than claiming the general serial/background-TM problem is solved.

**Non-Goals:**

- No power-loss recovery, bootloader or partition handoff, external supervisor
  IC integration, or secure-boot redesign.
- No hosted hardware watchdog claim.
- No broad operator-facing watchdog debug feature; only a bounded proof-only
  trigger may be added.
- No productized quiet mode; any quieting surface must stay probe-owned and
  restore normal service behavior after the proof exits.

## Decisions

- Add a target-only Linux watchdog sink that opens `/dev/watchdog0`, writes
  keepalive on every `watchdogFeedOut`, and cleanly disarms on normal shutdown.
- Add `RuntimeConfig` and installed-launch flags for
  `disabled|linux-device`, device path, and timeout seconds. Hosted remains
  `disabled`; Raspberry Pi service defaults to `linux-device`.
- Keep `WatchdogSupervisor` as the only feed-eligibility owner. The new sink is
  transport-like target plumbing only.
- Do not run the active OBC service as root. Instead, install a repo-owned udev
  rule for `/dev/watchdog0`, create or reuse a `watchdog` group, and add
  `SupplementaryGroups=watchdog` to `obc-comm-csp-stack.service`.
- When hardware watchdog mode is enabled, watchdog-source `R6_OBC_REBOOT`
  remains persisted through `BootManager` but does not request runtime exit
  `32`; the process remains alive and stops stroking the watchdog so hardware
  timeout performs the reset. Non-watchdog `R6` still exits `32`. `R2` still
  exits `31`.
- Add a bounded proof-only watchdog suppression surface because the active
  topology lacks a clean natural target trigger that can stop a single watchdog
  source beat without first killing the whole OBC process.
- Add a diagnostic-only quiet egress switch at `CommEgressMux`, plumbed through
  runtime and target service configuration, so the watchdog reset probe can
  temporarily suppress unsolicited packet/file downlink on the active serial
  ground path.
- Prove reboot on target by observing SSH disconnect/reconnect, changed boot
  evidence (`journalctl --list-boots` / uptime boundary), service recovery, boot
  metadata, and persistent fault readback.
- Use journal-first acceptance for the quiet-path target probe. GDS/event logs
  remain secondary diagnostics because quiet mode intentionally suppresses the
  noisy downlink that previously corrupted the serial path.

## Risks / Trade-offs

- `/dev/watchdog0` permission handling is target-OS specific. Mitigation:
  manage it only through repo-owned install helpers and document the contract.
- Linux watchdog drivers vary in close semantics. Mitigation: keep the capability
  gate in the change and fail the change if clean stop/reopen cannot be proven.
- Watchdog-source `R6` not exiting immediately changes target-only runtime
  behavior. Mitigation: scope that policy behind hardware-watchdog-enabled mode
  and add unit coverage that non-watchdog `R6` remains unchanged.
- Quiet mode could accidentally escape into normal service use. Mitigation:
  keep the switch diagnostic-only, default it off in the installed service,
  apply it only through a temporary probe-owned systemd override, and force
  cleanup that restores normal mode on success and failure.
