## 1. OpenSpec Artifacts

- [x] 1.1 Create proposal, design, core-system-contracts delta spec, verification-evidence delta spec, verification-path-registry delta spec, and tasks for `command-ingress-authority-v1`.
- [x] 1.2 Validate with `openspec validate command-ingress-authority-v1`.

## 2. Component And Topology

- [x] 2.1 Add `CommandIngressAuthority` as a passive F Prime component with classic UT harness.
- [x] 2.2 Implement configured ingress role handling and deny-by-default invalid/missing config behavior.
- [x] 2.3 Implement command packet decode, authority decision, forward, synthetic response, rejection event, and telemetry counters.
- [x] 2.4 Wire the component into default CCSDS topology between `ComCcsds.fprimeRouter.commandOut` and `CdhCore.cmdDisp.seqCmdBuff`.
- [x] 2.5 Wire the component into legacy ComFprime topology between `ComFprime.fprimeRouter.commandOut` and `CdhCore.cmdDisp.seqCmdBuff`.
- [x] 2.6 Preserve source port index and `Fw.Com.context` through forward and status paths.

## 3. Tests

- [x] 3.1 Add component tests for allowed forward exactly once.
- [x] 3.2 Add component tests for policy deny, malformed restricted, invalid config, and restricted unknown opcode synthetic responses exactly once.
- [x] 3.3 Add component tests for port index/context preservation.
- [x] 3.4 Add component tests for throttled rejection events and unthrottled counters.
- [x] 3.5 Add dictionary/catalog tests asserting no active `CmdSequencer` commands are present or, if present later, are classified and denied for UHF backup.

## 4. Hosted Probes And Evidence

- [x] 4.1 Update or add a default hosted CCSDS S-band probe proving a representative authorized command still works through the gate.
- [x] 4.2 Add hosted configured-profile proof where `uhf-backup` allows one status command and rejects at least one high-authority command with authority evidence and unchanged OBC state.
- [x] 4.3 Record that file and unknown packet authority are deferred and not covered by UHF backup v1.
- [x] 4.4 Update `docs/test-records/` and `docs/verification-path-registry.md` for the upgraded command ingress authority semantics.

## 5. Verification And Closeout

- [x] 5.1 Run fresh build and affected unit/component tests.
- [x] 5.2 Run focused hosted configured-profile probe.
- [x] 5.3 Run `openspec validate link-authority-vocabulary-v1`, `openspec validate command-ingress-authority-v1`, and `openspec validate --specs`.
- [x] 5.4 Update pending docs with completed command ingress authority status and remaining deferred work.
- [x] 5.5 Use change-closeout after implementation.
