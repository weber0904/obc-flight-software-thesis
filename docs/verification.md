# Verification

## Strategy

Verification is layered to match the architecture:

| Layer | Purpose | Typical mechanism |
|---|---|---|
| L1 helper tests | Protocol, parser, storage, and policy logic | C++, Python, and shell unit tests |
| L2 component tests | F Prime port, command, telemetry, and event contracts | generated `TesterBase` / `GTestBase` harnesses |
| Hosted integration | Complete runtime paths with deterministic simulators | repository-owned hosted probes |
| Target/lab integration | Raspberry Pi, UART, SocketCAN, subsystem, and watchdog behavior | target baseline plus focused probe |
| Static governance | Topology, interface, specification, package, and documentation consistency | repository check scripts and OpenSpec |

The full gate is:

```bash
bash scripts/run_verification_ci.sh
```

It generates and builds the native deployment, runs all 74 registered tests,
executes `fprime-util check --all`, validates OpenSpec, and runs repository
contract checks.

## Capability Coverage

| Capability | Unit/component coverage | Integration environment | Evidence |
|---|---|---|---|
| Core deployment and component wiring | topology and component tests | hosted, Raspberry Pi | [deployment runtime](../evidence/records/deployment-runtime-v1/README.md), [target integration](../evidence/records/rpi-target-integration-v1/README.md) |
| Mode safety and autonomy | mode policy and mission component tests | hosted routes | [mode model](../evidence/records/mode-model-v2-v1/README.md), [detumbling autonomy](../evidence/records/deploy-detumbling-autonomy-v1/README.md) |
| Secure command and authority | envelope, session, sequence, and authority tests | hosted and target links | [secure command](../evidence/records/challenge-handshake-secure-command-v1/README.md), [target auth](../evidence/records/target-secure-auth-proof-v1/README.md) |
| EPS and ADCS | subsystem managers and CSP adapter tests | hosted CSP, remote subsystem host | [EPS CSP](../evidence/records/eps-csp-vertical-slice-v1/README.md), [ADCS CSP](../evidence/records/adcs-csp-vertical-slice-v1/README.md) |
| S-band and UHF communication | link policy, framing, session, and transfer tests | hosted dual-link, UART, SocketCAN | [S-band CCSDS](../evidence/records/ccsds-sband-hosted-adoption-v1/README.md), [UHF primary](../evidence/records/uhf-primary-nonquiet-runtime-v1/README.md), [failover](../evidence/records/target-autonomous-uhf-failover-v1/README.md) |
| Payload operations | manager, backend, capture, and product tests | hosted and target capture | [payload E2E](../evidence/records/payload-e2e-downlink-closure-v1/README.md), [raw/preview products](../evidence/records/payload-raw-preview-dual-artifact-v1/README.md) |
| Data products and storage | `.fdp`, HK, storage, and file tests | hosted and target downlink | [onboard products](../evidence/records/onboard-data-products-and-live-beacon-v1/README.md), [HK chunks](../evidence/records/hk-trend-chunked-fdp-v1/README.md) |
| FDIR and recovery | detector, executor, persistent-state, and watchdog tests | hosted fault injection, target reset | [multi-subsystem FDIR](../evidence/records/multi-subsystem-fdir-v1/README.md), [target recovery](../evidence/records/target-recovery-closure-v1/README.md), [watchdog](../evidence/records/target-hardware-watchdog-reset-proof-v1/README.md) |
| Boot and update trust | manifest, version, and rollback tests | package/install workflow | [boot trust](../evidence/records/boot-trust-chain-v1/README.md), [boot update](../evidence/records/boot-update-v1/README.md) |
| Ground operations | gateway, console, sequence, and file tests | hosted/target ground stacks | [Mission Console](../evidence/records/mission-console-phase1/README.md), [per-band stacks](../evidence/records/per-band-stock-ground-stacks-v1/README.md) |
| Chapter 5 scenarios | subsystem and route assertions | hosted Route 1/2/3 | [integrated route closure](../evidence/records/chapter5-integrated-route-closure-v1/README.md) |

## Chapter 5 Routes

The thesis demonstrations group the end-to-end behavior into three routes:

1. sequence-driven payload capture, product creation, downlink, and extraction;
2. mode policy, TTC window behavior, and communication recovery;
3. fault injection, recovery escalation, process restart, and watchdog-facing
   state.

Run them with the commands in
[Chapter 5 Demonstration Routes](operator/thesis-demo.zh-TW.md).

## Interpreting A Record

Every evidence record identifies:

- the capability and exact runtime path;
- the code revision and date;
- the environment and prerequisites;
- commands and observable assertions;
- the verdict and linked artifacts.

The machine-readable [`evidence/catalog.json`](../evidence/catalog.json)
provides record and artifact digests. The detailed
[verification path registry](../evidence/verification-path-registry.md) is the
source for selecting a reusable probe.

## Reproducing A Focused Result

For hosted work:

1. build the current deployment;
2. choose the registered hosted probe;
3. use its isolated runtime root and port variables;
4. inspect the assertions and generated evidence directory.

For target work:

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
bash scripts/<registered-target-probe>.sh
```

The first command owns shared target services, the second owns the ground
stack, and the focused probe owns only its test action and observations.
