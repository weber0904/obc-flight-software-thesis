# target-direct-control-matrix-cases-v1 Evidence

Date:
- `2026-05-22`
- `2026-05-23`

OpenSpec change:
- `target-direct-control-matrix-cases-v1`

## Scope

This record covers the final target `direct-control` matrix cells:

- target TCP `direct-control`
- target CAN `direct-control`

These wrappers intentionally prove only the isolated
`fprime-cli -> GDS -> OBC` target path. They do not claim:

- target TCP southbound parity closure
- target CAN S-band or UHF satcom closure
- target physical UHF UART provenance

## Implemented Wrappers

Branch-local matrix ownership now includes a dedicated target direct helper:

- helper:
  [scripts/comm_verification/lib/run_target_direct_matrix_probe.py](../../../scripts/comm_verification/lib/run_target_direct_matrix_probe.py)
- matrix case entrypoint:
  [scripts/comm_verification/cases/direct-control.sh](../../../evidence/verification-path-registry.md)

The helper keeps the direct path isolated from satcom launchers by:

- starting local `fprime-gds` and direct-control listeners under case-owned
  roots
- keeping target TCP subsystem realism on `subsystem.local` with only
  `eps_simulator(node 2)` and `adcs_simulator(node 3)` over ZMQHUB
- keeping target CAN subsystem realism on `subsystem.local` with only
  `eps_simulator(node 2)` and `adcs_simulator(node 3)` on shared SocketCAN
- launching `OBC` directly toward `fprime-gds` without `ground_ttc_gateway`,
  node `5`, or node `6`

The bounded cleanup checks on the current branch also confirmed:

- no repo-root `.sequence-staging`, `.sequence-admitted`, `.adm-*`, or
  `.stg-*` residue after the failed reruns
- no leftover local `fprime-gds`, `run_env_matrix.sh`,
  `run_target_direct_matrix_probe.py`, or dedicated `csp_zmqproxy` processes
  after the failing runs exited

## Target TCP Direct-Control

Command:

```bash
COMM_VERIFICATION_ENV=rpi_tcp \
COMM_VERIFICATION_CASES='direct-control' \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact roots:

- first clean pass:
  `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260523T100016Z`
- post-isolation sequential rerun:
  `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260523T100654Z`

Observed result:

- `direct-control`: `PASS`
- carrier: `direct-control`
- wrapper intent: `dedicated-governed`
- blocker class: none

Closure findings on the current branch:

- the dedicated wrapper now reuses the registered target direct `OBC -> GDS`
  path rather than falling back to a blocker stub, and it suspends the
  installed target stack service while the probe-owned runtime is active
- the direct helper now records a reviewable connection timeline, launcher
  args, a historical registered direct-path checklist, and per-command raw
  contract artifacts alongside each run
- the target OBC launcher now correctly supports `COMMAND_AUTH=disabled`,
  `COMMAND_AUTHORITY_PROFILE=dev-direct`, and `HEADLESS=1`
- the direct rerun now exercises two bounded plain-command comparators:
  - `OBCApp.modeManager.MODE_GET` (`0x10030001`)
  - `OBCApp.bootManager.GET_RESET_CAUSE` (`0x10038006`)
- the first narrowed blocker turned out to mix two non-product causes:
  - the helper was incorrectly forcing local `fprime-gds` to use historical
    connectivity-only `fprime` framing instead of the active `TopCcsds`
    direct-uplink CCSDS framing truth
  - the remote workspace no longer had a built
    `build-fprime-automatic-native/bin/Linux/OBC`, so bounded reruns could stop
    before the target runtime ever reached `GROUND_LINK_UP`
- after correcting the helper to use
  `space-packet-space-data-link(scid=68, vcid=1, frame-size=1024)` and
  rebuilding the remote workspace, the dedicated target TCP direct wrapper now
  proves:
  - local `fprime-gds` accepts the direct target TCP connection
  - target OBC reports `GROUND_LINK_UP`
  - target OBC remains alive in `Runtime mode: headless`
  - both bounded plain commands complete with target-visible `OpCodeCompleted`
  - the immediate sequential rerun reproduces the same result
- one bounded parallel rerun on
  `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260523T100156Z`
  failed before direct bring-up because two local `fprime-gds` instances tried
  to create the same repo-root `logs/.../custom-data-handlers-app` path in the
  same second. That rerun was a local probe-isolation failure, not a target TCP
  product regression.

Current interpretation:

- target TCP `direct-control` is now formally closed
- the passing matrix-owned wrapper proves the dedicated target
  `fprime-cli -> GDS -> OBC` path on the governed direct adapter topology
- the resolved causes were helper contract drift and missing remote workspace
  build output, not a surviving target direct-ingress product failure

## Target CAN Direct-Control

Command:

```bash
COMM_VERIFICATION_ENV=rpi_can \
COMM_VERIFICATION_CASES='direct-control' \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact roots:

- first clean pass:
  `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260523T100103Z`
- post-isolation rerun:
  `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260523T100729Z`

Observed result:

- `direct-control`: `PASS`
- carrier: `direct-control`
- wrapper intent: `dedicated-governed`
- blocker class: none

Closure findings on the current branch:

- the dedicated wrapper now launches EPS/ADCS-only subsystem realism on shared
  SocketCAN and keeps the ground path fully direct
- the direct helper now reuses the same corrected CCSDS GDS framing contract as
  target TCP, records the same raw-command comparator artifacts, and keeps the
  ground path fully direct while EPS/ADCS remain online on shared SocketCAN
- with the same helper framing fix and rebuilt remote workspace output, the
  target CAN direct wrapper now proves:
  - local `fprime-gds` accepts the direct TCP adapter session
  - target OBC reports `GROUND_LINK_UP`
  - target OBC remains alive in `Runtime mode: headless`
  - bounded plain `MODE_GET` and `GET_RESET_CAUSE` complete with
    target-visible `OpCodeCompleted`
  - the immediate rerun reproduces the same result

Current interpretation:

- target CAN `direct-control` is now formally closed
- the same corrected helper contract proves that the remaining failure family
  was shared helper/workspace drift, not a CAN-specific target direct product
  regression

## Follow-On Impact

Current branch truth:

- matrix-owned target TCP and target CAN direct wrappers now exist
- both wrappers are isolated from satcom-specific helpers and leave no
  checkout-level residue
- both target direct-control cells now pass with governed matrix-owned wrappers
- the direct helper must preserve the active `TopCcsds` CCSDS uplink framing
  contract on the direct target path; historical `fprime`-framed evidence from
  `rpi-target-integration-v1` remains connectivity-only evidence and must not
  be reused as the command-dispatch contract
- target TCP and target CAN direct-control are now ready for registry reuse as
  dedicated matrix-owned `fprime-cli -> GDS -> OBC` target evidence
