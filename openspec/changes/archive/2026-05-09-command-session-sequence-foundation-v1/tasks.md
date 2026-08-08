## 1. OpenSpec Artifacts

- [x] 1.1 Create proposal, design, core-system-contracts delta spec, verification-evidence delta spec, and tasks for `command-session-sequence-foundation-v1`.
- [x] 1.2 Validate with `openspec validate command-session-sequence-foundation-v1`.

## 2. Helper Contract

- [x] 2.1 Add `CommandSessionKey` and `CommandSequenceWindow` helper types.
- [x] 2.2 Implement strict monotonic `evaluateAndAccept(key, sequenceNumber)`.
- [x] 2.3 Implement explicit `resetSession(key)`.
- [x] 2.4 Keep helper inactive; do not wire it into `CommandIngressAuthority` runtime command evaluation.

## 3. Tests And Evidence

- [x] 3.1 Add direct helper tests for first/increasing sequence acceptance.
- [x] 3.2 Add tests for duplicate and lower sequence rejection.
- [x] 3.3 Add tests for independent ingress port, session ID, and role epoch keys.
- [x] 3.4 Add tests for explicit reset and wraparound-without-reset rejection.
- [x] 3.5 Record evidence that this is a foundation only, not active runtime enforcement or replay protection.

## 4. Verification

- [x] 4.1 Run affected helper/component tests.
- [x] 4.2 Run `openspec validate command-session-sequence-foundation-v1` and `openspec validate --specs`.
