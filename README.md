# CubeSat OBC Flight Software Thesis

[繁體中文摘要](README.zh-TW.md) ·
[Architecture](docs/architecture/current-development-architecture.md) ·
[Verification](docs/verification-matrix.md) ·
[OpenSpec](openspec/specs/) ·
[Evidence](docs/evidence/README.md)

An F Prime v4.1.0–based CubeSat onboard-computer flight-software prototype.
The repository is the curated `thesis-submission-v1` public release: it keeps
the implemented system, formal specifications, reviewable evidence, and
maintained verification paths while separating historical development material
from current operator guidance.

> Research prototype: this software is not flight-certified. Hardware and lab
> results are commit-scoped previously demonstrated evidence, not fresh target
> verification of the public tag. See [SECURITY.md](SECURITY.md) and
> [release provenance](release/RELEASE_PROVENANCE.md).

## What This Project Demonstrates

- One maintained F Prime deployment:
  `OBC/TopCcsds/topology.fpp`
- Component-oriented OBC services for mode safety, mission autonomy, payload
  operations, command authority, health monitoring, recovery, boot trust, and
  official F Prime data products
- Internal CSP services for EPS, ADCS, COMM, and virtual payload paths
- CCSDS S-band primary operations plus governed non-quiet UHF primary/failover
  paths
- Challenge-response secure command bootstrap and secure-command sequencing
- Hosted simulation, Raspberry Pi packaging, target/lab operating procedures,
  Mission Console, and staged Chapter 5 verification routes
- Formal engineering history through 35 OpenSpec capabilities and archived
  change artifacts

## Architecture At A Glance

```text
Mission Console / stock F Prime GDS / repository helpers
                         |
             CCSDS S-band or governed UHF
                         |
            Command and file authority gates
                         |
                 OBC / TopCcsds
        +----------------+----------------+
        |                |                |
   Mission/mode     FDIR/recovery    Data products
        |                |                |
        +---------- internal CSP ----------+
                   |    |    |    |
                  EPS  ADCS COMM Payload
```

F Prime supplies the component model, topology, command/telemetry/event
infrastructure, sequencing, file services, and data-product foundations.
This project supplies the mission-specific topology, authority and recovery
components, CSP subsystem integration, packaging, operator surfaces, and
evidence-governed verification. See
[Project Contributions](docs/architecture/project-contributions.md).

## Quick Start

Supported release CI environment: Ubuntu 24.04, Python 3.11, Node 24. macOS is
also used for hosted development and lab ground operations.

```bash
git clone --recurse-submodules \
  https://github.com/weber0904/obc-flight-software-thesis.git
cd obc-flight-software-thesis

python3 -m venv fprime-venv
fprime-venv/bin/pip install --upgrade pip setuptools wheel
fprime-venv/bin/pip install -r requirements.txt

bash scripts/bootstrap_dev_config.sh
fprime-venv/bin/fprime-util generate -f
fprime-venv/bin/fprime-util build
```

Run the default hosted S-band development stack:

```bash
bash scripts/run_dev_stack.sh
```

Run the full repository gate:

```bash
bash scripts/run_verification_ci.sh
```

The public command-auth example contains known development keys. Never use it
for a target. Target packaging requires an external private keystore through
`OBC_PACKAGE_KEYSTORE_PATH`.

## Verification Status

| Boundary | Release status |
|---|---|
| Static governance, OpenSpec, build and unit tests | Required on the public commit |
| Selected hosted runtime and Chapter 5 paths | Required on the public commit |
| Raspberry Pi, SocketCAN, UART and watchdog | Previously demonstrated; preserved with original provenance |
| RF, flight certification, hardware-backed key storage | Not claimed |

Start from the [Verification Matrix](docs/verification-matrix.md) and
[Verification Path Registry](docs/verification-path-registry.md). Do not pick
a script merely because its name resembles an old evidence record.

## Repository Guide

- `OBC/`: components, runtime, topology, and component tests
- `simulators/`: repository-owned EPS, ADCS, COMM, GPS, and payload substrates
- `scripts/`: maintained operators, probes, packaging helpers, and governance
- `docs/`: current architecture, operations, verification, thesis map, and
  test-record summaries
- `openspec/specs/`: normative capability baseline
- `openspec/changes/archive/`: formal development decisions and change history
- `obc-dev-spec/`: human-readable narrative companion to OpenSpec

## Evidence And Reproducibility

Test-record summaries remain in Git. Raw captures, logs, and payload artifacts
are distributed as the checksummed
`obc-flight-software-thesis-evidence-thesis-submission-v1.tar.gz` release
asset. See [Evidence Catalog](docs/evidence/README.md) and
[Publication Manifest](release/publication-manifest.json).

## License And Citation

Original project material is licensed under Apache-2.0. F Prime, libcsp, and
TooJPEG retain their own notices and license terms. See
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md), [NOTICE](NOTICE), and
[CITATION.cff](CITATION.cff).
