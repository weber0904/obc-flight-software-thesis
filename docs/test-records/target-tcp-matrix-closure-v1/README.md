# target-tcp-matrix-closure-v1 Evidence

Current-note:

- this record remains valid for target TCP development-carrier transport,
  failover, file, and sequence closure
- any `SESSION_OPEN` wording below belongs to the retained historical auth
  boundary in the May 2026 artifacts and must not be cited as the current
  maintained secure-auth truth
- use the secure-auth family and fresh Chapter 5 Route 3 reruns when the claim
  is specifically about current target auth bootstrap/re-auth semantics

Date:
- `2026-05-22`
- `2026-05-23`

OpenSpec change:
- `target-tcp-matrix-closure-v1`

## Scope

This record covers the target TCP high-level matrix cells layered on top of the
governed three-host parity launcher:

- `sband-sequence-subsystem`
- `uhf-primary-sequence-subsystem`
- `failover-command`

The governed development-carrier topology remains:

- `macOS`: `csp_zmqproxy`, `fprime-gds`, `ground_ttc_gateway`, ground-side
  listeners, and file stores
- `obc.local`: `OBC` only
- `subsystem.local`: `sband_comm_csp_node(node 5)`,
  `uhf_comm_csp_node(node 6)`, `eps_simulator(node 2)`,
  `adcs_simulator(node 3)`

This record does not claim:

- target direct-control closure
- target physical UHF UART provenance

## Implemented Harness

Branch-local matrix ownership now extends the target TCP parity helper:

- dedicated target TCP helper:
  [scripts/comm_verification/lib/run_target_tcp_matrix_probe.py]($REPO_ROOT/scripts/comm_verification/lib/run_target_tcp_matrix_probe.py)
- target TCP case entrypoints:
  - [scripts/comm_verification/cases/sband-sequence-subsystem.sh]($REPO_ROOT/scripts/comm_verification/cases/sband-sequence-subsystem.sh)
  - [scripts/comm_verification/cases/uhf-primary-sequence-subsystem.sh]($REPO_ROOT/scripts/comm_verification/cases/uhf-primary-sequence-subsystem.sh)
  - [scripts/comm_verification/cases/failover-command.sh]($REPO_ROOT/scripts/comm_verification/cases/failover-command.sh)

The helper now owns:

- official sequence generation on the governed parity launcher
- same-path node-`5` sequence upload, validate, run, and subsystem round-trip
- target TCP failover continuity with explicit node-`5` loss and node-`6`
  session reopen
- bounded debug instrumentation for node-`6` sequence ingress, including:
  - prewarmed UHF ground/API before switching to `UHF primary`
  - a low-level start/data/end file-packet fallback sender
  - raw packet and launcher contract audit artifacts
  - quiet-path sequence same-path proof via inner opcode completion instead of
    `channels --search`

## Foundation Recovery

Command:

```bash
COMM_VERIFICATION_CASES='csp-reachability' \
TARGET_TCP_NODE5_READY_TIMEOUT_SEC=45 \
bash scripts/comm_verification/matrix/run_env_matrix.sh rpi_tcp
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260523T081132Z`

Observed result:

- `csp-reachability`: `PASS`
- carrier: `internal-csp-rpi-tcp-parity`
- wrapper intent: `dedicated-governed`

Recovery notes:

- the earlier branch-local regression was not a generic target TCP transport
  failure:
  - Linux-only `csp_zmqproxy + csp_service_peer + csp_runtime_smoke` on
    `obc.local` passed
  - Darwin-hosted `csp_zmqproxy` plus local Darwin `csp_runtime_smoke` could
    also reach remote Linux peers
- the useful narrowing came from a generic remote smoke sweep:
  - `subsystem.local` and `obc.local` both established raw TCP connections to
    the Darwin-hosted `csp_zmqproxy`
  - `obc.local` still failed when `csp_runtime_smoke` only waited `200ms`
    after `runtime.init()`
  - the same binary on `obc.local` passed once `--settle-ms 1000` was used
- that diagnosis led to two harness fixes instead of a product-side rollback:
  - target TCP probe now gives the subsystem stack a bounded startup settle
    window before starting OBC
  - target TCP OBC now runs with governed
    `--comm-subsystem-ping-timeout-ms 1000` and
    `--comm-primary-unavailable-failure-threshold 10`
- the first post-fix rerun still reported
  `timed out waiting for node 5 readiness`, but that failure was an oracle bug:
  the readiness gate still hard-coded the old
  `CSP ping node 5 success 1 timeout 500 ms` fragment after the harness had
  moved to `1000ms`
- once the readiness gate was updated to match the configured timeout, the
  governed target TCP parity foundation closed again on the same branch

## Passing Cells

### `sband-sequence-subsystem`

Command:

```bash
COMM_VERIFICATION_ENV=rpi_tcp \
COMM_VERIFICATION_CASES='sband-sequence-subsystem' \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T073659Z`

Observed result:

- `sband-sequence-subsystem`: `PASS`
- carrier: `sband-tcp`
- wrapper intent: `dedicated-governed`

Passing markers:

- historical then-current authenticated node-`5` `SESSION_OPEN`
- same-path sequence upload to `.sequence-staging`
- `SEQ_VALIDATE` non-reject preflight
- `SEQ_RUN(..., WAIT)` completion with:
  - `EPS_GET_STATUS`
  - `ADCS_GET_ATTITUDE`
  - `CS_SequenceComplete`

### `failover-command`

Command:

```bash
COMM_VERIFICATION_ENV=rpi_tcp \
COMM_VERIFICATION_CASES='failover-command' \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260522T075252Z`

Observed result:

- `failover-command`: `PASS`
- carrier: `uhf-tcp-southbound`
- wrapper intent: `dedicated-governed`

Passing markers:

- historical then-current authenticated node-`5` `SESSION_OPEN`
- pre-failover `GET_RESET_CAUSE` command completion on S-band
- explicit `COMM_SET_ACTIVE(UHF)` switch to node `6`
- bounded node-`5` loss with no post-loss S-band command completion
- historical authenticated node-`6` `SESSION_OPEN`
- post-failover `GET_RESET_CAUSE` command completion on UHF

### `uhf-primary-sequence-subsystem`

Command:

```bash
COMM_VERIFICATION_ENV=rpi_tcp \
COMM_VERIFICATION_CASES='uhf-primary-sequence-subsystem' \
COMM_NODE_INGRESS_DIAGNOSTICS=1 \
TARGET_TCP_UHF_SERIAL_PREAMBLE_LINES=0 \
TARGET_TCP_UHF_SERIAL_PREAMBLE_DELAY_MS=0 \
TARGET_TCP_DELAY_UHF_GROUND_UNTIL_SWITCH=1 \
TARGET_TCP_POST_UHF_SESSION_SETTLE_SEC=2 \
DIAGNOSTIC_QUIET_PACKET_EGRESS=1 \
TARGET_TCP_SKIP_API_SEQUENCE_UPLOAD=1 \
TARGET_TCP_MANUAL_SEQUENCE_SENDER_ORDER=raw-socket,pipeline \
bash scripts/comm_verification/matrix/run_env_matrix.sh rpi_tcp
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/rpi_tcp/20260523T081219Z`

Observed result:

- `uhf-primary-sequence-subsystem`: `PASS`
- carrier: `uhf-tcp-southbound`
- wrapper intent: `dedicated-governed`

Passing markers:

- explicit node-`5` `COMM_SET_ACTIVE(UHF)` transition to node `6`
- historical authenticated node-`6` `SESSION_OPEN`
- same-path sequence upload to `.sequence-staging/target-can-uhf-roundtrip.bin`
- `FILE_INGRESS_START_ACCEPTED`
- `FileReceived`
- `SEQ_VALIDATE` non-reject preflight
- `SEQ_RUN(..., WAIT)` completion with:
  - `EPS_GET_STATUS`
  - `ADCS_GET_ATTITUDE`
  - `CS_SequenceComplete`

Closure notes:

- the quiet-path blocker on `20260523T023829Z` was an oracle bug, not a
  product-path failure:
  - `CommEgressMux` with `diagnosticQuietPacketEgress` suppresses packet TM, so
    `channels --search EPS_SOC/ADCS_MODE` is not a valid success oracle on this
    bounded target TCP UHF path
  - the governed helper therefore proves same-path subsystem round-trip via
    inner `OpCodeCompleted` for `EPS_GET_STATUS` and `ADCS_GET_ATTITUDE`
- the later branch-local failures on `2026-05-23` were no longer node-`6`
  ingress failures; they were parity-foundation regressions caused by remote
  ZMQ hub settle timing and a stale node-`5` readiness log fragment
- once the target TCP foundation was reclosed, the same governed node-`6`
  sequence case again reached:
  - `FILE_INGRESS_START_ACCEPTED`
  - `FileReceived`
  - non-reject `SEQ_VALIDATE`
  - successful `SEQ_RUN(..., WAIT)`

## Follow-On Impact

Current branch truth:

- target TCP `csp-reachability` is formally reclosed on the governed
  development-carrier launcher
- target TCP `sband-sequence-subsystem` is formally closed
- target TCP `failover-command` is formally closed
- target TCP `uhf-primary-sequence-subsystem` is now formally closed on the
  governed development carrier
- target TCP still must not be read as physical UHF UART provenance
