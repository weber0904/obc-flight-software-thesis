# public-thesis-submission-v1 Release Evidence

Status: fresh local release-candidate verification for the curated public
repository.

Date: 2026-07-29.

## Provenance And Scope

- Development source commit:
  `142683f20ba46f59f894f594f2caf71dfeddf16f`
- Public release branch: `release/thesis-submission-v1`
- F Prime fork commit:
  `54f02168c676d5b61990d7a48ea9c61b9a8d0b5f`
- libcsp fork commit:
  `241b756a7fb5af1ba0967183b4a2b5843f77ebdb`
- Maintained deployment: `OBC/TopCcsds/topology.fpp`
- Release boundary: native build, unit and integration tests, static
  governance, and selected hosted proofs

Raspberry Pi, serial, SocketCAN, target watchdog, and other hardware-backed
records remain commit-scoped previously demonstrated evidence. They were not
rerun for this public release candidate.

## Publication Boundary

The release is built from an exhaustive source-path disposition manifest. At
the first complete publication audit it classified 3,516 tracked source
paths: 2,108 included, 262 transformed, 1,045 externalized, and 101 excluded.
The final generated counts are authoritative in
[`release/publication-manifest.json`](../../../release/publication-manifest.json).

All 158 source test-record summaries remain reviewable in Git. The 1,045 raw
artifact files are distributed in the deterministic, checksummed release
asset documented by [`docs/evidence/catalog.json`](../../evidence/catalog.json)
and [`release/SHA256SUMS`](../../../release/SHA256SUMS).

The public-tree checker also proves:

- every source path has exactly one publication disposition
- every retained executable has an owner and publication status
- excluded credentials and personal environment defaults are absent
- license texts, dependency notices, SBOM entries, and dependency gitlinks
  agree
- every externalized evidence file is catalogued with original and public
  SHA-256 digests

## Fresh Build And Test Gate

The public tree passed:

```bash
bash scripts/run_verification_ci.sh
```

Covered steps:

- F Prime native generate and build
- F Prime UT generate and build
- 74 registered unit and integration tests
- `fprime-util check --all`
- publication, repository-consistency, documentation, APID/MTU, classic
  component-test, legacy-ZMQ retirement, and OpenSpec checks
- 36 real component harnesses and 5 directly tested helper/support modules
- shell syntax, Python bytecode compilation, and machine-readable JSON checks

## Fresh Hosted Proofs

The following repository-owned probes passed on the public release candidate:

| Proof family | Entrypoint | Result |
|---|---|---|
| Per-band stock ground stacks | `scripts/run_per_band_stock_ground_stacks_hosted_probe.sh` | PASS |
| Secure challenge/command lifecycle | `scripts/run_challenge_handshake_secure_command_hosted_probe.sh` | PASS |
| Uplink authority and key hardening | `scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh` | PASS |
| Node-5 S-band observability governance | `scripts/run_sband_observability_governance_hosted_probe.sh` | PASS |
| Mission Console end-to-end workflow | `scripts/run_mission_console_hosted_probe.sh` | PASS |
| Chapter 5 Route 1 | `scripts/chapter5_routes/hosted/route1_mission_payload.sh` | PASS |
| Chapter 5 Route 2 | `scripts/chapter5_routes/hosted/route2_ttc_link_recovery.sh` | PASS |
| Chapter 5 Route 3 | `scripts/chapter5_routes/hosted/route3_recovery_chain.sh` | PASS |

The fresh F Prime gate additionally covers the registered hosted CSP runtime,
EPS CSP, and ADCS CSP integration tests.

## Observability Oracle Correction

The first public-tree observability run stopped after authentication because
the probe required a repeated
`GROUND_LINK_HEALTH_S_BAND_AVAILABLE` sample. That channel is declared
`update on change`; when its value remains `true`, suppressing a duplicate is
the correct component behavior and is asserted by its classic component test.

The probe oracle was narrowed to require the periodic
`GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS` channel for representative
post-auth ground-link-health visibility. No flight component, FPP interface,
or runtime topology changed. The corrected probe then passed all governed
cases: pre-auth quiet, post-auth open, curated summary, resource surfaces,
bounded EPS readback, reset-cause readback, and S-band close after UHF-primary
switch.

## Non-Claims

This record does not prove:

- fresh Raspberry Pi, physical UART, SocketCAN, or watchdog behavior on the
  public tag
- RF performance or vendor-radio closure
- hardware-backed key storage or production credential handling
- flight certification, mission assurance, or launch readiness

Those boundaries are deliberate release constraints, not implied test gaps.
