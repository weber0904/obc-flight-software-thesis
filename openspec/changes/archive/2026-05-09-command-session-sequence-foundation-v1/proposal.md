## Why

The command authority vocabulary now includes command class and resource labels, and `command-ingress-source-index-v1` establishes configured ingress source indexes. The final design calls for per-link/source/session sequence windows, but the current routed `Fw.Com` command path has no project-owned `session_id` or `sequence_number` field. Enforcing session or sequence policy on the active command path now would either misuse `Fw.Com.context` or create a fake runtime proof.

This change adds an inactive helper/library contract for future command session and sequence enforcement. It is intentionally not wired into the runtime command path.

## What Changes

- Add a small session/sequence helper keyed by ingress port, link identity, link role, and caller-provided session ID.
- Implement strict monotonic sequence evaluation.
- Provide explicit session reset behavior for future authenticated session-open/resync flows.
- Add direct helper tests proving duplicate/lower sequence rejection and independent keys.
- Document that this is not active runtime enforcement and not replay protection.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: add a command session/sequence foundation contract for future command envelope and authentication work.
- `verification-evidence`: require direct helper evidence and explicit non-enforcement boundaries.

## Impact

- Affected code:
  - `OBC/Components/CommandIngressAuthority` support/helper layer or adjacent command authority library
  - direct L1 tests
  - OpenSpec and evidence docs
- Non-goals:
  - No command envelope, wire format change, authentication, nonce, MAC/signature, runtime enforcement, persistent anti-replay state, resource locks, command ack packet, execution report packet, or replay protection claim.
