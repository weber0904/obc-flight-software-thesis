# thesis-submission-v1 Verification

Date: 2026-07-29

## Revisions

| Item | Revision |
|---|---|
| OBC source | `142683f20ba46f59f894f594f2caf71dfeddf16f` |
| F Prime | `54f02168c676d5b61990d7a48ea9c61b9a8d0b5f` |
| libcsp / csp-es | `241b756a7fb5af1ba0967183b4a2b5843f77ebdb` |
| Deployment | `OBC/TopCcsds/topology.fpp` |

## Repository Gate

```bash
bash scripts/run_verification_ci.sh
```

Result: PASS

- native F Prime generate and build;
- unit-test generate and build;
- 74 registered unit and integration tests;
- `fprime-util check --all`;
- OpenSpec and repository contract validation;
- 36 classic F Prime component harnesses;
- 5 directly tested helper/support modules;
- shell, Python, and JSON static checks.

## Hosted End-To-End Results

| Path | Entrypoint | Result |
|---|---|---|
| S-band and UHF ground stacks | `scripts/run_per_band_stock_ground_stacks_hosted_probe.sh` | PASS |
| Secure challenge and command lifecycle | `scripts/run_challenge_handshake_secure_command_hosted_probe.sh` | PASS |
| Uplink authority and key handling | `scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh` | PASS |
| Node `5` observability | `scripts/run_sband_observability_governance_hosted_probe.sh` | PASS |
| Mission Console workflow | `scripts/run_mission_console_hosted_probe.sh` | PASS |
| Chapter 5 Route 1 | `scripts/chapter5_routes/hosted/route1_mission_payload.sh` | PASS |
| Chapter 5 Route 2 | `scripts/chapter5_routes/hosted/route2_ttc_link_recovery.sh` | PASS |
| Chapter 5 Route 3 | `scripts/chapter5_routes/hosted/route3_recovery_chain.sh` | PASS |

The registered gate also exercises hosted CSP runtime, EPS CSP, and ADCS CSP
integration.

## Evidence Integrity

- 158 result summaries are indexed by
  [`evidence/catalog.json`](../../catalog.json).
- 1,045 raw artifacts are mapped to the release evidence archive.
- Source and transformed SHA-256 values are recorded per artifact.
- Dependency revisions, license texts, notices, SBOM entries, and gitlinks are
  checked together.
- The source-to-repository mapping is recorded in
  [`release/publication-manifest.json`](../../../release/publication-manifest.json).

Raspberry Pi, UART, SocketCAN, subsystem, and watchdog results are indexed by
their execution revision, date, environment, commands, and artifact digests in
the [Evidence Library](../../README.md).

## Observability Oracle Correction

The initial node-`5` observability run waited for a duplicate
`GROUND_LINK_HEALTH_S_BAND_AVAILABLE` sample. That channel is configured
`update on change`, so a stable `true` value correctly produces no duplicate.
The probe now uses periodic
`GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS` telemetry for post-authentication
visibility. The corrected probe passed the pre-authentication, authenticated
traffic, summary, resource, EPS readback, reset-cause, and link-switch cases.
