# per-band-stock-ground-stacks-v1 Evidence

Date: 2026-05-28.

OpenSpec change: `per-band-stock-ground-stacks-v1`.

## Scope

This record proves the maintained hosted-first near-term simultaneous operator
baseline is now a repo-owned workflow with separate per-band stock ground
stacks. In combined mode the two exposed stock surfaces share one hosted
`TopCcsds` runtime; standalone per-band launchers each own a mode-scoped
runtime subdirectory under the requested runtime namespace root.

Path under proof:

```text
stock fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> shared hosted OBC / TopCcsds
stock fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> uhf_comm_csp_node(node 6) -> shared hosted OBC / TopCcsds
```

Newly proven in this change:

- maintained repo-owned launcher entrypoints exist for:
  - hosted S-band stock stack
  - hosted UHF stock stack
  - combined start/stop composition-only wrapper
- the launcher surface is reviewable:
  - per-band GDS/TTS ports
  - per-band southbound endpoints
  - per-band file stores
  - launcher-owned runtime root
  - internal listener ports for CSP hub, radio mock, and S-band COMM when present
  - per-process logs
  - startup order
  - shutdown/cleanup order
  - explicit operator roles and non-claims
- the combined wrapper starts and stops both distinct stock stacks on one
  shared hosted runtime without claiming a new orchestration owner

This record does **not** prove:

- simultaneous dual-link runtime arbitration
- one stock GDS consuming heterogeneous upstream feeds
- one `ground_ttc_gateway` instance multiplexing simultaneous S-band and UHF
- target/lab simultaneous S-band plus UHF proof
- RF closure
- UHF reliable-transfer redesign

## Maintained Entrypoints

The new maintained operator entrypoints are:

- `scripts/run_hosted_sband_stock_ground_stack.sh`
- `scripts/run_hosted_uhf_stock_ground_stack.sh`
- `scripts/run_hosted_per_band_stock_ground_stacks.sh`
- `scripts/run_per_band_stock_ground_stacks_hosted_probe.sh`
- `scripts/per_band_stock_ground_stacks.py`

These entrypoints reuse the existing stock `fprime-gds` launcher surface and
the hosted `TopCcsds` runtime shape. They do not replace or reinterpret older
probe-only UHF command/downlink evidence as if it were the new operator
workflow.

## Hosted Maintained-Baseline Verdict

Repository-owned proof entrypoint:

```bash
bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| verdict | `PASS` |
| formal verdict | `per-band-stock-ground-stacks-hosted-baseline` |
| logs | `/tmp/per-band-stock-ground-stacks-hosted.ghyAMv` |
| S-band manifest | `/tmp/per-band-stock-ground-stacks-hosted.ghyAMv/sband/stack/manifest.json` |
| UHF manifest | `/tmp/per-band-stock-ground-stacks-hosted.ghyAMv/uhf/stack/manifest.json` |
| combined manifest | `/tmp/per-band-stock-ground-stacks-hosted.ghyAMv/combined/stack/manifest.json` |
| combined requested runtime root | `/tmp/per-band-stock-ground-stacks-hosted.ghyAMv/combined/runtime` |
| combined runtime root | `/private/tmp/per-band-stock-ground-stacks-hosted.ghyAMv/combined/runtime/combined` |
| combined S-band GDS | `127.0.0.1:53279` |
| combined UHF GDS | `127.0.0.1:53281` |

Observed PASS markers:

```text
launcher-sband=PASS
launcher-uhf=PASS
launcher-combined=PASS
combined-distinct-surfaces=PASS
per-band-stock-ground-stacks-hosted-probe: PASS
formal-verdict=per-band-stock-ground-stacks-hosted-baseline
reused-proof=sband hosted CCSDS adoption remains separate
reused-proof=uhf hosted CCSDS adoption remains separate
reused-proof=comm-session-and-downlink-qos remains separate
reused-proof=uhf beacon suppression remains separate
reused-proof=uhf primary packet quiet remains separate
```

What this passing proof means:

- both per-band stock stacks start successfully
- both per-band stock stacks stop cleanly
- the operator surfaces remain distinct and reviewable
- standalone S-band/UHF launchers cannot wipe each other's external runtime
  namespace root by default
- the hosted proof now checks internal listener cleanup as well as exposed
  GDS/TTS and southbound listener cleanup
- the proof stays hosted-only
- the proof does not over-claim orchestration or simultaneous runtime
  arbitration

## Reused Current Baseline Evidence

### Fresh reruns in this change

These fresh reruns confirm the new maintained operator baseline did not reopen
the current S-band and UHF semantic boundaries it is supposed to preserve.

| Surface | Command | Result | Logs |
|---|---|---|---|
| hosted CCSDS S-band adoption | `bash scripts/run_ccsds_sband_hosted_adoption_probe.sh` | `PASS` | `/tmp/obc-ccsds-sband-adoption.UaUJD8` |
| hosted UHF primary packet quiet | `bash scripts/run_uhf_primary_packet_quiet_hosted_probe.sh` | `PASS` | `/tmp/uhf-primary-packet-quiet-hosted.oqSLWv` |
| hosted UHF beacon suppress/runtime | `bash scripts/run_uhf_beacon_suppression_hosted_probe.sh` | `PASS` | `/tmp/uhf-beacon-suppress-hosted.uFq9pQ/attempt-1` |

### Explicitly rerun but still separate blocked path

The helper extraction also touched the repository-owned QoS wrapper surface, so
the closeout reran it explicitly against the fresh build:

| Surface | Command | Result | Logs | Blocker |
|---|---|---|---|---|
| hosted COMM session/downlink QoS | `bash scripts/run_comm_session_and_downlink_qos_probe.sh` | `BLOCKED (known reused path)` | `/tmp/comm-session-downlink-qos.lxy6gm` | timed out waiting for `Command session opened ingress 1 identity 2 role 2 session 9401 replaced 0` in `sband-ground/events.log`; the fresh rerun again observed the S-band session open and pass-start path but no matching node-`6` backup-open event |

This change does **not** convert that still-blocked fresh rerun into passing
evidence. The maintained per-band baseline remains valid because this change
does not claim fresh QoS/orchestration closure, and the wrapper split stayed a
thin `exec` handoff into the preserved core probe logic.

Observed preserved current truths:

- S-band remains the unchanged full formal path.
- UHF primary packet quiet remains primary-band-driven.
- UHF beacon suppress remains accepted-session-driven.
- Official file/data-product downlink remains formal during UHF primary packet
  quiet.

### Directly reused archived evidence

These existing records remain the direct reused evidence for the older explicit
switch and hosted UHF CCSDS adoption behavior. They are intentionally kept
separate from the new maintained operator-baseline proof.

- [evidence/records/comm-session-and-downlink-qos-v1/README.md](../comm-session-and-downlink-qos-v1/README.md)
- [evidence/records/uhf-ccsds-hosted-adoption-v1/README.md](../uhf-ccsds-hosted-adoption-v1/README.md)

This change does not promote either record into proof that the new maintained
operator launcher surface already closes higher-level orchestration or
target-bearing simultaneous behavior.

## Verification

Local checks used for this change:

| Step | Command | Result |
|---|---|---|
| fresh local closeout gate | `PATH="$PWD/fprime-venv/bin:$PATH" bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| helper syntax | `python3 -m py_compile scripts/per_band_stock_ground_stacks.py` | PASS |
| launcher and retained probe shell syntax | `bash -n scripts/run_hosted_sband_stock_ground_stack.sh scripts/run_hosted_uhf_stock_ground_stack.sh scripts/run_hosted_per_band_stock_ground_stacks.sh scripts/run_per_band_stock_ground_stacks_hosted_probe.sh scripts/run_comm_session_and_downlink_qos_probe.sh scripts/run_comm_session_and_downlink_qos_probe_core.sh` | PASS |
| documentation governance | `python3 scripts/check_documentation_governance.py` | PASS |
| repo consistency | `python3 scripts/check_repo_consistency.py` | PASS |
| change validation | `openspec validate per-band-stock-ground-stacks-v1` | PASS |
| main spec validation | `openspec validate --specs` | PASS |

## Verdict

PASS for the new maintained hosted-first per-band stock ground/operator
baseline.

This record now authorizes citing the current repo-owned hosted operator truth
as:

- two stock GDS processes
- two gateway processes
- separate southbound paths
- one shared hosted `TopCcsds` runtime
- one combined wrapper that only coordinates start/stop

It does not authorize citing:

- simultaneous dual-link runtime arbitration
- one-GDS multi-upstream operator surfaces
- one-gateway multiplexer behavior
- target-bearing simultaneous S-band plus UHF closure
- RF closure

Those follow-up boundaries remain intentionally deferred to future
`comm-dual-link-orchestration-v1` or later target-bearing work.
