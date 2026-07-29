## Context

The active routed command chain is:

```text
FprimeRouter.commandOut
  -> CommandIngressAuthority.seqCmdBuffIn[port]
  -> CommandIngressAuthority.seqCmdBuffOut[port]
  -> CmdDispatcher.seqCmdBuff[port]
```

`CommandIngressAuthority` already owns:

- configured ingress authority identity and role by port index
- envelope v1 parse/unwrap
- explicit `SESSION_OPEN(seq0)` lifecycle
- strict-monotonic sequence enforcement after lifecycle match

What is still missing is source-bound integrity verification on the active envelope path. This change adds authenticated envelope verification inside the same runtime owner instead of creating a new gate.

## Design

### Auth contract

- Keep the existing outer pseudo-opcode `0x0BC10001`.
- Extend envelope v1 with:
  - `source_id`
  - `key_slot`
  - `session_id`
  - `sequence_number`
  - inner command length/payload
  - MAC
- Use `HMAC-SHA256` only in v1.
- MAC input covers protocol/version fields, `source_id`, `key_slot`, `session_id`, `sequence_number`, and the full inner command payload.
- Authority role/classification remains local OBC policy state and is not sender-controlled.

### Runtime auth config

- Extend the existing ingress config passed into `CommandIngressAuthority` with repo-controlled auth config:
  - `enabled`
  - expected `source_id`
  - expected `key_slot`
  - algorithm enum fixed to `HMAC_SHA256` for this change
  - shared key material
- Hosted runtime/profile wiring remains the configuration source for v1.
- `sband-primary` and `uhf-backup` each get deterministic source identity, key slot, and key material.
- This is a provider boundary only; it does not introduce external catalog loading or persistent secure storage.

### Runtime order and boundaries

For enveloped command traffic, runtime order becomes:

1. parse envelope
2. verify auth
3. authority
4. lifecycle
5. sequence
6. dispatch

State-mutation boundaries:

- parse/auth failure:
  - no session open
  - no active session metadata mutation
  - no sequence consumption
- auth-pass but authority-denied:
  - no implicit session open
  - no active session metadata mutation
  - no sequence consumption
- auth-pass and authority-pass but lifecycle-denied:
  - no sequence consumption
- only auth-pass, authority-pass, lifecycle-pass traffic may reach sequence evaluation
- only auth-pass, authority-pass, lifecycle-pass, sequence-pass traffic may dispatch

Auth-failed traffic must never be reported in a way that can be confused with authenticated acceptance. Low-level parse/raw-envelope telemetry is allowed only if it is clearly distinct from authenticated/session-valid evidence.

### Source binding

- Envelope `source_id` and `key_slot` are cross-checked against the configured ingress source.
- The configured ingress port remains the runtime source epoch owner.
- `Fw.Com.context`, gateway metadata, and packet body claims do not establish trust.

### Legacy compatibility

- Legacy non-envelope routed commands remain supported.
- Legacy traffic stays explicitly outside the authenticated claim.
- Legacy traffic must not inherit authenticated privileges, session semantics, or authenticated evidence surfaces.

### Replay boundary

- This change can claim authenticated ingress foundation with lifecycle/sequence binding.
- It cannot claim full replay protection because there is still no nonce window, persistent anti-replay state, or reboot-persistent secure state.

### Verification shape

- Component coverage extends the existing classic `CommandIngressAuthority` harness plus direct helper tests.
- Hosted proof stays on current routed ingress port `0` only.
- The new hosted probe reuses the current raw GDS injector pattern and proves acceptance/rejection ordering rather than broad transport behavior.
