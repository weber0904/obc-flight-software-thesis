# Tasks: payload-capture-modes-v2

## 1. Governance

- [x] 1.1 Add proposal, design, tasks, and delta specs for capture-mode v2
- [x] 1.2 Update branch-verifiable docs as the contract evolves

## 2. Public Contract

- [x] 2.1 Add session-kind, profile, and capture-metadata support types
- [x] 2.2 Extend `PayloadOpsController` commands/events/telemetry for auto and deterministic capture
- [x] 2.3 Keep v1 payload commands as wrappers during the PR stage

## 3. Runtime Behavior

- [x] 3.1 Add OFF-only auto and deterministic defaults
- [x] 3.2 Add explicit apply-mask override handling and capability reporting
- [x] 3.3 Add JPEG sidecar metadata write/readback

## 4. Verification

- [x] 4.1 Add or update component/helper tests for session and metadata behavior
- [x] 4.2 Add a hosted node-5 probe for auto and deterministic captures
- [x] 4.2.1 Rebase the hosted official-sequence phase onto the already-proven
  `official-sequencing-system-resources-v1` harness spine instead of a bespoke
  envelope/event path
- [x] 4.3 Run `openspec validate payload-capture-modes-v2`
