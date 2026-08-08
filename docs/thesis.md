# Thesis Technical Index

The thesis contribution is the integration of a reviewable OBC software
architecture across mission policy, subsystem communication, secure command,
fault recovery, data products, deployment, and ground operations.

## Contribution Map

| Thesis theme | Implementation | Specification | Verification |
|---|---|---|---|
| Component-oriented OBC | `OBC/Components/`, `OBC/TopCcsds/` | [`core-system-contracts`](../openspec/specs/core-system-contracts/spec.md) | [component and deployment coverage](verification.md#capability-coverage) |
| Multi-subsystem CSP integration | EPS, ADCS, COMM, and payload adapters | [`csp-runtime-owner`](../openspec/specs/csp-runtime-owner/spec.md) | [CSP vertical slices](verification.md#capability-coverage) |
| Secure TTC | command authority, handshake, sessions, freshness | [`interface-contract-index`](../openspec/specs/interface-contract-index/spec.md) | [secure command records](../evidence/records/challenge-handshake-secure-command-v1/README.md) |
| S-band/UHF communication | CCSDS gateways and COMM nodes `5`/`6` | [`comm-subsystem`](../openspec/specs/comm-subsystem/spec.md) | [per-band stacks](../evidence/records/per-band-stock-ground-stacks-v1/README.md) |
| Autonomy and safety | mode policy, sequences, TTC window, low battery | [`mission-autonomy`](../openspec/specs/mission-autonomy/spec.md) | [mode and autonomy records](../evidence/records/mode-safety-policy-v1/README.md) |
| FDIR | detectors, recovery executors, persistent faults, watchdog | [`persistent-fault-ring`](../openspec/specs/persistent-fault-ring/spec.md) | [FDIR route](../evidence/records/multi-subsystem-fdir-v1/README.md) |
| Mission data | `.fdp` products, HK trends, payload raw/preview | [`onboard-data-products-and-live-beacon`](../openspec/specs/onboard-data-products-and-live-beacon/spec.md) | [data-product records](../evidence/records/onboard-data-products-and-live-beacon-v1/README.md) |
| Deployability | signed manifests, versioning, Raspberry Pi services | [`platform-baseline`](../openspec/specs/platform-baseline/spec.md) | [package and boot records](../evidence/records/boot-trust-chain-v1/README.md) |
| Operator usability | Mission Console and Chapter 5 route runners | [`mission-console`](../openspec/specs/mission-console/spec.md) | [integrated route closure](../evidence/records/chapter5-integrated-route-closure-v1/README.md) |

## Demonstration Structure

The Chapter 5 demonstrations are designed as complete mission narratives rather
than isolated component calls:

- Route 1 follows a payload request from sequence execution to file extraction.
- Route 2 exercises mode/TTC policy and communication-link recovery.
- Route 3 injects subsystem faults and observes recovery escalation.

Commands and expected observations are in
[論文章節展示流程](operator/thesis-demo.zh-TW.md). Simulator inputs used during
the demonstrations are documented in
[Simulator Controls](operator/simulator-controls.zh-TW.md).

## Reading The Engineering Record

Use the following sequence when reviewing a claim:

1. [Architecture](architecture.md) for ownership and system boundaries.
2. [Interfaces](interfaces.md) for packet, node, authority, and size contracts.
3. the relevant capability under [`openspec/specs/`](../openspec/specs/).
4. [Verification](verification.md) for the test layer and environment.
5. the linked record under [`evidence/records/`](../evidence/README.md).
6. the archived OpenSpec change for the design rationale and task history.

This structure connects thesis-level statements to source code, formal
requirements, executable verification, and recorded outputs.
