# Chapter 5 Route Worktree

This directory owns the current Chapter 5 integrated-route redesign on the
active repository baseline. It does not revive the historical
`mode/ttc/payload-ttc/comm-ttc` probe families.

## Layout

- `eps_sim_control.py`
  - repo-owned helper for the EPS simulator runtime SoC control socket
- `lib/`
  - shared helpers for future hosted/target route probes
- `hosted/`
  - hosted route wrappers, staged scripts, or hosted command playbooks
- `target/`
  - target A/B/C-governed route wrappers, staged scripts, or target command
    playbooks

## Current Staged Route Plan

### Route 1

Preferred staged split:

- `hosted/route1_prepare_and_capture`
- `hosted/route1_soc_fallback_check`
- `target/route1_prepare_and_capture`
- `target/route1_soc_fallback_check`

Auth-family note:

- `route1_prepare_and_capture` now points at the hosted current secure-auth /
  direct `AUTO` payload path plus official `SEQ_VALIDATE` / `SEQ_RUN`
  deterministic payload family, followed by preview-only `dpCatalog`
  downlink of the deterministic sequence preview
  - the hosted wrapper starts both the current node-`5` S-band and node-`6`
    UHF baseline surfaces even though the Route 1 command/data flow remains on
    S-band; omitting node `6` allows required COMM health traffic to starve
    the node-`5` downlink queue
  - aggregate wrapper evidence and each stage evidence use separate probe
    roots, so a stage cleanup cannot erase the aggregate summary
  - target aggregate wrappers use the same aggregate/stage root separation
    for retained A/B/C evidence
  - each target stage keeps wrapper-owned `baseline/` and `summary.log` at
    the stage root while a cleanup-owning C probe runs under `scenario/`
- target `route1_prepare_and_capture` now points at the governed node-5
  sequence-driven payload path:
  - apply current manual auth preflight
  - externally raise EPS SoC
  - refresh `EPS_GET_STATUS`
  - confirm `IDLE -> PAYLOAD`
  - upload / validate / run the official payload sequence
  - corroborate `PAYLOAD_CAPTURE_METADATA`
  - close preview-only deterministic `dpCatalog` downlink
  - reports failure if either required postflight baseline check fails; a C
    functional PASS is not a complete governed Route 1 PASS by itself
- `route1_soc_fallback_check` is auth-neutral local runtime control
- target `route1_soc_fallback_check` reuses the same current target auth /
  preflight baseline assumptions as the first stage before driving SoC
  fallback, and similarly requires its postflight A/B checks to pass
- `route1_downlink_and_extract` is retained adjacent hosted file/downlink
  lineage and must not be cited as hosted secure-auth proof by itself

Expected observability:

- authenticated command/readback
- official sequence validate/run on the current payload public surface
- payload capture success for one VGA `AUTO` preview on the direct current
  path and one VGA `DETERMINISTIC` preview on the official sequence path
- target current Route 1 sequence probe implementation:
  - `scripts/comm_verification/lib/run_route1_sequence_payload_target_probe.py`
- live SoC reduction while the stack stays online
- `PAYLOAD -> IDLE` or `PAYLOAD -> SAFE` policy outcome as appropriate
- preview-only payload `.fdp` receipt plus byte/hash parity after extraction
  for the deterministic sequence preview slice

Repo-owned operator/demo asset:

- `scripts/manual_ops/examples/route1-demo.seq`
  - mirrors the current Route 1 sequence truth for manual compile / upload /
    `SEQ_VALIDATE` / `SEQ_RUN`
  - intended for Mission Console or manual dual-GDS demos after external
    SoC raise and `PAYLOAD` mode entry

### Route 2

Preferred staged split:

- `hosted/route2_mode_ttc_entry`
- `hosted/route2_link_recovery_and_hk`
- target equivalents with nonquiet-first and quiet-fallback handling

Auth-family note:

- `route2_mode_ttc_entry` is auth-neutral hosted runtime control
- `route2_link_recovery_and_hk` reuses hosted dual-link / HK evidence and must
  be cited separately from hosted secure-auth ancestry

Expected observability:

- `SAFE -> HELL -> SAFE -> IDLE -> TTC`
- TTC auto-entry
- ADCS mode readback showing `POINTING`
- bounded node-5 loss and UHF continuity on the currently supported path
- HK `.fdp` receipt

### Route 3

Preferred staged split:

- `hosted/route3_recovery_chain_pre_reboot`
- `target/route3_recovery_chain_pre_reboot`
- `target/route3_watchdog_reboot_and_postcheck`

Auth-family note:

- hosted Route 3 remains auth-neutral pre-reboot behavior only
- hosted Route 3 current wrappers reuse scoped `run_recovery_executors_v1_probe.sh`;
  the older `run_multi_subsystem_fdir_v1_probe.sh` closure is retained only as
  historical evidence lineage, not as a maintained route driver
- target Route 3 current closure uses secure-auth post-restart / post-reboot
  command-path readback and must not cite legacy `SESSION_OPEN` truth

Expected observability:

- hosted: watchdog-source `R2`, ADCS `R3`, EPS `R3/R5`
- target: current Route 3 target closure now has fresh ADCS `R3`, EPS `R3/R5`,
  and watchdog `R6` reruns on the corrected host-role and COMM-CAN baseline
- target ADCS uses simulator runtime state-drop injection; target EPS uses
  simulator runtime `drop-status` injection so the proof does not over-escalate
  itself from legitimate `R3/R5` into `R6`

## Governance Rules

- Hosted routes should reuse the maintained hosted stock-stack or dual-GDS
  baseline instead of private stack launch logic whenever possible.
- Target routes must follow A/B/C ownership:
  - A: `scripts/ensure_target_comm_lab_baseline.sh`
  - B: `scripts/ensure_ground_dual_gds_baseline.sh`
  - C: Chapter 5 route logic only
- A route may be closed by multiple scripts or by a script plus a repo-owned
  playbook when one monolithic script would make verdict ownership unclear.
- Mission Console `packet-lab negative evidence` is not part of the Chapter 5
  verdict boundary unless a later route explicitly tests that oracle.

## Current Implementation Status

- Runtime EPS SoC control surface: implemented
- TTC-triggered ADCS `POINTING` hook: implemented
- New Chapter 5 probe tree: created
- Current wrappers now separate hosted current secure-auth, auth-neutral, and
  retained legacy compatibility families
- Target Route 3 current path now uses secure-auth-only post-restart and
  post-reboot readback
- Target Route 3 current reruns now pass with:
  - ADCS simulator runtime drop-state
  - EPS simulator runtime `drop-status`
  - secure-auth resend-readback watchdog re-bootstrap

Until the route wrappers are fully implemented, this README is the canonical
repo-tracked playbook for how the staged closures are expected to split.
