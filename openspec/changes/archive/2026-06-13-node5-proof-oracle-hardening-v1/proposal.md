## Why

The freshly archived node-`5` residual-observability cleanup landed the intended governance buckets, but review found that two maintained proof families still had oracle weaknesses: the hosted and target node-`5` observability proofs could accept passive-listener quiet without reasserting gateway-byte quiet, and the target secure-auth proof could stay pinned to stale S-band wire-capture counts even when the native packet log saw the newer handshake. This needs a bounded follow-up now because these are proof-integrity bugs on already-governed paths, not future cleanup ideas.

## What Changes

- Harden the hosted node-`5` observability proof so bounded detailed `GET_*` readback stays reviewable through a bounded command-specific packet-path artifact, and primary-switch close still requires downlink-capture quiet in addition to passive observer behavior.
- Harden the target node-`5` observability proof with the same split oracle so the maintained target path does not rely only on passive observer quiescence and does not mistake normal post-auth summary traffic for a failure.
- Replace the target secure-auth handshake watcher's single-count fallback with source-aware progress tracking across wire capture and native packet logs so S-band retries can recover from capture-source drift without misclassifying old messages as new ones.
- Record the hardened oracle boundary in the formal verification artifacts without widening the underlying product or path claims.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `verification-evidence`: tighten the formal evidence wording for maintained node-`5` observability and target secure-auth proofs so bounded detailed readback, switch-close quiet, and handshake-source recovery stay reviewable on the governed packet path.
- `verification-path-registry`: clarify that the maintained hosted and target node-`5` observability entries are packet-path proofs, and that the maintained target secure-auth entry may use source-aware handshake observation without changing the path identity.

## Impact

- Affected code:
  repository-owned hosted and target probe helpers under `scripts/`
- Affected systems:
  hosted node-`5` observability governance proof, target node-`5` observability governance proof, target secure-auth proof
- Affected docs/specs:
  change artifacts, verification evidence/registry specs, and the follow-up evidence record
