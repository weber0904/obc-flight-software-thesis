# System Architecture

## Design Goal

The system is a component-oriented CubeSat OBC prototype built around one
maintained F Prime deployment:
[`OBC/TopCcsds/topology.fpp`](../OBC/TopCcsds/topology.fpp). The architecture
separates mission policy from transport details so that the same OBC behavior
can be exercised in hosted simulation and on a Raspberry Pi target.

## System Context

```text
                    Ground segment
        +----------------+----------------+
        |                                 |
  Mission Console                    F Prime GDS
        |                                 |
        +--------- gateway / radio --------+
                          |
                 CCSDS S-band or UHF
                          |
                 +--------v--------+
                 |  OBC / TopCcsds |
                 +--------+--------+
                          |
                  internal CSP fabric
          +---------------+---------------+
          |               |               |
        EPS node 2     ADCS node 3    COMM nodes 5/6
          |               |               |
     power model      attitude model    radio/link model
                          |
                    payload service
```

Four boundaries remain explicit throughout the implementation:

1. the physical or hosted carrier;
2. the CSP endpoint and service;
3. the CCSDS/F Prime packet class;
4. mission authority to perform the requested action.

This prevents a transport connection from becoming implicit command authority
and lets tests target each boundary independently.

## F Prime Foundation

The project uses F Prime for:

- component interfaces, ports, commands, events, and telemetry;
- FPP topology and dictionary generation;
- rate groups, queues, health monitoring, and active/passive scheduling;
- CCSDS communication subtopologies;
- command sequencing, file upload/download, and data-product services;
- classic generated component-test harnesses.

The `lib/fprime` submodule stays close to v4.1.0. Project-specific integration
fixes and their licensing are recorded in
[`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md).

## Project Components

The project-owned layer under `OBC/Components/` is organized by responsibility.

| Responsibility | Representative components |
|---|---|
| Mission state | mode manager, mode safety, mission executive, TTC pass-window policy |
| Authority | command ingress authority, secure session handling, file admission |
| Autonomy | deployment/detumbling logic, low-battery response, official sequences |
| Subsystems | EPS, ADCS, GPS, COMM, and payload managers/adapters |
| Fault management | detector aggregation, recovery executors, restart policy, watchdog |
| Mission data | housekeeping/data-product producers, storage health, payload products |
| Platform trust | boot manifest, update/version metadata, package-time configuration |

Each real F Prime component has a classic L2 harness. Protocol parsers,
stores, framing helpers, and other support classes have direct L1 tests.

## Runtime Layers

### Mission Layer

Mode policy owns admission and exit conditions for mission states. Autonomous
logic requests transitions through that policy rather than mutating shared
state. TTC windows and payload captures are expressed through commands and
official F Prime sequences, making operator actions and autonomous actions use
the same interfaces.

### Authority Layer

The uplink path associates every command with a source and service. A
challenge-response exchange establishes a session; authenticated envelopes
then carry a monotonic sequence number, command data, and MAC. Replay,
out-of-session, wrong-source, and unauthorized actions are rejected before
dispatch.

File staging and sequence execution pass through dedicated admission checks.
The security configuration is fixed when a target bundle is packaged and its
digest is included in the bundle manifest.

### Communication Layer

The ground-facing link carries CCSDS command, telemetry, event, handshake, and
file packets. S-band uses COMM CSP node `5`; UHF uses node `6`. Link policy can
operate UHF as the selected path and can perform autonomous failover using
health and session state.

Internal CSP connects the OBC with EPS node `2`, ADCS node `3`, COMM nodes
`5`/`6`, and the payload service. Hosted adapters provide reproducible ZMQ/TCP
services; target deployments use TCP, SocketCAN, and UART where configured.

### Fault-Management Layer

Detector components own fault observations. Recovery components own actions:
subsystem reset, safe-state request, bounded process restart, and watchdog
handling. Persistent fault state survives process restart where mission policy
requires it. Hardware watchdog reset is supervised by the target platform
rather than simulated as a normal component restart.

### Data Layer

Live telemetry and events support immediate operations. Persistent mission
history uses official F Prime `.fdp` products with explicit records and chunk
metadata. Payload operations keep preview and raw artifacts distinct so that
operators can inspect a lightweight result without losing the original capture.

## Deployments

### Hosted

`scripts/run_dev_stack.sh` starts the OBC, subsystem simulators, COMM service,
and ground stack. Repository probes allocate isolated ports and runtime roots
for deterministic end-to-end tests.

### Raspberry Pi

The target pipeline builds and packages `TopCcsds`, configuration, scripts,
service units, version metadata, and a private command-auth keystore. The
installer verifies the manifest before changing the active bundle. Systemd
owns startup and restart behavior.

Subsystem simulators can run on a second host, allowing the ground computer,
OBC target, and CSP subsystem services to form a three-machine laboratory
topology.

## Operator Surfaces

The stock F Prime GDS remains the engineering interface. Mission Console adds
a mission-oriented browser surface for:

- overall status and link state;
- authenticated operations and readback;
- sequence authoring, compilation, staging, and execution;
- file transfer and data-product inspection;
- trends, beacons, and packet-level experiments.

Both surfaces use the same OBC command and telemetry dictionaries.

## Engineering Extensions

The architecture provides clear integration points for:

- replacing deterministic development credentials with a hardware-backed
  keystore and device identity;
- characterizing S-band and UHF behavior with flight-like radios, antennas,
  channel loss, and regulatory link budgets;
- moving Raspberry Pi packaging toward a qualified OBC platform and redundant
  storage;
- extending mission assurance with timing analysis, fault-injection campaigns,
  long-duration soak tests, and environmental qualification;
- connecting the data-product and operations model to a mission archive and
  ground scheduling service.

Each extension can be introduced as a bounded OpenSpec change against the
existing authority, transport, recovery, and evidence interfaces.

## Traceability

Architecture requirements are under [`openspec/specs/`](../openspec/specs/).
Implementation decisions are preserved in
[`openspec/changes/archive/`](../openspec/changes/archive/). Capability results
are summarized in [Verification](verification.md) and linked to detailed
records in the [Evidence Library](../evidence/README.md).
