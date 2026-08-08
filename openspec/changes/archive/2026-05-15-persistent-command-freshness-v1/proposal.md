## Why

The current active command-security path already requires authenticated command
envelopes, explicit `SESSION_OPEN`, and strict in-memory sequence monotonicity.
What it still does not survive is process restart: reboot clears all session
state, so an old `SESSION_OPEN(seq0)` may be replayed after restart and reopen
the source under the previous session epoch.

Follow-up 02 needs a bounded reboot-safe anti-replay contract on the active
`OBC` / `TopCcsds` path without changing the wire format, adding nonce
challenge flow, or moving session persistence into `BootManager`.

## What Changes

- Upgrade `session_id` into a monotonic reopen epoch per source epoch
  `(ingressPort, linkIdentity, linkRole)`.
- Persist the highest accepted session epoch or next allowed reopen floor in a
  dedicated `CommandIngressAuthority` freshness store under
  `persistent-data/command-ingress/`.
- Require `SESSION_OPEN(seq0)` after restart to use a strictly higher
  `session_id` than any previously accepted session for that source epoch.
- Keep per-command sequence enforcement in memory after open; do not persist
  every accepted sequence.
- Fail closed for comm-managed enveloped ingress if the persistent freshness
  store cannot be loaded truthfully.
- Add dedicated telemetry, events, tests, hosted probes, and bounded Raspberry
  Pi persistence evidence on the active path.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: define reboot-safe monotonic `session_id` behavior
  and fail-closed persistent freshness ownership for `CommandIngressAuthority`.
- `interface-contract-index`: require `docs/interfaces.md` to describe
  `session_id` as a monotonic reopen epoch on the active path and keep the
  non-claims explicit.
- `verification-evidence`: require store tests, component tests, hosted reboot
  probe proof, and Raspberry Pi persistence evidence.
- `verification-path-registry`: extend the active hosted command-ingress path
  and add a separate Raspberry Pi persistence path for this bounded closure.

## Impact

- Affected code:
  - `OBC/Components/CommandIngressAuthority`
  - active and legacy `Main*.cpp` runtime wiring
  - hosted runtime argument plumbing
  - unit tests, hosted probes, Raspberry Pi persistence probe, and evidence
- Public/dictionary-visible impact:
  - `session_id` semantics become monotonic reopen epoch per source epoch on
    the active path
  - new persistent-freshness telemetry and events become reviewable operator
    evidence
- Non-goals:
  - no envelope wire-format change
  - no nonce or challenge-response replay design
  - no persistent secure key storage
  - no hardware-backed secure boot
  - no legacy retirement
  - no full COMM lab-operational closure
