## Context

The active command ingress chain is still:

```text
FprimeRouter.commandOut
  -> CommandIngressAuthority.seqCmdBuffIn[port]
  -> CommandIngressAuthority.seqCmdBuffOut[port]
  -> CmdDispatcher.seqCmdBuff[port]
```

`CommandIngressAuthority` already owns:

- configured source identity by ingress port index;
- command envelope v1 parse/unwrap;
- active authority-then-sequence enforcement for valid envelopes;
- runtime-memory-only sequence state keyed by ingress port, link identity, link role, and `session_id`.

What is still missing is an explicit session lifecycle contract. Today, any first authority-allowed command for a new `session_id` becomes the baseline implicitly. That makes operator recovery after desync ambiguous and leaves reboot behavior undocumented.

## Design

Session lifecycle remains inside `CommandIngressAuthority`; no parallel manager is introduced.

### Lifecycle command surface

- Add a dictionary-visible `SESSION_OPEN()` command on `CommandIngressAuthority`.
- `SESSION_OPEN` is a valid inner opcode for envelope handling, but it is not allowed as a legacy non-envelope command path.
- Legacy direct `SESSION_OPEN` SHALL be rejected before `CmdDispatcher`.

### Source epoch model

- A source epoch is `(ingressPort, linkIdentity, linkRole)`.
- Each source epoch may have at most one active session.
- The active session state records:
  - active flag
  - current `session_id`
  - last accepted sequence

### Open / replace / resync contract

- `SESSION_OPEN` is the only v1 open and recovery surface.
- `SESSION_OPEN` requires:
  - valid command envelope v1
  - authority allow on the inner opcode
  - `sequence_number == 0`
- If the source epoch has no active session:
  - accept `SESSION_OPEN`
  - establish active session `session_id`
  - establish sequence baseline `0`
- If the source epoch is already open with the same `session_id`:
  - reject with `ALREADY_OPEN`
  - do not mutate lifecycle or sequence state
- If the source epoch is already open with a different `session_id`:
  - accept the fresh `SESSION_OPEN`
  - atomically replace the prior session
  - discard the old session's sequence state
  - establish the new session baseline at `0`

This is also the v1 resync path after duplicate/lower/stale sequence rejection.

### Runtime order

For valid envelope v1 commands, processing order becomes:

1. parse envelope
2. observe metadata
3. evaluate authority on the inner opcode
4. if the inner opcode is `SESSION_OPEN`, apply lifecycle open/replace rules and synthesize one response without `CmdDispatcher`
5. if the inner opcode is not lifecycle:
   - require an active source session
   - require envelope `session_id` to match the active session
6. evaluate strict-monotonic sequence using the active session key
7. forward exactly one inner command only when the sequence result is accepted

Authority-denied, malformed, invalid-config, unknown/restricted, and legacy-lifecycle commands do not mutate session or sequence state.

### Reboot/runtime reset behavior

- Session lifecycle state remains runtime memory only.
- OBC process restart or topology teardown clears all lifecycle and sequence state.
- After reboot/runtime restart, enveloped non-lifecycle commands must fail closed until a fresh `SESSION_OPEN` is accepted.

### UHF backup boundary

- `uhf-backup` is allowed to use `SESSION_OPEN`.
- This preserves the existing operational boundary where UHF may still send enveloped read/status commands.
- `SESSION_OPEN` does not expand UHF authority; follow-on inner commands remain limited by the existing `uhf-backup` allowlist.

## Public Contract

Session lifecycle reject reasons include at least:

| Code | Name |
|---:|---|
| 1 | `NOT_OPEN` |
| 2 | `SESSION_MISMATCH` |
| 3 | `ALREADY_OPEN` |
| 4 | `BAD_OPEN_SEQUENCE` |
| 5 | `LEGACY_LIFECYCLE_UNSUPPORTED` |
| 6 | `WINDOW_FULL` |

`COMMAND_SESSION_OPENED` records ingress port, link identity, link role, session ID, and replace flag.

`COMMAND_SESSION_REJECTED` records ingress port, link identity, link role, session ID, sequence number, inner opcode, reason, and response.

Telemetry records:

- whether a session is active
- the active source epoch
- the active session ID
- the last accepted sequence
- total session opens
- total session rejects
- the last reject reason

## Risks And Boundaries

- This is still not authenticated replay protection.
- No persistent secure storage is added for session lifecycle.
- No change is made to the existing envelope wire format.
- Hosted proof remains limited to the currently routed ingress port `0`.
- `command-auth-envelope-v1` is expected to add authenticated source, MAC/signature, and stronger anti-replay inputs ahead of this lifecycle contract rather than replacing it.
