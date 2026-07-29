# Tasks: payload-target-backend-hardening-v1

## 1. Governance

- [x] 1.1 Add proposal, design, tasks, and delta specs for target backend hardening

## 2. Helper Isolation

- [x] 2.1 Add target helper IPC protocol and helper lifecycle management
- [x] 2.2 Move target `libcamera` operations behind the helper boundary
- [x] 2.3 Map timeout, abort, and fault transitions through the helper boundary

## 3. Target Closure

- [x] 3.1 Add target build detection and explicit backend truth
- [x] 3.2 Add target probe for real OV5647 enumerate and JPEG capture
- [x] 3.3 Add target raw-register proof or explicit bounded non-claim
- [x] 3.4 Record Raspberry Pi camera power-control investigation result

## 4. Verification

- [x] 4.1 Add IPC/helper tests
- [x] 4.2 Run hosted regression needed for the new target boundary
- [x] 4.3 Run target proofs and `openspec validate payload-target-backend-hardening-v1`
