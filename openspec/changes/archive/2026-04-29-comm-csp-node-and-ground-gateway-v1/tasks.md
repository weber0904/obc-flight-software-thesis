## 1. Formalize the new COMM TT&C contract

- [x] 1.1 Sync and validate the proposal, design, and delta specs for COMM node `4`, service ports `30-39`, and the first gateway-backed TT&C path.
- [x] 1.2 Update the active topology/runtime surfaces so the ground-link backend can select `direct-tcp` or `comm-csp` without removing the existing direct GDS baseline.

## 2. Implement the OBC-side ground-link integration

- [x] 2.1 Add a repo-owned `GroundLinkDriver` component that implements the F' byte-stream driver contract and preserves direct GDS behavior through a `direct-tcp` backend.
- [x] 2.2 Add the `comm-csp` backend and COMM chunk protocol support so OBC can exchange bounded framed byte chunks with COMM node `4`.
- [x] 2.3 Add classic component-harness coverage for the real `GroundLinkDriver` contract and direct L1 coverage for any new backend/protocol helpers.

## 3. Implement the subsystem COMM node and ground gateway

- [x] 3.1 Add the first COMM CSP protocol definitions, link/status structures, and subsystem-side COMM node executable.
- [x] 3.2 Add the repository-owned ground gateway process that proxies stock `fprime-gds` TCP traffic to the lab-side serial ingress while reusing stock F' framing.
- [x] 3.3 Add focused helper or integration tests that cover bounded uplink polling, downlink writes, and basic gateway serial/TCP proxy behavior.

## 4. Add governed probes, evidence, and regression coverage

- [x] 4.1 Add governed launcher/probe scripts for the first gateway-backed COMM TT&C path and keep the direct GDS baseline probe separately runnable.
- [x] 4.2 Record the new evidence README and verification-path registry entry, clearly separating the new gateway-backed COMM path from reused direct GDS and existing external comm baselines.
- [x] 4.3 Run focused verification, `openspec validate <change>`, and `openspec validate --specs`, then update this task list to reflect completed work.
