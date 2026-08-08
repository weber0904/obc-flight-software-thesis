# ground-link-boundary-split-v1 Evidence

## Scope

This record governs the `ground-link-boundary-split-v1` change that splits low-level ground-link byte/backend ownership from COMM link-health policy.

Target boundary after this change:

- `GroundLinkDriver` owns backend configuration, worker lifecycle, byte-stream send/receive, buffer allocation, and low-level driver telemetry/events
- `GroundLinkHealthProvider` owns per-band runtime observation reduction into `CommLinkHealthView`
- `CommController` consumes provider-owned health views instead of pulling driver stats directly
- active `COMM_CSP` policy scope remains hosted node `5` S-band and node `6` UHF
- legacy generic node `4` remains a compatibility path and is not treated as an active transport-fault policy baseline

## Not Covered

- new southbound keepalive semantics for direct TCP
- new physical RF or UART hardware behavior
- changes to the reusable direct `OBC -> GDS` TCP validation boundary beyond the focused regression probe
- refreshed architecture-review followup notes under `docs/roadmap/architecture-review-followups/`

## Governing Commands

- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate -f`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate --ut -f`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build --ut`
- `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util check --all`
- `python3 scripts/check_component_test_baseline.py`
- `openspec validate ground-link-boundary-split-v1`
- `openspec validate --specs`
- `bash scripts/run_sband_ccsds_primary_probe.sh`
- `bash scripts/run_uhf_node6_backup_probe.sh`
- `bash scripts/run_direct_tcp_ground_link_health_probe.sh`

## Compatibility Note

The legacy hosted node-4 gateway compatibility probe `scripts/run_comm_csp_ground_gateway_probe.sh` has a long-standing flaky `GROUND_LINK_TX_BYTES` channel gate. For this change, command/event round-trip and bounded OBC-side state readback remain the governing compatibility proof, while `GROUND_LINK_TX_BYTES` capture is retained only as best-effort diagnostics.

## Results

Date:

- `2026-05-15`

Final local-ready verification:

| Step | Command | Result |
|---|---|---|
| generate | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate -f` | PASS |
| build | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build` | PASS |
| UT generate | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate --ut -f` | PASS |
| UT build | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build --ut` | PASS |
| local gate | `PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util check --all` | PASS |
| component baseline | `python3 scripts/check_component_test_baseline.py` | PASS |
| OpenSpec change validate | `openspec validate ground-link-boundary-split-v1` | PASS |
| OpenSpec spec validate | `openspec validate --specs` | PASS |
| focused hosted probe | `bash scripts/run_sband_ccsds_primary_probe.sh` | PASS |
| focused hosted probe | `bash scripts/run_uhf_node6_backup_probe.sh` | PASS |
| focused hosted probe | `bash scripts/run_direct_tcp_ground_link_health_probe.sh` | PASS |

Focused hosted probe outcomes:

```text
sband-ccsds-primary-probe: PASS
comm-node=5
primary-command=SBAND
sband-reason=HEALTHY_ACTIVITY
evidence=events-and-status
logs=/tmp/obc-sband-ccsds-primary.sw8JcT

uhf-node6-backup-probe: PASS
formal-verdict=uhf-node6-backup
comm-node=6
framing=space-packet-space-data-link
scid=0x44
vcid=2
frame-size=1024
primary-command=UHF
uhf-reason=HEALTHY_ACTIVITY
evidence=ccsds-failover-and-status
logs=/tmp/obc-uhf-node6-backup.ntEaoy

direct-tcp-ground-link-health-probe: PASS
gds-port=51504
radio-port=51505
status-checks=2
ground-link-mode=direct-tcp
sband-reason=CONNECTED_ONLY_FALLBACK
logs=/tmp/obc-direct-tcp-ground-link.9CbMTY
```

Boundary verdict:

- PASS for the `GroundLinkDriver` / `GroundLinkHealthProvider` / `CommController` boundary split
- active hosted `COMM_CSP` policy remained governed on node `5` S-band and node `6` UHF
- direct TCP remained connected-only fallback and did not inherit stale-activity semantics
- legacy generic node `4` compatibility stayed covered without treating its historically flaky `GROUND_LINK_TX_BYTES` capture as a governing failure oracle
