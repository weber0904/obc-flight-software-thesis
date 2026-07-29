## Context

Flight-style command session and sequence policy requires a clear source identity and a command-carried sequence field. The current repository has configured command authority profiles and F Prime command context correlation, but no mission command envelope carrying `session_id` or `sequence_number`. This change therefore adds only a helper contract that a future envelope/authentication layer can call.

## Design

Add a helper type set near the command authority implementation:

- `CommandSessionKey`
  - `FwIndexType ingressPort`
  - `AuthorityLinkIdentity linkIdentity`
  - `AuthorityLinkRole linkRole`
  - `U32 sessionId`
- `CommandSequenceWindow`
  - `evaluateAndAccept(key, sequenceNumber)`
  - `resetSession(key)`

Strict monotonic policy:

- First sequence for a key is accepted and becomes the baseline.
- Later sequence is accepted only if `sequenceNumber > lastAccepted`.
- Duplicate or lower sequence is rejected.
- Wraparound is not automatically accepted.
- Resync requires explicit `resetSession(key)` in v1.

The key includes `linkRole`. A role change is treated as a new authority epoch. Future runtime enforcement must pair role changes with explicit session reset/open semantics rather than silently continuing a prior role's sequence state.

## Boundaries

- The helper does not generate, parse, transmit, or persist session IDs.
- Tests provide caller-supplied session IDs.
- The helper is not invoked by `CommandIngressAuthority` for active routed commands.
- This is a future replay/duplicate rejection primitive, not replay protection. Replay protection requires authenticated source, nonce/session-open semantics, active wire enforcement, and possibly persistent state.
