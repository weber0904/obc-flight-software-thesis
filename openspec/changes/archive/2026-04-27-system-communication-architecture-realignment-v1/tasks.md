## 1. Architecture Baseline Realignment

- [x] 1.1 Convert the current decision snapshot into governed architecture wording across the change artifacts and checked-in review docs
- [x] 1.2 Update repository-facing architecture narratives so direct GDS, omitted-RF TT&C, internal CSP, and direct GPS paths are named as separate domains
- [x] 1.3 Record the agreed near-term hardware allocation for `obc.local` and `subsystem.local`, including direct GPS UART and the two subsystem-side CAN channel groups

## 2. Formal Spec Alignment

- [x] 2.1 Sync the new `ground-ttc-gateway` capability and the modified architecture requirements into the formal spec layer
- [x] 2.2 Reconcile `platform-baseline`, `comm-subsystem`, `core-system-contracts`, and `gps-subsystem` so their future roles no longer conflict
- [x] 2.3 Reconcile `verification-evidence` and `verification-path-registry` so future implementation slices cannot over-claim adjacent path coverage

## 3. Follow-On Slice Definition

- [x] 3.1 Define the governed follow-on sequence for `gps-live-uart-source-v1`, `shared-canfd-csp-bus-foundation-v1`, `comm-csp-node-and-ground-gateway-v1`, and `ttc-over-comm-end-to-end-v1`
- [x] 3.2 Capture the current hardware-driven limitations explicitly, including shared-controller CAN realism limits and the lack of full dual-bus redundancy on `obc.local`
- [x] 3.3 Leave unresolved implementation-specific questions as explicit open questions instead of silently fixing them in code or scripts

## 4. Closeout Validation

- [x] 4.1 Run `python3 scripts/check_repo_consistency.py`
- [x] 4.2 Run `python3 scripts/check_agent_entrypoint.py`
- [x] 4.3 Run `openspec validate system-communication-architecture-realignment-v1`
- [x] 4.4 Run `openspec validate --specs`
