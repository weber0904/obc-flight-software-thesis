# Target OBC COMM CSP Lab Operational Runbook

Status: current target/lab operator runbook.
Last reconciled during `chapter5-integrated-route-closure-v1` plus
`uhf-primary-nonquiet-autofailover-v1` on 2026-06-25.

This runbook is the operator entrypoint for the current lab target path:

```text
default target path
macOS fprime-gds + ground_ttc_gateway
  -> S-band TCP relay
  -> subsystem.local sband_comm_csp_node node 5 on can1
  -> shared SocketCAN bus
  -> obc.local OBC node 1 on can0

maintained UHF-primary failover path
macOS fprime-gds + ground_ttc_gateway
  -> physical lab serial ingress
  -> subsystem.local uhf_comm_csp_node node 6 on can1
  -> shared SocketCAN bus
  -> obc.local OBC node 1 on can0

historical bounded quiet UHF diagnostic path
macOS fprime-gds + ground_ttc_gateway
  -> physical lab serial ingress
  -> subsystem.local uhf_comm_csp_node node 6 on can1
  -> shared SocketCAN bus
  -> obc.local OBC node 1 on can0
```

This runbook documents the maintained default node-`5` target path, the
maintained detector-triggered `S-band -> UHF primary` failover path, and the
bounded historical quiet-UHF diagnostic proofs that still remain archived for
exact-scope review only.

Current maintained target/lab communication truth is:

- default operator bootstrap is node `5` S-band
- target/lab readiness now requires both `subsystem-sband-csp.service` and
  `subsystem-uhf-csp.service` by default
- maintained `UHF primary` is non-quiet for live `event/tlm` packet egress
- `UHF beacon suppress` remains a separate auth-triggered runtime policy
- current `S-band -> UHF` promotion truth is internal
  `COMM_PRIMARY_UNAVAILABLE` detection plus `RecoveryExecutor` failover, not a
  manual `COMM_SET_ACTIVE(UHF)` operator step
- the canonical target readiness helper may escalate from ordinary service
  restart repair to a bounded CAN/COMM transport reset when the maintained OBC
  S-band availability marker remains broken after the first repair pass

Target secure-auth proof on this baseline closes installed-release keystore
provenance, S-band APID `0x00FE` secure auth, secure command v2, staged upload
after auth, bounded physical node-`6` `uhf-backup` auth/deny behavior, and UHF
re-auth on the maintained UHF-primary path after promotion or failover.

Historical quiet node-`6` reliable-transfer records remain useful only for the
exact bounded official HK `.fdp` diagnostic slice they proved at the time. They
are not the maintained operator truth for current UHF-primary observability.

For payload delivery, the current active baseline now has both a hosted
official payload `.fdp` family closure path and a bounded governed target
node-`5` COMM-backed payload `.fdp` family proof. Do not treat the older
Pi-local direct payload target probe or the generic target file/downlink probe
as a substitute for that governed target payload entry.

This baseline is operational for the lab target path, but it is not a final flight deployment baseline. The OBC installable bundle carries only the OBC binary, dictionary, metadata, OBC launch profile, and OBC service template. Subsystem service templates, CAN oneshot templates, the ground launcher, and this runbook are repo-governed lab deployment artifacts outside the OBC tarball boundary.

Subsystem workspace services and CAN oneshot provisioning remain transitional
mechanisms. Public examples use the configurable `operator` account; real
installations should use dedicated non-login users such as `obc-runtime` and
`subsystem-runtime`. Root is used only for the CAN oneshot bring-up services.

## Prerequisites

- macOS ground host has this repository built locally with `fprime-gds`, `fprime-cli`, dictionary artifacts, and `ground_ttc_gateway`.
- `obc.local` is reachable by SSH as `operator@obc.local`.
- `subsystem.local` is reachable by SSH as `operator@subsystem.local`.
- `subsystem.local` has the governed workspace synced and natively built.
- `obc.local` has a current OBC installable release under `/home/operator/obc-deploy/current`.
- The lab serial endpoint defaults to `/dev/cu.usbserial-CHANGE_ME` on macOS and `/dev/serial0` on `subsystem.local`.
- The SocketCAN defaults are `can0` on `obc.local`, `can0` for subsystem EPS/ADCS, and `can1` for subsystem COMM. Use an explicit override only if the lab wiring has moved COMM onto a different controller.

Use explicit environment overrides when the lab differs from those defaults:

```bash
OBC_SSH_TARGET=operator@obc.local
SUBSYSTEM_SIM_SSH_TARGET=operator@subsystem.local
HOST_SERIAL_DEVICE=/dev/cu.usbserial-CHANGE_ME
SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0
```

## Package And Install OBC

From macOS, package the current Raspberry Pi build output and install the selected bundle:

```bash
bash scripts/package_rpi_bundle.sh
bash scripts/install_rpi_bundle.sh build-artifacts/packages/rpi/<release-id>/obc-rpi-<release-id>.tar.gz
```

The package manifest should include:

- `launch/run_obc_comm_csp_stack.sh`
- `systemd/obc-comm-csp-stack.service.template`

It should not claim subsystem service templates, CAN oneshot templates, the ground launcher, or this runbook as OBC tarball content.

## Install Lab CAN Provisioning

Install the transitional CAN oneshot units:

```bash
bash scripts/install_lab_can_services.sh
bash scripts/lab_can_status.sh
```

Default timing is:

```text
bitrate 500000 dbitrate 2000000 restart-ms 100 fd on
```

`restart-ms 100` is now part of the governed lab baseline so a transient
SocketCAN `BUS-OFF` does not leave the interface permanently down until a
manual reprovision. This does not claim that underlying `mcp251xfd` CRC or bus
quality issues are fixed; it only keeps the baseline in bounded auto-recovery
mode instead of `restart-ms 0`.

Override `CAN_BITRATE`, `CAN_DBITRATE`, `CAN_RESTART_MS`,
`OBC_CSP_CAN_DEVICE`, `SUBSYSTEM_SIM_CSP_CAN_DEVICE`, or
`SUBSYSTEM_SIM_COMM_CAN_DEVICE` only when the lab wiring requires it. These
oneshots are lab/development provisioning helpers, not the final flight OS CAN
configuration model.

## Install Subsystem Services

Sync and build the subsystem workspace first:

```bash
bash scripts/sync_subsystem_sim_workspace.sh
bash scripts/bootstrap_subsystem_sim_workspace.sh
```

Install the profile-driven aggregate targets and four independent runtime services:

```bash
bash scripts/install_subsystem_comm_csp_services.sh
bash scripts/subsystem_comm_csp_status.sh
```

The subsystem-side units are:

- `subsystem-eps-csp.service`: EPS node `2` on `can0`
- `subsystem-adcs-csp.service`: ADCS node `3` on `can0`
- `subsystem-sband-csp.service`: S-band COMM node `5` on `can1` with TCP listener `0.0.0.0:18520`
- `subsystem-uhf-csp.service`: UHF COMM node `6` on `can1` and `/dev/serial0`

The aggregate targets are:

- `subsystem-sband-csp-stack.target`
- `subsystem-uhf-csp-stack.target`

`TARGET_COMM_PROFILE=sband` enables the S-band aggregate target.
`TARGET_COMM_PROFILE=uhf-primary|uhf-backup` enables the UHF aggregate target
on `subsystem.local`, while the bounded node-`6` proofs still keep the OBC
service on the default S-band bootstrap baseline unless the proof explicitly
documents a different override.

`bash scripts/install_subsystem_comm_csp_services.sh` enables both
`subsystem-sband-csp-stack.target` and `subsystem-uhf-csp-stack.target`.
Current provisioning intent is therefore that `subsystem-uhf-csp.service`
starts automatically on boot through the enabled UHF stack target; current
target readiness must not depend on ad hoc probe-owned service starts for node
`6`.

Use per-service journal output to isolate EPS, ADCS, S-band COMM, and UHF COMM failures instead of treating subsystem runtime as one opaque process.

## Install OBC COMM CSP Service

Install the dedicated OBC COMM CSP service:

```bash
bash scripts/install_rpi_comm_csp_autostart.sh
bash scripts/rpi_comm_csp_status.sh
```

The helper disables and stops `obc-installed-stack.service`, then enables and starts `obc-comm-csp-stack.service`.

Expected default OBC service values:

```text
TARGET_COMM_PROFILE=sband
GROUND_LINK_MODE=comm-csp
COMM_CSP_NODE=5
COMMAND_AUTHORITY_PROFILE=sband-primary
INITIAL_COMM_BAND=sband
CSP_TRANSPORT=socketcan
CSP_CAN_DEVICE=can0
CSP_CAN_PROMISC=0
OBC_GPS_SOURCE_MODE=live-uart
OBC_GPS_SERIAL_DEVICE=/dev/serial0
OBC_GPS_BAUDRATE=9600
HARDWARE_WATCHDOG=linux-device
HARDWARE_WATCHDOG_DEVICE=/dev/watchdog0
HARDWARE_WATCHDOG_TIMEOUT_SEC=15
ENABLE_COMM_SUBSYSTEM_HEALTH_DETECTOR=1
COMM_SUBSYSTEM_PING_TIMEOUT_MS=500
COMM_PRIMARY_UNAVAILABLE_FAILURE_THRESHOLD=10
```

After service start, confirm the OBC journal shows `profile=sband commNode=5 authority=sband-primary`, `Ground link via COMM CSP node: 5`, and GPS source `LIVE_UART`. The GPS acceptance criterion is advancing accepted sentence count; this runbook does not require a valid fix.

The installed launch path now relies on the release-bundled
`config/security/command-auth.ini` asset for command-auth keystore truth. The
governed `obc-comm-csp-stack.service` surface does not use `COMMAND_AUTH_*`
environment overrides.

`TARGET_COMM_PROFILE=uhf-primary` and `TARGET_COMM_PROFILE=uhf-backup` remain
bounded operational profiles, not the normal operator baseline. The maintained
baseline still boots the installed OBC service as `sband-primary` on node `5`,
then relies on detector-triggered autonomous failover when the current primary
becomes unavailable. On that maintained promoted-UHF path, live `event/tlm`
packet egress is non-quiet by default. `UHF beacon suppress` remains a
separate auth-triggered runtime policy and does not imply packet quiet.
`uhf-backup` keeps S-band active and exercises node-`6` as bounded allowlisted
backup ingress rather than beacon-only behavior.

Current node-`5` observability governance truth:

- startup on the maintained S-band target path is quiet by default for
  packetized live `event/tlm`
- accepted S-band secure auth opens node-`5` live `event/tlm` observability
  for that authenticated session only
- revoke, role invalidation, restart, autonomous failover, or other
  primary-band reconfiguration closes node-`5` live `event/tlm` again
- the current content-selection baseline now keeps only curated summary live
  after auth for scheduled `EPS`, `GPS`, `ADCS`, `RADIO`, and `STORAGE`
  surfaces
- formal node-`5` resource keep-live truth now uses
  `OBCApp.watchdogSupervisor.SYS_CPU_USAGE`, `SYS_MEM_RSS_MB`,
  `SYS_RESOURCE_DEGRADED`, and `SYS_LOW_MEMORY`
- `SYS_MEM_RSS_MB` is the current resident-memory truth surface; the warning
  events are threshold-crossing signals rather than repeated above-threshold
  chatter
- detailed state review now prefers fresh bounded `GET_*` / read-status
  readback on that authenticated node-`5` path rather than ambient scheduled
  chatter; this still does not create a second command plane
- `SystemResources.*` is supplemental diagnostics-only live telemetry, not
  node-`5` operator truth
- `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`, transport-error growth,
  `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
  `CSP_OWNER_TIMEOUT` / `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band
  `CommEgressMux` counters remain formal reviewable observability, but not the
  pass-time keep-live summary
- remaining queue-depth, UART, and residual COMM-internal chatter stays
  diagnostics-only / non-baseline live
- `STORAGE_SCAN_NOW` is retired; fresh storage review now goes through
  `STORAGE_GET_STATUS`
- maintained `UHF primary` now remains non-quiet for live packet egress;
  quiet-UHF remains a bounded historical diagnostic overlay only, while beacon
  suppress stays separate and auth-triggered

## Start Ground Stack

From macOS:

```bash
bash scripts/run_target_comm_csp_ground_stack.sh
```

The launcher uses the active `OBC` dictionary and CCSDS GDS framing. It prints
the GDS port, TTS port, file-storage directory, serial endpoint, baudrate,
CCSDS settings, serial preamble settings, and direct-GDS keepalive interval.
Defaults are:

```text
GDS bind: 0.0.0.0:51900
GDS TTS port: 51901
GDS file storage: /tmp/target-obc-comm-csp-lab-gds-downlink
Host serial endpoint: /dev/cu.usbserial-CHANGE_ME @ 115200
CCSDS framing: scid=68 vcid=profile-derived (sband=1, uhf-primary|uhf-backup=2) frame-size=4096
Serial preamble: lines=0 delay-ms=0
GDS keepalive interval: 0
```

The launcher keeps running in the foreground and writes process logs under `build-artifacts/target-obc-comm-csp-ground-stack/`.

## Status And Journal

Use these status helpers first:

```bash
bash scripts/lab_can_status.sh
bash scripts/subsystem_comm_csp_status.sh
bash scripts/rpi_comm_csp_status.sh
```

Direct systemd commands are useful for focused diagnosis:

```bash
ssh operator@obc.local 'systemctl --no-pager --full status obc-comm-csp-stack.service'
ssh operator@obc.local 'journalctl -u obc-comm-csp-stack.service -n 120 --no-pager'
ssh operator@subsystem.local 'systemctl --no-pager --full status subsystem-sband-csp-stack.target subsystem-uhf-csp-stack.target subsystem-eps-csp.service subsystem-adcs-csp.service subsystem-sband-csp.service subsystem-uhf-csp.service'
```

CAN state should remain `ERROR-ACTIVE` with no bus-off condition:

```bash
ssh operator@obc.local 'ip -details -statistics link show can0'
ssh operator@subsystem.local 'ip -details -statistics link show can0; ip -details -statistics link show can1'
```

Review the `restart-ms` field while checking these snapshots. The governed lab
baseline now expects `restart-ms 100` rather than `0`.

## Baseline Governance

Current target/lab proofs no longer own the shared target baseline lifecycle.
The governed ownership model is:

- `bash scripts/ensure_target_comm_lab_baseline.sh`
  ensures or repairs the shared target satellite baseline on `obc.local` and
  `subsystem.local`
- `bash scripts/ensure_ground_dual_gds_baseline.sh`
  ensures the macOS dual-GDS ground baseline is clean before a proof starts

The target baseline manager is the canonical ready-state contract for:

- `obc-lab-can.service`
- `obc-comm-csp-stack.service`
- `subsystem-eps-adcs-lab-can.service`
- `subsystem-comm-lab-can.service`
- `subsystem-eps-csp.service`
- `subsystem-adcs-csp.service`
- `subsystem-sband-csp.service`
- `subsystem-uhf-csp.service`

Current baseline policy is that `bash scripts/ensure_target_comm_lab_baseline.sh`
always requires UHF readiness. A proof may exercise only the S-band functional
path, but the shared target/lab baseline is not allowed to report ready while
node `6` is absent.

It also verifies or repairs:

- `can0/can1` presence and governed `restart-ms 100`
- the S-band listener on `subsystem.local:18520`
- current OBC baseline markers such as node-`5` CSP ping continuity and the
  installed `sband-primary` service
  environment
- probe-owned systemd drop-ins that must not persist across proofs
- duplicate runtime processes that conflict with governed services

It does not require `groundLinkDriver.GROUND_LINK_UP` as an A-layer ready
signal. That event means a ground-side TCP client has actually attached to the
node-`5` S-band listener, and the governed target baseline does not start a
persistent macOS ground client on its own. Fresh `GROUND_LINK_UP` belongs to
the ground-surface or proof layer that starts `fprime-gds` plus
`ground_ttc_gateway`.

The ground baseline manager reaps proof-owned macOS helper residue such as:

- `fprime-gds`
- `fprime_gds.executables.comm`
- `fprime_gds.executables.tcpserver`
- `CustomDataHandlers`
- `ground_ttc_gateway`
- `fprime-cli events`
- `fprime-cli channels`
- secure-auth helper residue

The intended execution order for independent proofs is:

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
bash scripts/<target-proof-wrapper>.sh
```

Within one continuous semantic scenario, such as S-band secure-auth followed by
a governed node-`5` unavailable window, autonomous failover, and UHF re-auth,
the probe must not rerun the baseline managers mid-scenario.

Probe cleanup owns only:

- probe-owned local listeners and helpers
- probe-owned temp runtime roots
- probe-owned temporary service drop-ins or diagnostics overlays

It does not own shared baseline shutdown for OBC, subsystem, or `lab-can`
services.

Current secure-auth proof wrappers may temporarily enable overlays such as:

- `OBC_GROUNDLINK_DIAGNOSTICS=1`
- `SBAND/UHF COMM_NODE_INGRESS_DIAGNOSTICS=1`
- bounded per-proof systemd drop-ins used only for evidence capture

These are not governed baseline settings. They are probe-scoped diagnostics
overlays and must be explicitly removed and verified absent before the proof is
counted complete.

## E2E Verification

Current maintained target/lab proof entrypoints are:

```bash
bash scripts/run_target_secure_auth_command_path_probe.sh
bash scripts/run_target_secure_auth_proof.sh
bash scripts/run_target_uhf_primary_nonquiet_runtime_probe.sh
bash scripts/run_target_autonomous_uhf_failover_probe.sh
PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh
bash scripts/run_rpi_target_hardware_watchdog_reset_probe.sh
```

Use those wrappers when validating the maintained target baseline. In current
policy wording they close these reviewable paths:

- default node-`5` secure-auth bootstrap and secure-command-v2 readback
- default node-`5` observability-gated staged upload and bounded `GET_*`
  readback
- maintained non-quiet `UHF primary` packet observability
- detector-triggered autonomous `S-band -> UHF primary` failover, UHF re-auth,
  beacon suppress, and bounded UHF readback
- target Route `3` ADCS `R3` first-fault proof on the maintained node-`5`
  path
- target Route `3` EPS `R3/R5` first-fault proof on the maintained node-`5`
  path
- target `R6` hardware-watchdog board reset with post-reboot secure-auth
  re-bootstrap

The current maintained UHF-primary proof family is:

```bash
bash scripts/run_target_uhf_primary_nonquiet_runtime_probe.sh
bash scripts/run_target_autonomous_uhf_failover_probe.sh
```

Current governed truth from those wrappers is:

- maintained `UHF primary` is non-quiet for live packet egress
- success is based on fresh ground readback visibility, not journal-only
  acceptance
- target readback steps now use bounded resend-until-ground-readback policy
  rather than one-shot observability assumptions
- promotion from S-band to UHF primary is detector-triggered current truth,
  not a manual `COMM_SET_ACTIVE(UHF)` operator oracle
- accepted UHF secure-auth is still required before bounded `GET_*` readback
  and before beacon suppress may start

Historical reference only:

```bash
bash scripts/run_target_dual_link_proof.sh
bash scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh
```

Keep those wrappers only for exact-scope historical review. They are no longer
the maintained authority for current UHF-primary semantics, current failover
truth, or current ground-readback acceptance policy.

For target secure auth and bounded uplink authority, this repository now has:

```bash
bash scripts/run_target_secure_auth_proof.sh
```

Before accepting this proof, run the current secure-auth command-path preflight
on the same target/subsystem service baseline when provenance was just
refreshed or when CAN/service health is uncertain:

```bash
bash scripts/run_target_secure_auth_command_path_probe.sh
```

For the dedicated node-`5` observability-governance boundary on the same
service-managed target family, use:

```bash
bash scripts/run_target_sband_observability_governance_probe.sh
```

Acceptance for that proof is:

- pre-auth S-band packetized live `event/tlm` remains quiet on node `5`
- accepted S-band APID `0x00FE` secure auth opens live S-band observability
- representative node-`5` curated summary visibility is observed only after
  auth opens the gate
- one representative bounded detailed readback,
  `EPS_GET_STATUS -> EPS_IBAT`, remains available on the authenticated
  node-`5` path without reopening broad live chatter
- bounded cached readback such as `GET_RESET_CAUSE` also remains available on
  the authenticated node-`5` path
- primary-band reconfiguration or autonomous failover closes old node-`5` live
  observability and requires fresh UHF secure-auth before later UHF
  high-authority traffic
- this proof now covers both the live gate boundary and the first node-`5`
  tier-selection slice plus the residual-governance cleanup; it does not claim
  that every diagnostics-only residual channel has been removed from runtime
  output, only that the current baseline now classifies those surfaces
  explicitly

Use the older `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash
scripts/run_rpi_target_recovery_restart_probe.sh` comparator only for retained
legacy authenticated-session (`SESSION_OPEN`) paths. Do not use it as the
current gate for secure-auth-based command proofs.

This current preflight is intentionally narrower than the full secure-auth
proof. It gates:

- installed-release bundled-keystore provenance
- S-band APID `0x00FE` secure-auth bootstrap
- one current secure command v2 round-trip (`GET_RESET_CAUSE`)

Its malformed-handshake observation is advisory only. Keep the full
`run_target_secure_auth_proof.sh` wrapper as the formal fail-closed proof.

Acceptance for `run_target_secure_auth_proof.sh` is:

- installed release `current` is a symlink under `/home/operator/obc-deploy`
- `obc-comm-csp-stack.service` has `WorkingDirectory=/home/operator/obc-deploy/current`
- bundled `config/security/command-auth.ini` SHA matches the repo keystore
  and the release manifest
- target service environment has expected
  `TARGET_COMM_PROFILE=sband`, `COMM_CSP_NODE=5`, and
  `COMMAND_AUTHORITY_PROFILE=sband-primary`
- forbidden `COMMAND_AUTH_*` service environment and `--command-auth*` CLI
  injection are absent
- S-band malformed handshake fails closed
- S-band APID `0x00FE` auth succeeds and secure command v2 succeeds on the
  command APID
- first accepted S-band secure command may use non-`1` sequence; the next
  accepted command must be strict next-sequence and duplicate sequence is
  rejected
- S-band `.sequence-staging/<leaf>` upload is admitted after secure auth
- physical node-`6` UHF secure auth succeeds with `ServiceID=2`
- `uhf-backup` accepts read/status secure command, denies high-authority
  command, and denies staged upload
- failover or other UHF-primary role re-entry invalidates old UHF auth/session
  state and requires UHF re-auth before secure-command acceptance
- wrapper prints `target-secure-auth-proof-v1: PASS`
- `diagnostics/cleanup-status.json` reports `PASS`
- local post-run process scan shows no proof-owned `fprime-gds`,
  `fprime-cli`, `ground_ttc_gateway`, or secure-auth probe residue

The proof root preserves `secure-auth-keystore-provenance.json`, gateway byte
captures, target journal snapshots, `checkpoints.jsonl`,
`secure-auth-proof-summary.json`, and cleanup status. This proof does not
claim UHF primary staged-upload success, encryption, RF, boot-trust expansion,
hardware-backed key storage, one-GDS aggregation, one-gateway simultaneous
multiplexing, or legacy v1 retirement.

## Target Payload Canonical `.fdp` Boundary

Historical hosted payload end-to-end closure remains archived at:

```bash
bash scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh
```

Do not treat that hosted wrapper as the current maintained payload authority.
It remains reviewable ancestry only and historically proved:

- `PayloadOpsController -> DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink`
- stock `BUILD_CATALOG` plus `START_XMIT_CATALOG(NO_WAIT)` ownership for the
  canonical payload `.fdp`
- GDS-received `.fdp` family byte-match plus repo-owned decode and JPEG extraction
  parity on the hosted node-`5` path

The governed target payload wrapper is now:

```bash
bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh
```

Current operator rule on the target baseline:

- Do not cite `bash scripts/run_payload_target_backend_hardening_v1_target_probe.sh`
- default node-`5` stock file/downlink now keeps the same
  `DpCatalog -> CommController -> FileDownlink` ownership while the subsystem
  `sband_comm_csp_node` now prefers `39 DOWNLINK_CONTROL_V3` plus
  `40 DOWNLINK_DATA_V3` below `GroundLinkDriver`; if the node-`5` v3 probe is
  unavailable it falls back directly to `31 DOWNLINK_WRITE`
- node-`5` `DOWNLINK_CONTROL_V3(STATUS)` success means bytes are accepted or
  committed inside the subsystem COMM in-memory stage/commit/drain path, not
  that the external S-band relay has already flushed every byte to
  `ground_ttc_gateway` or GDS
  as target payload official downlink closure. It is a Pi-local direct payload
  backend proof, not a governed node-`5` COMM-backed payload `.fdp` family proof.
- Do not cite `bash scripts/run_comm_csp_socketcan_file_downlink_probe.sh` as
  payload closure. It proves generic official file/downlink behavior on the
  target path, not payload capture promotion into a canonical payload `.fdp`
  family.

Current minimum accepted target payload claim in this runbook is:

- target payload capture occurs on the active target backend
- local `PIC%02X.bin` raw plus `PIC%02X.jpg` preview are written under the
  governed target payload capture root
- canonical preview payload `.fdp` family is written under the target
  official `data-products/` root, and raw `.fdp` promotion remains a separate
  explicit command step
- stock `BUILD_CATALOG` plus `START_XMIT_CATALOG(NO_WAIT)` selects that exact
  payload `.fdp` family on the governed node-`5` path
- the GDS-received `.fdp` family byte-matches the target source family
- repo-owned decode tooling extracts the payload header and the selected
  preview or raw artifact bytes, validates preview JPEG shape, and
  hash-matches the extracted artifact against the target source file
- the wrapper exits without proof-owned local or target process residue
- the bounded target proof claim is currently:
  - governed `vga` preview downlink PASS
  - governed `vga` raw downlink PASS
  - target-local `hd` preview capture plus payload `.fdp` decode/hash truth
  - bounded `full` raw deferred rejection with `PRESULT_STORAGE_FAILED detail 24`

The current final closeout reference is the 2026-07-12 live-capture proof at
`/private/tmp/node5-v3-pr1-official-target-final`. It used installed release
`v0.1.0-232-g4f6a6ad88`, completed the selected VGA preview/raw catalog in
`204.929 s`, received the final GDS slice at `204.752 s`, byte-matched the
preview and ten-slice raw families, and left both A and B at
`READY/no-action-needed`. C only verified the A-owned scoped COMM CAN FD
profile; it did not restart or remove the shared profile.

This runbook still does **not** claim:

- governed target `hd` or larger raw payload-family downlink closure
- payload-family reliable-transfer widening
- raw-register closure
- a physically switched EPS camera rail
- nonquiet UHF runtime stability

Historical bounded quiet node-`6` reliable-transfer proof:

```bash
ALLOW_HISTORICAL_WRAPPER=1 \
bash scripts/run_comm_csp_socketcan_uhf_reliable_transfer_probe.sh
```

If the target/subsystem transport baseline was just reset, or if stale helper
processes or prior COMM errors were observed before the proof, first rerun the
governed command-path comparator:

```bash
bash scripts/run_target_secure_auth_command_path_probe.sh
```

Only review this quiet node-`6` proof as an exact historical diagnostic slice
after that comparator passes on the same service baseline.

Historical bounded quiet node-`6` beacon suppress/runtime proof:

```bash
ALLOW_HISTORICAL_WRAPPER=1 \
bash scripts/run_target_can_uhf_beacon_suppression_probe.sh
```

This wrapper is also retained only for exact historical compatibility review.
Do not treat it as a current maintained secure-auth closeout gate.

Acceptance for that wrapper is:

- default node-`5` secure-auth bootstrap succeeds first
- the probe uses the older explicit-switch path before judging UHF
  reliable-transfer truth
- quiet switched node-`6` secure-auth succeeds on the governed UHF path
- `BUILD_CATALOG` plus `START_XMIT_CATALOG(NO_WAIT)` selects one official HK
  `.fdp` source file
- target journal shows `COMM_RT_ROUTE_SELECTED`,
  `COMM_RT_TRANSFER_STARTED`, bounded resend/progress activity as observed,
  and `COMM_RT_FINAL_RESULT`
- the node-`6` receiver writes the byte-matching `.fdp` under
  `/tmp/comm-reliable-transfer-node6`
- stock GDS file storage stays empty for the reliable-transfer path
- the wrapper removes `53-obc-reliable-transfer-admission.conf`,
  `52-reliable-transfer-output.conf`, clears probe-owned quiet override, and
  restarts services back to the normal non-quiet baseline before exit

This probe proves the exact historical quiet switched node-`6`
reliable-transfer slice only. It does not promote nominal non-quiet UHF
reliable-transfer behavior into the maintained operator baseline.

## Historical Command-Path Preflight Helper

`run_rpi_target_recovery_restart_probe.sh` remains available, but current
baseline usage is limited to historical/supporting command-path preflight:

```bash
PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh
```

This mode uses the same installed `obc-comm-csp-stack.service` path, but it is
now a target command-path preflight rather than an ADCS-induced restart proof
or a current Route `3` closure surface.

Acceptance for this focused path is:

- the secure-auth command path returns to a passing bounded baseline on the
  active node-`5` target COMM profile
- OBC and subsystem services stay `active` through the preflight run
- the preflight remains a command-path sanity gate, not a current ADCS
  service-managed restart proof

This is a target command-path preflight. It is not the current ADCS first-fault
recovery proof, not the current EPS `R3/R5` proof, not hardware watchdog
reset, not Linux reboot, not bootloader/partition handoff, RF behavior, or
final flight deployment proof.

Current Route `3` target closure should instead use:

```bash
bash scripts/chapter5_routes/target/route3_adcs_r3_first_fault_and_clear.sh
bash scripts/chapter5_routes/target/route3_eps_r3_safe_fallback.sh
bash scripts/chapter5_routes/target/route3_watchdog_reboot_and_postcheck.sh
bash scripts/chapter5_routes/target/run_route3_target.sh
```

## Hardware Watchdog Capability Gate

`target-hardware-watchdog-reset-proof-v1` also adds a non-destructive target
capability gate:

```bash
bash scripts/run_rpi_target_hardware_watchdog_capability_probe.sh
```

The gate verifies the active Raspberry Pi baseline can safely use
`bcm2835-wdt` under the current service lifecycle:

- `/dev/watchdog0` and `/sys/class/watchdog/watchdog0` are present
- the driver identity is `bcm2835-wdt`
- timeout is fixed at `15` seconds and `SETTIMEOUT` is unsupported
- `nowayout=0`
- open, repeated keepalive, close, and immediate reopen succeed
- restarting `obc-comm-csp-stack.service` does not reboot the board

This gate is a lifecycle compatibility proof, not the board-reset proof itself.

## Hardware Watchdog Reset Probe

The repo-owned target board-reset proof is:

```bash
bash scripts/run_rpi_target_hardware_watchdog_reset_probe.sh
```

The probe uses the same installed `obc-comm-csp-stack.service` path, confirms
`LinuxWatchdogSink` has opened `/dev/watchdog0`, then uses the bounded
`SET_WATCHDOG_PROBE_SUPPRESSION` proof trigger to let a real watchdog-source
incident stop stroking the Raspberry Pi hardware watchdog. The acceptance
boundary is:

- watchdog device is owned by the active OBC service before the trigger
- watchdog-source suppression first enters shared recovery at
  `R2_RESTART_SOFTWARE_COMPONENT`
- SSH disconnects and reconnects
- the target boot marker changes
- `obc-comm-csp-stack.service` returns to `active`
- boot metadata reports `RECOVERY_WATCHDOG`, `WATCHDOG_ADCS_FDIR`, and
  `R6_OBC_REBOOT`
- persistent fault readback contains both reboot-pending and boot-ack truth
- post-reboot secure-auth re-bootstrap succeeds and bounded
  `GET_RESET_CAUSE` readback succeeds on the maintained command path
- fresh post-reboot `GET_HW_WATCHDOG_STATUS` readback confirms the hardware
  watchdog device reopened with timeout `15`
- fresh post-reboot `GET_WATCHDOG_STATUS` readback confirms aggregate watchdog
  health plus per-source status returned to `HEALTHY`
- fresh post-reboot `GET_PERSISTENT_FAULT_HISTORY` readback confirms
  `REBOOT_PENDING` and `RECOVERY_BOOT_ACK` truth for the same
  `WATCHDOG_ADCS_FDIR / R6_OBC_REBOOT` sequence

Precondition: the active COMM lab command path must already be healthy. In
practice that means `obc-lab-can.service` is active and `obc.local:can0`
exists/up, because the proof trigger and post-reboot readback both traverse the
governed COMM lab path. If the target shows `mcp251xfd` probe/CRC failures,
`can0` is missing or down, or `obc-comm-csp-stack.service` is already stuck in
repeated SocketCAN preflight failures, stop and treat that as a target
environment blocker rather than a watchdog-proof failure.

This probe is the repository-owned Raspberry Pi hardware watchdog board-reset
proof for the active `TopCcsds` target package. It uses a probe-owned temporary
quiet egress override plus journal-first acceptance to keep the pre-trigger
governed command path stable enough for the bounded proof. That does **not**
promote quiet mode into a normal operator baseline, and it does **not** claim
that bounded background-TM CCSDS serial stability is solved in general. The
scope remains narrower than power-loss recovery, external supervisor IC proof,
bootloader/partition handoff, RF behavior, or final flight deployment proof.

Current target baseline readiness also removes stale proof-owned watchdog
drop-ins before declaring the machine ready. The maintained baseline should
therefore come up with `HARDWARE_WATCHDOG=linux-device` active rather than
carrying a leftover disabled-watchdog override from an older probe run.

## Rollback

To roll the OBC target back to the older installed service:

```bash
ssh operator@obc.local 'sudo systemctl disable --now obc-comm-csp-stack.service'
ssh operator@obc.local 'sudo systemctl enable --now obc-installed-stack.service'
```

To stop the subsystem lab services:

```bash
ssh operator@subsystem.local 'sudo systemctl disable --now subsystem-sband-csp-stack.target subsystem-uhf-csp-stack.target subsystem-eps-csp.service subsystem-adcs-csp.service subsystem-sband-csp.service subsystem-uhf-csp.service'
```

To stop transitional CAN provisioning:

```bash
ssh operator@obc.local 'sudo systemctl stop obc-lab-can.service'
ssh operator@subsystem.local 'sudo systemctl stop subsystem-eps-adcs-lab-can.service subsystem-comm-lab-can.service'
```

Rollback does not remove installed unit files; it only changes what is enabled/running.

## Not Covered

This lab operational baseline does not claim:

- final flight deployment identity or OS provisioning
- RF behavior or vendor radio control plane
- no-preamble first-byte-clean serial acquisition
- packet-loss-tolerant retransmission
- arbitrary file downlink beyond the bounded official data-product acceptance set
- ScenarioBridge or pass automation
