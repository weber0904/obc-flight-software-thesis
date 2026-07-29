# Current Development Architecture

Status: canonical public architecture for `thesis-submission-v1`.  
Last reconciled: 2026-07-29.

## Baseline Boundary

- Framework: F Prime v4.1.0 plus the pinned public project fork
- Maintained deployment: `OBC/TopCcsds/topology.fpp`
- Hosted runtime: macOS/Linux native simulation
- Target integration: Raspberry Pi plus a separate subsystem-simulator host
- Internal network: libcsp/CSP services for EPS, ADCS, COMM, and payload
- Ground links: CCSDS S-band primary and governed UHF primary/failover paths
- Operator surfaces: stock F Prime GDS, repository helpers, and Mission Console

Legacy `OBC/Top`, stock `ComFprime`, the HK ring fallback, public legacy
`SESSION_OPEN`, and quiet-UHF explicit-switch paths are historical only.

## Layering And Ownership

```text
Operator layer
  Mission Console | stock GDS | governed CLI helpers
          |
Ground transport
  CCSDS framing | ground_ttc_gateway | S-band/UHF CSP nodes
          |
Ingress authority
  challenge auth | secure command sequence | file/sequence admission
          |
OBC application
  mode safety | autonomy | payload | state | products | boot/update
          |
FDIR and recovery
  detector-local truth | recovery executors | watchdog
          |
Subsystem services
  EPS | ADCS | COMM | payload over internal CSP
```

Each layer owns a distinct decision:

- Gateways relay transport; they do not own mission authority.
- `CommandIngressAuthority` and secure-link authorization own admitted command
  identity and sequencing.
- `ModeSafetyController` owns current mode-safety policy.
- Detectors own fault truth; recovery executors own bounded actions.
- Official F Prime `.fdp` products own mission history.
- Subsystem bridges use CSP services without exposing transport details as
  mission commands.

## Communication

S-band node `5` is the default maintained command and observability path.
UHF node `6` supports the maintained non-quiet primary/failover path. Accepted
secure auth is required before governed command/readback behavior. Historical
quiet-UHF and explicit-switch evidence remains useful only for its exact
recorded branch.

The project does not claim RF qualification, a single heterogeneous multi-link
GDS, or a single gateway multiplexing all physical links.

## Command And File Authority

The current public secure baseline uses APID `0x00FE` challenge-response
authorization, service-specific root material, strict active-session secure
command sequencing, and bounded file/sequence admission.

The repository tracks only a public example keystore. Hosted developers create
an ignored local copy. Target packaging injects a private keystore before
installation and records its digest; runtime key injection remains prohibited.
This is not hardware-backed key storage or complete persistent replay defense.

## Mission, Payload, And Data Products

- Mode safety, TTC windows, and mission sequences reuse F Prime commands and
  sequencing instead of creating a parallel scheduler.
- Payload operations support governed capture and official preview/raw data
  products.
- Official F Prime `.fdp` products are the only active mission-history path.
- Mission Console provides dashboard, command, readback, sequence, file, trend,
  beacon, and packet-lab surfaces while reusing the existing authority helpers.

## Verification Boundary

The public tag requires full native build/UT/static checks and selected hosted
route probes. Raspberry Pi, UART, SocketCAN, and watchdog records are preserved
as previously demonstrated evidence at their original commits; curation and
keystore packaging changes mean they are not fresh tag-level target proof.

Start every proof selection from `docs/verification-path-registry.md`. Target
and lab work follows A/B/C ownership:

1. `scripts/ensure_target_comm_lab_baseline.sh`
2. `scripts/ensure_ground_dual_gds_baseline.sh`
3. the exact probe-owned functional test

## Known Limits

- No flight certification or RF closure
- No hardware-backed secure key store
- No fresh target rerun on the public release commit
- No claim that hosted timing numbers are flight WCET
- No claim that historical compatibility paths are maintained
- Planned flight hardening remains in `target-flight-design.md`
