# Evidence Library

This directory connects capability statements to executable verification and
recorded results.

## Contents

| Path | Purpose |
|---|---|
| [`catalog.json`](catalog.json) | machine-readable record index, artifact mapping, sizes, and SHA-256 digests |
| [`verification-path-registry.md`](verification-path-registry.md) | detailed registry of reusable test paths and entrypoints |
| [`records/`](records/) | result summaries with environment, commands, observations, and verdicts |

Large logs, packet captures, images, and binary outputs are distributed in:

`obc-flight-software-thesis-evidence-thesis-submission-v1.tar.gz`

The catalog records the archive digest, byte size, embedded-manifest digest,
and the files associated with each record. Extracted files can therefore be
matched to the repository summary without relying on filenames alone.

## Recommended Entry Points

### Architecture And Build

- [Deployment Runtime](records/deployment-runtime-v1/README.md)
- [Raspberry Pi Target Integration](records/rpi-target-integration-v1/README.md)
- [Boot Trust Chain](records/boot-trust-chain-v1/README.md)

### Command And Communication

- [Challenge Handshake And Secure Command](records/challenge-handshake-secure-command-v1/README.md)
- [Per-Band Stock Ground Stacks](records/per-band-stock-ground-stacks-v1/README.md)
- [UHF Primary Runtime](records/uhf-primary-nonquiet-runtime-v1/README.md)
- [Autonomous UHF Failover](records/target-autonomous-uhf-failover-v1/README.md)

### Mission And Payload

- [Mode Model](records/mode-model-v2-v1/README.md)
- [Payload End-To-End Downlink](records/payload-e2e-downlink-closure-v1/README.md)
- [Payload Raw And Preview Products](records/payload-raw-preview-dual-artifact-v1/README.md)
- [Onboard Data Products](records/onboard-data-products-and-live-beacon-v1/README.md)

### Fault Management

- [Multi-Subsystem FDIR](records/multi-subsystem-fdir-v1/README.md)
- [Recovery Executors](records/recovery-executors-v1/README.md)
- [Target Recovery](records/target-recovery-closure-v1/README.md)
- [Hardware Watchdog Reset](records/target-hardware-watchdog-reset-proof-v1/README.md)

### Ground Operations And Thesis Routes

- [Mission Console](records/mission-console-phase1/README.md)
- [Chapter 5 Integrated Route Closure](records/chapter5-integrated-route-closure-v1/README.md)
- [Release Gate](records/public-thesis-submission-v1/README.md)

## Record Format

A record contains:

- the change or capability under test;
- the source revision and execution date;
- the host, target, carrier, and service topology;
- setup and execution commands;
- expected and observed results;
- a verdict;
- links to code, specifications, and artifacts.

Records are immutable engineering observations. A later implementation change
adds another record and updates the capability-level verification overview.

## Selecting A Probe

Use the [verification path registry](verification-path-registry.md) when
repeating a result. It distinguishes paths that can otherwise look similar,
including:

- direct OBC-to-GDS and CLI-through-GDS;
- S-band node `5` and UHF node `6`;
- hosted ZMQ/TCP and target SocketCAN/UART;
- live telemetry, explicit readback, file products, and beacon output;
- hosted recovery, target process restart, and hardware watchdog reset.

The selected entry identifies the owning launcher, prerequisites, assertions,
and output record.
