# CubeSat OBC Flight Software

[繁體中文](README.zh-TW.md) ·
[Architecture](docs/architecture.md) ·
[Interfaces](docs/interfaces.md) ·
[Verification](docs/verification.md) ·
[Operations](docs/README.md#operations) ·
[OpenSpec](openspec/specs/) ·
[Evidence](evidence/README.md)

This repository contains an F Prime v4.1.0–based onboard-computer software
prototype for a CubeSat. It brings mission control, subsystem communication,
fault recovery, command security, data products, target deployment, and
operator tooling into one reviewable system.

The maintained deployment is defined by
[`OBC/TopCcsds/topology.fpp`](OBC/TopCcsds/topology.fpp).

## Highlights

- Mission-mode policy, autonomous sequencing, TTC windows, and payload capture
- EPS, ADCS, COMM, GPS, and payload integration over internal CSP services
- CCSDS S-band operations and a governed UHF primary/failover path
- Challenge-response authentication, session sequencing, and command authority
- Fault detection, subsystem recovery, process restart, and hardware watchdog
- Official F Prime `.fdp` mission-history products and file downlink
- Signed boot manifests, Raspberry Pi packaging, and systemd deployment
- A browser-based Mission Console plus stock F Prime GDS compatibility
- Normative requirements and design history maintained with OpenSpec

## System Overview

```text
 Mission Console / F Prime GDS / CLI
                  |
       S-band CCSDS or UHF link
                  |
  authentication · authority · transfer
                  |
             OBC / TopCcsds
        +---------+---------+
        |         |         |
   mission &    FDIR &    data products
    payload     recovery      & files
        |         |         |
        +------ internal CSP ------+
                 |   |   |   |
                EPS ADCS COMM Payload
```

F Prime provides the component framework, topology language, command and
telemetry infrastructure, sequencing, file services, and data-product
facilities. The project adds the mission topology, OBC components, CSP
subsystem integration, secure command path, deployment tooling, operational
interfaces, and verification system.

## Build

The CI reference environment is Ubuntu 24.04 with Python 3.11. Hosted
development is also supported on macOS.

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

Start the hosted OBC, simulators, radio service, and ground data system:

```bash
bash scripts/run_dev_stack.sh
```

Run the repository verification gate:

```bash
bash scripts/run_verification_ci.sh
```

## Verification

The verification system combines:

- classic F Prime component tests generated from `TesterBase` and `GTestBase`;
- direct tests for protocol, storage, policy, and parser helpers;
- hosted end-to-end probes with isolated ports and runtime directories;
- Raspberry Pi, UART, SocketCAN, and subsystem-backed integration records;
- static checks for topology, interfaces, OpenSpec, documentation, packaging,
  and release integrity.

The release gate builds the deployment and runs all 74 registered tests. The
[verification overview](docs/verification.md) connects system capabilities to
their test environments, while the [evidence library](evidence/README.md)
provides detailed records and machine-readable provenance.

## Repository Map

| Path | Contents |
|---|---|
| `OBC/` | OBC components, `TopCcsds` deployment, configuration, and tests |
| `simulators/` | EPS, ADCS, COMM, GPS, and payload simulation services |
| `scripts/` | Build, operation, packaging, probing, and governance tools |
| `docs/` | Architecture, interfaces, verification, and operator guides |
| `openspec/specs/` | Normative capability specifications |
| `openspec/changes/archive/` | Requirements, designs, and decisions by change |
| `evidence/` | Verification catalog, records, and detailed path registry |
| `obc-dev-spec/` | Narrative engineering specification |

## Security

The checked-in command-auth configuration contains deterministic development
credentials for local simulation and CI. Raspberry Pi packaging requires a
private keystore supplied through `OBC_PACKAGE_KEYSTORE_PATH`. See
[SECURITY.md](SECURITY.md).

## License

Original project material is licensed under Apache-2.0. F Prime, libcsp, and
TooJPEG retain their respective notices and license terms. See
[LICENSE](LICENSE), [NOTICE](NOTICE), and
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Citation metadata is available in [CITATION.cff](CITATION.cff).
