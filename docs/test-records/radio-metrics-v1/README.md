# radio-metrics-v1 Evidence

## Scope

This record governs the `radio-metrics-v1` change that freezes the current
bounded COMM observability contract for the active hosted and target/lab
baselines.

Covered review surfaces:

- hosted S-band node-`5` current baseline path
- hosted UHF node-`6` bounded backup path
- hosted direct-TCP connected-only fallback path
- service-managed target/lab default node-`5` command/readback path

Frozen contract boundary:

- `GroundLinkDriver` owns raw ground-link observation only
- `GroundLinkHealthProvider` owns derived link-health / freshness reduction
- `CommController` owns policy-facing role, owner, and COMM FDIR runtime state
- `RadioController` owns cached raw radio observation, freshness, and
  unavailable-result semantics
- `UartDriver` keeps raw byte-stream counters through `ByteStreamStats`

## Covered

- raw per-band ground-link observation now includes `mode`,
  `healthSemantics`, `connected`, `txChunks`, `rxChunks`, `txBytes`,
  `rxBytes`, `txErrors`, `rxErrors`, and `successfulStatusObservations`
- cached radio observation now includes `haveSample`, `statusAgeTicks`,
  `lastResult`, and last successful raw `enabled`, `powerDbm`, `freqHz`,
  `temperatureC`, and `rssiDbm`
- hosted status readback prints raw ground-link observation, derived
  link-health state, policy-facing COMM state, and cached radio observation as
  separate layers
- active `COMM_CSP` stale semantics remain `activityAgeTicks > 1` fast-group
  tick for node `5` / node `6`
- direct-TCP remains connected-only fallback and does not inherit active stale
  semantics from silence alone
- service-managed target node-`5` remains the fresh target/lab support path for
  bounded command/readback and service-journal evidence

## Not Covered

- reliable transfer, ARQ, NACK, or CFDP behavior
- RF over-the-air truth or real-hardware RSSI claims
- SNR fields or any signal-quality contribution to provider or COMM policy
- broader UHF beacon/session arbitration or dual-link runtime redesign
- hosted-style interactive status dump on the service-managed target OBC

## Governing Commands

- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate -f`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate --ut -f`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build --ut`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util check --all`
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
- `python3 scripts/check_component_test_baseline.py`
- `bash scripts/run_radio_metrics_v1_hosted_probe.sh`
- `bash scripts/run_radio_metrics_v1_target_probe.sh`
- `openspec validate radio-metrics-v1`
- `openspec validate --specs`

## Results

Date:

- `2026-05-26`

Final local-ready verification:

| Step | Command | Result |
|---|---|---|
| generate | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate -f` | `PASS` |
| build | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build` | `PASS` |
| UT generate | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate --ut -f` | `PASS` |
| UT build | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build --ut` | `PASS` |
| local gate | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util check --all` | `PASS` |
| authoritative local-ready gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | `PASS` |
| component baseline | `python3 scripts/check_component_test_baseline.py` | `PASS` |
| focused hosted probe | `bash scripts/run_radio_metrics_v1_hosted_probe.sh` | `PASS` |
| focused target probe | `bash scripts/run_radio_metrics_v1_target_probe.sh` | `PASS` |
| OpenSpec change validate | `openspec validate radio-metrics-v1` | `PASS` |
| OpenSpec spec validate | `openspec validate --specs` | `PASS` |

Hosted proof notes:

- Fresh hosted proof passed through `scripts/run_radio_metrics_v1_hosted_probe.sh`.
- Hosted node-`5` S-band `COMM_CSP` evidence root:
  `/tmp/radio-metrics-v1-hosted.1JrzIy/sband-node5`
- Hosted node-`6` UHF bounded backup evidence root:
  `/tmp/radio-metrics-v1-hosted.1JrzIy/uhf-node6`
- Hosted direct-TCP connected-only fallback evidence root:
  `/tmp/radio-metrics-v1-hosted.1JrzIy/direct-tcp`
- Proven hosted claims:
  raw per-band `groundLinkRaw` readback prints backend `mode`,
  `healthSemantics`, `connected`, `txChunks`, `rxChunks`, `txBytes`,
  `rxBytes`, `txErr`, `rxErr`, and `statusObs`; cached
  `radioObservation sample/ageTicks/result` is printed separately from the raw
  numeric radio fields.
- Proven hosted truth boundaries:
  `comm-csp` node-`5` and node-`6` show active-band raw observation with
  provider-facing semantics still derived separately; direct-TCP shows
  `CONNECTED_ONLY_FALLBACK` without inheriting active stale semantics.

Target/lab proof notes:

- Fresh target/lab proof passed through
  `scripts/run_radio_metrics_v1_target_probe.sh`.
- Target node-`5` service-managed command/readback evidence:
  `/tmp/radio-metrics-v1-target.Gex4At/target-node5-command-path/service-status.log`
- Target node-`5` command-path recovery log:
  `/tmp/radio-metrics-v1-target.Gex4At/target-node5-command-path/command-path-recovery.log`
- Target node-`5` event capture:
  `/tmp/radio-metrics-v1-target.Gex4At/target-node5-command-path/fprime-events-command-recovery.log`
- Proven target/lab claim:
  the current service-managed node-`5` baseline still supports bounded command
  readback and service-journal review that is consistent with the frozen
  observability contract ownership split.
- Explicit non-claim:
  the target service path does not claim a hosted-style interactive status dump
  surface for the full raw observation contract.

## Verdict

Fresh local verification, the authoritative shared local-ready gate, focused
hosted proof, focused target node-`5` proof, and both OpenSpec validation gates
passed. This change now freezes the current bounded radio/link observability
contract as branch-reviewable baseline truth without claiming reliable
transfer, RF over-the-air behavior, or broader dual-link policy redesign.
