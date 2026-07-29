## Context

The active ingress order is already:

```text
parse -> auth -> authority -> lifecycle -> sequence -> dispatch
```

`CommandIngressAuthority` currently owns:

- active source identity by ingress port
- authenticated command-envelope v1 verification
- explicit `SESSION_OPEN` lifecycle
- strict per-session in-memory sequence monotonicity

But all lifecycle state is memory-only. After restart:

- the active session is gone
- an old authenticated `SESSION_OPEN(seq0)` can reopen the source
- stale old-session traffic is rejected only because the session is unopened,
  not because a persisted session epoch floor exists

## Design

Session persistence remains owned by `CommandIngressAuthority`; `BootManager`
keeps owning boot trust and boot metadata.

### Source epoch and persisted floor

A source epoch remains:

- `ingressPort`
- `linkIdentity`
- `linkRole`

For each source epoch, the persistent store records the highest accepted
`session_id` epoch. Operationally this is the reopen floor:

- first-ever open for a source epoch may use any `session_id`, including `0`
- later opens on that source epoch must use a strictly higher `session_id`
- equal or lower `session_id` values are stale replay and fail closed

### Runtime behavior

`SESSION_OPEN` remains the only lifecycle surface and still requires:

- valid command-envelope v1
- auth success
- authority allow
- `sequence_number == 0`

Additional persisted-freshness rules:

- if the persistent store is unavailable or invalid for all copies on a
  comm-managed ingress path, the gate rejects before opening the session
- if `session_id <= persistedFloor`, the gate rejects as stale replay
- if `session_id > persistedFloor`, the gate persists the new floor first,
  then establishes the in-memory active session and sequence baseline `0`

Non-lifecycle envelopes still require:

- an active in-memory session
- matching `session_id`
- strictly increasing in-memory sequence after open

Restart behavior becomes:

- no session is active in memory after restart
- old non-lifecycle traffic fails `NOT_OPEN`
- old `SESSION_OPEN(seq0)` with replayed or lower/equal `session_id` fails
  stale-replay validation
- only a strictly higher `SESSION_OPEN(seq0)` reopens the source epoch

### Persistent store

The store is a dedicated dual-copy snapshot under:

```text
persistent-data/command-ingress/
```

Properties:

- two whole-file copies
- fixed header
- generation counter
- CRC32 over the snapshot payload
- newest valid generation wins
- single-copy corruption falls back to the older valid copy
- both-invalid state fails closed for comm-managed enveloped ingress

The store is not a general history log and does not replace boot metadata.

### Runtime configuration surface

`CommandIngressAuthority` gets a runtime persistent-root configuration surface.
That surface is wired through:

- `OBC::Runtime::RuntimeServices`
- `HostedRuntime`
- `Main.cpp`
- `MainComFprimeLegacy.cpp`

The active path uses that runtime-configured root formally. Legacy is wired only
to keep it buildable and regression-usable.

### Observability

The component exposes dedicated reviewable persistence truth, including:

- persistent-store health or availability
- active source epoch persisted floor
- last loaded generation or copy
- load and save fault counters or last-fault reasons
- explicit stale-replay or persistent-state-unavailable rejection evidence

No new operator command is added in this wave.

### Boot trust boundary

`BootManager` remains unchanged as the owner of:

- signed manifest verification
- version-floor policy
- boot metadata persistence

The bounded boot-trust claim here is only that the corrected active Raspberry Pi
path now runs that existing boot-trust runtime and that target persistence
evidence shows boot-trust metadata and command-freshness state coexist
truthfully on the same active runtime root.

## Risks And Boundaries

- This is still not nonce-based or cryptographically complete replay
  protection.
- A fully unavailable or doubly corrupt freshness store fails closed for
  comm-managed ingress rather than falling back to permissive behavior.
- The proof boundary is bounded to active hosted relaunch and active Raspberry
  Pi persistence; it does not prove power-loss robustness, bootloader handoff,
  hardware secure boot, or legacy retirement.
