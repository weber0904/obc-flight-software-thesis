## Context

The current command ingress chain is:

```text
FprimeRouter.commandOut
  -> CommandIngressAuthority.seqCmdBuffIn[port]
  -> CommandIngressAuthority.seqCmdBuffOut[port]
  -> CmdDispatcher.seqCmdBuff[port]
```

`CommandIngressAuthority` already owns three prerequisites:

- configured authority source mapping by ingress port index;
- command envelope v1 parsing and inner-command unwrap;
- helper-only `CommandSequenceWindow` keyed by ingress port, link identity, link role, and session ID.

## Design

Sequence enforcement applies only to valid envelope v1 commands. Legacy non-envelope commands keep the existing authority path and are not sequence gated.

Valid envelope processing order is locked as `Authority Then Sequence`:

1. parse the envelope and observe metadata;
2. evaluate authority policy for the inner opcode using the configured source for the input port;
3. if authority rejects, return the existing authority rejection and do not update the sequence window;
4. if authority allows, evaluate `CommandSequenceWindow` using `ingressPort`, configured `linkIdentity`, configured `linkRole`, and envelope `sessionId`;
5. forward the inner command only when the sequence result is accepted;
6. synthesize one command status and emit sequence evidence when the sequence result rejects.

This ordering avoids letting restricted or unauthorized envelopes consume sequence numbers before the repository has authentication, session-open, or resync semantics.

## Public Contract

Sequence rejection responses:

| Sequence result | Response |
|---|---|
| duplicate, lower, or wraparound sequence | `Fw::CmdResponse::VALIDATION_ERROR` |
| sequence window full | `Fw::CmdResponse::EXECUTION_ERROR` |

Sequence rejection reason codes:

| Code | Name |
|---:|---|
| 1 | `NOT_INCREASING` |
| 2 | `WINDOW_FULL` |

`COMMAND_SEQUENCE_REJECTED` records ingress port, configured link identity/role, session ID, rejected sequence number, inner opcode, reason, and response. Bounded telemetry records total and per-reason rejection counts plus the last rejected source/session/sequence/opcode/reason.

## Risks And Boundaries

- This is an active duplicate/lower sequence rejection primitive, not full replay protection.
- No production runtime reset command/path is added. Existing helper reset remains test/helper behavior only.
- Sequence state is runtime memory only and is not persisted across reboot.
- Future `session-open/reset/resync` remains required before ground operations can safely recover an accidentally advanced or stale session window without reboot.
- Hosted proof covers the current authority ingress port `0` only.
