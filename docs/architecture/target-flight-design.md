# Target Flight Design

Status: non-canonical planned end-state guide.  
Last reconciled with the public baseline: 2026-07-29.

This file describes the direction beyond `thesis-submission-v1`. It must never
be cited alone as implementation or verification evidence.

## Status Vocabulary

- **Implemented**: present in code and active topology
- **Verified**: implemented and supported by the exact registered evidence path
- **Previously demonstrated**: target/lab evidence exists for an earlier commit
- **Planned**: design direction without current closure

## Current Implemented Foundation

- F Prime `TopCcsds` deployment and component topology
- Internal CSP subsystem services
- S-band and governed UHF communication paths
- Mode safety, mission autonomy, payload operations, FDIR and recovery
- Official data-product history and payload products
- Signed-manifest boot trust decision
- Mission Console and governed operator helpers

## Planned Flight Hardening

- Hardware-backed command and boot trust roots
- Persistent, complete anti-replay policy across power cycles
- Flight-processor timing and resource-budget closure
- RF-qualified radio integration and link-budget validation
- Environmental and long-duration fault-injection campaigns
- Operational key rotation and recovery procedures
- Deployment-specific redundancy, safe-mode and update acceptance review

## Architectural Invariants

- Mission semantics remain independent of the physical carrier.
- Each fault has one detector truth owner and one bounded recovery owner.
- Authority is decided before command or file mutation.
- Flight-history products use official F Prime data-product infrastructure.
- Current evidence and non-claims remain separate from target design intent.
