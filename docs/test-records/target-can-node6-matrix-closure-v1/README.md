# target-can-node6-matrix-closure-v1 Evidence

Current-note:

- this record remains valid for quiet-UHF CAN carrier, file, sequence, and
  failover closure on the governed target baseline
- any `SESSION_OPEN` wording in the retained May 2026 artifacts is historical
  auth-boundary wording and must not be cited as current secure-auth truth
- current maintained secure-auth truth for target node-`5` / node-`6` command
  bootstrap belongs to the secure-auth family and Chapter 5 reruns, not to
  this older matrix record alone
- explicit `COMM_SET_ACTIVE(UHF)` in this record is historical quiet-path
  compatibility evidence, not current autonomous failover truth
- do not reuse the embedded matrix cases here as current Chapter 5 route
  templates without re-checking the current registry and A/B/C governance

Date:
- `2026-05-22`

OpenSpec change:
- `target-can-node6-matrix-closure-v1`

## Scope

This record covers the current branch work for the remaining target CAN matrix
cells tied to:

- target CAN `sband-sequence-subsystem`
- target CAN `uhf-primary-file`
- target CAN `uhf-primary-sequence-subsystem`
- target CAN `failover-command`

The work reuses the current target/lab CAN topology:

- macOS `fprime-gds`
- macOS `ground_ttc_gateway`
- `subsystem.local` node `5` S-band COMM, node `6` UHF COMM, EPS, and ADCS
- `obc.local` OBC over shared SocketCAN

This record does not claim:

- non-quiet UHF stability under background telemetry
- target TCP parity
- direct-control closure for target CAN

## Implemented Harness

Branch-local matrix ownership now includes:

- dedicated target CAN helper:
  [scripts/comm_verification/lib/run_target_can_matrix_probe.py]($REPO_ROOT/scripts/comm_verification/lib/run_target_can_matrix_probe.py)
- dedicated target CAN case entrypoints:
  - [scripts/comm_verification/cases/sband-sequence-subsystem.sh]($REPO_ROOT/scripts/comm_verification/cases/sband-sequence-subsystem.sh)
  - [scripts/comm_verification/cases/uhf-primary-file.sh]($REPO_ROOT/scripts/comm_verification/cases/uhf-primary-file.sh)
  - [scripts/comm_verification/cases/uhf-primary-sequence-subsystem.sh]($REPO_ROOT/scripts/comm_verification/cases/uhf-primary-sequence-subsystem.sh)
  - [scripts/comm_verification/cases/failover-command.sh]($REPO_ROOT/scripts/comm_verification/cases/failover-command.sh)

The helper now owns:

- probe-owned runtime roots and copied artifacts
- ground-side GDS/gateway orchestration
- official sequence generation and same-path upload
- official `.fdp` source snapshotting and byte-match checks
- target-side quiet override handling
- subsystem service restart hygiene

## Passing Cell

Command:

```bash
COMM_VERIFICATION_ENV=rpi_can \
COMM_VERIFICATION_CASES=sband-sequence-subsystem \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260522T053648Z`

Observed result:

- `sband-sequence-subsystem`: `PASS`
- carrier: `sband-tcp`
- wrapper intent: `dedicated-governed`

Passing markers:

- historical then-current authenticated S-band `SESSION_OPEN`
- same-path sequence ingress to `.sequence-staging`
- `FileHandling.fileUplink.FileReceived`
- `SEQ_VALIDATE` non-reject preflight
- `SEQ_RUN(..., WAIT)` completion with:
  - `EPS_GET_STATUS`
  - `ADCS_GET_ATTITUDE`
  - `CS_SequenceComplete`

## Additional Passing Cells

### target CAN `uhf-primary-file`

Command:

```bash
COMM_VERIFICATION_ENV=rpi_can \
COMM_VERIFICATION_CASES=uhf-primary-file \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260522T141125Z`

Observed result:

- `uhf-primary-file`: `PASS`
- carrier: `uhf-physical-uart`
- wrapper intent: `dedicated-governed`

Passing markers:

- installed `subsystem-uhf-csp.service` baudrate was followed at `9600`
- node-`5` bootstrap and quiet-UHF startup checkpoints were recorded under the
  probe-owned diagnostics tree
- explicit switch to UHF primary and historical authenticated node-`6` session
  use stayed within the governed helper contract
- official `.fdp` source snapshots on target OBC byte-matched the
  GDS-received downlink artifact through the physical UHF UART path

### target CAN `uhf-primary-sequence-subsystem`

Command:

```bash
COMM_VERIFICATION_ENV=rpi_can \
COMM_VERIFICATION_CASES=uhf-primary-sequence-subsystem \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260522T141314Z`

Observed result:

- `uhf-primary-sequence-subsystem`: `PASS`
- carrier: `uhf-physical-uart`
- wrapper intent: `dedicated-governed`

Passing markers:

- quiet-UHF node-`6` sequence upload completed on the same UHF path
- `FileHandling.fileUplink.FileReceived`
- `SEQ_VALIDATE` non-reject preflight
- `SEQ_RUN(..., WAIT)` completion with:
  - `EPS_GET_STATUS`
  - `ADCS_GET_ATTITUDE`
  - `CS_SequenceComplete`

### target CAN `failover-command`

Command:

```bash
COMM_VERIFICATION_ENV=rpi_can \
COMM_VERIFICATION_CASES=failover-command \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/rpi_can/20260522T141428Z`

Observed result:

- `failover-command`: `PASS`
- carrier: `uhf-physical-uart`
- wrapper intent: `dedicated-governed`

Passing markers:

- historical authenticated node-`5` S-band session and pre-failover command
  success
- explicit `COMM_SET_ACTIVE(UHF)` transition to node-`6` primary
- bounded node-`5` loss handling
- quiet node-`6` historical session reopen and post-failover command
  continuity

## Diagnostics Outcome

The branch-local target CAN helper now emits probe-owned diagnostics for:

- installed service snapshots and effective baudrate selection
- node-`5` bootstrap readiness
- node-`6` service readiness
- session-open visibility per ground path
- sequence ingress and file/downlink checkpoints

These diagnostics were added to make later physical-UHF regressions easier to
classify without expanding the quiet-UHF claim boundary.

## Follow-On Impact

Current branch truth:

- target CAN `sband-sequence-subsystem`, `uhf-primary-file`,
  `uhf-primary-sequence-subsystem`, and `failover-command` are formally closed
  on the current quiet-UHF lab baseline
- this change does not require hardware-suspect quarantine because the
  physical target CAN UHF family now passes its governed matrix cells
- umbrella closeout can now treat target CAN UHF closure as passing evidence
  and limit remaining blockers to the shared direct target path plus the
  node-`6` same-path ingress family outside CAN
