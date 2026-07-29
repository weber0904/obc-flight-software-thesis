# Verification Troubleshooting Playbook

Status: current public support guide.  
Last reconciled: 2026-07-29.

## Select The Exact Path First

Start from `verification-path-registry.md`. Keep these distinctions explicit:

- direct OBC-to-GDS versus gateway/COMM TT&C
- hosted CSP versus target CSP
- ZMQ/TCP development carrier versus SocketCAN or physical UART
- S-band node `5` versus UHF node `6`
- current non-quiet/autonomous-failover paths versus historical quiet/manual
  paths
- runtime observation versus ground-side packet observation

## Build Before Evidence

Run the full generate/build/UT gate before a focused hosted probe. Output from
old binaries is diagnostic only. Record the source commit and dependency SHAs.

## Isolate Hosted Runs

- Use a fresh runtime root and file store.
- Allocate bounded, non-conflicting ports.
- Reap only repository-owned stale listeners.
- Start GDS, wait for readiness, then start CLI listeners before the runtime can
  emit the traffic being asserted.
- Use managed cleanup and run an immediate rerun-safety check when touching a
  lifecycle-heavy probe.

## Assert The Right Oracle

- A runtime log is not a substitute for required GDS/CLI observation.
- One backlog-prone channel sample is not final-state proof.
- Distinguish event, telemetry, readback, file, and persistent product evidence.
- State both what a probe proves and what it does not prove.

## Target A/B/C Ownership

For every independent target testcase:

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
bash scripts/<current-functional-probe>.sh
```

The functional probe must not restart or stop shared target baseline services.
Within one continuous failover scenario, do not rerun A/B mid-flow.

## Common Failure Triage

1. Confirm source, build, installed release and dictionary provenance.
2. Confirm runtime root, ports and listener startup order.
3. Confirm the selected registry path and command-authority profile.
4. Check service readiness and CSP node identity.
5. Separate missing ground observation from rejected onboard behavior.
6. Preserve failed attempts and classify infrastructure, harness, or product
   failure honestly.

Historical case timelines remain in `docs/test-records/`; they are not repeated
here as current instructions.
