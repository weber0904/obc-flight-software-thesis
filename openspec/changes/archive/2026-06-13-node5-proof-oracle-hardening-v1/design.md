## Context

`node5-observability-residual-cleanup-v1` already established the intended
resource-truth, reviewable-proof, bounded-readback, and diagnostics-only
boundaries. The remaining issue is not product behavior; it is proof-oracle
integrity on already-governed paths.

The hosted observability probe now accepts a bounded-search fallback for
representative detailed `GET_*` readback, but the fallback can stop the passive
channel listener and then let later switch-close acceptance rely only on local
observer quiet. The target observability proof has a similar packet-path risk
because its switch-close acceptance also only rechecks passive observer quiet.

The target secure-auth proof improved by adding native packet-log parsing, but
its S-band watcher still tracks only a single preferred-source count. Once any
old wire-capture handshake exists, the watcher can ignore newer native-log
progress until timeout even though the underlying target path is healthy.

Fresh target reruns also exposed a second maintained-proof weakness on the same
path: after the explicit switch to `uhf-primary-after-failover`, the governed
UHF re-auth loop could retry `REQ_AUTH` before the just-issued challenge had
arrived on the maintained packet-path sources. That creates duplicate
`REQ_AUTH` traffic, churns APID `0x00FE` sequence continuity on the maintained
uplink path, and misclassifies a healthy runtime as a proof failure.

## Goals / Non-Goals

**Goals:**

- Keep the existing hosted and target node-`5` observability path identities and
  product scope unchanged while making the proof acceptance surfaces stricter.
- Require gateway/downlink capture quiet when claiming bounded detailed
  readback and switch-close quiet on the maintained node-`5` observability
  paths.
- Make target secure-auth handshake polling advance on whichever governed
  source actually saw the next handshake message, without confusing duplicate
  source observations for a new handshake.
- Keep target UHF primary re-auth bounded when challenge issuance is confirmed
  but the maintained packet-path artifact arrives late, so the proof does not
  create duplicate `REQ_AUTH` retries on the governed path.
- Record the tightened oracle semantics in the follow-up evidence/spec layer.

**Non-Goals:**

- No secure-auth redesign.
- No second command plane.
- No telemetry schema rewrite.
- No broad UHF baseline widening.
- No new hosted or target proof family identity.

## Decisions

### 1. Hosted and target observability proofs will keep bounded detailed readback and switch-close on distinct packet-path oracles.

The maintained claim is about the packetized node-`5` path, not only about
local `fprime-cli` listeners. The follow-up therefore keeps passive logs as
useful observer surfaces, but it splits the hardening rule by checkpoint:

- representative detailed `EPS_GET_STATUS -> EPS_IBAT` closure must stay
  reviewable through one bounded command-specific packet-path artifact rather
  than only through a long-running passive listener
- S-band close on primary switch away from S-band must still reassert actual
  downlink-capture quiet on the maintained packet path

This keeps a stopped or stale passive listener from satisfying the switch-close
condition, while avoiding the false assumption that authenticated ambient
summary traffic should disappear immediately after a bounded detailed readback.

Alternative considered:

- only restart passive listeners and keep the old acceptance oracle

Why rejected:

- restarting listeners helps local observability but still leaves the proof
  weaker than the actual packet-path claim if capture quiet is never rechecked

### 2. Hosted bounded-readback fallback stays allowed, but it must no longer make later quiet checks vacuous.

The bounded-search fallback remains useful because the branch intentionally
stopped relying on one long-lived passive `channels.log`. The hardening change
therefore keeps the fallback, requires the command-specific capture to stay
bounded, and restores observer-state handling so later switch-close acceptance
still has live proof surfaces.

Alternative considered:

- remove the bounded-search fallback entirely and require passive listener
  freshness only

Why rejected:

- that would regress the same same-change decision to avoid over-trusting one
  long-running passive listener

### 3. Target secure-auth handshake tracking will use source-aware progress state and bounded late-challenge recovery.

The proof needs to accept the next handshake from wire capture or native packet
log, depending on which surface actually observed it first. A single integer
count is insufficient because it can pin later waits to the wrong source. The
follow-up will track per-source progress for each handshake type and return the
first source that truly advanced beyond the last seen state.

When the maintained target journal has already confirmed
`SECURE_AUTH_CHALLENGE_ISSUED` on the active ingress but the corresponding
challenge packet has not yet appeared on the maintained packet-path artifacts,
the proof will wait one bounded grace window before sending another
`REQ_AUTH`. This keeps the proof aligned with the existing path identity while
avoiding proof-created duplicate request churn on the UHF primary re-auth step.

Alternative considered:

- keep one preferred source for S-band and only use native logs when the wire
  source is completely empty

Why rejected:

- that is the exact failure mode the review identified

Alternative considered:

- immediately resend `REQ_AUTH` whenever the packet-path challenge wait times
  out, even if the target journal already confirms that a challenge was issued

Why rejected:

- the fresh failing target proof showed that this behavior creates duplicate
  `REQ_AUTH` traffic and `UnexpectedSequenceCount` noise on the maintained UHF
  uplink path even though the runtime challenge/response math is correct

## Risks / Trade-offs

- [More oracle strictness may expose real residual timing issues] → Mitigation:
  rerun the fresh hosted/target probes and the full target secure-auth proof on
  the fresh local build before archive.
- [Source-aware handshake state could still mishandle duplicate-source replay if
  implemented loosely] → Mitigation: keep explicit per-source progress state and
  verify the maintained full target secure-auth proof, not only the smaller
  preflight.
- [Late challenge handling could accidentally widen the proof into a new retry
  policy] → Mitigation: keep the grace window bounded, require journal evidence
  that a challenge was already issued on the maintained ingress, and leave the
  product secure-auth contract unchanged.
- [Follow-up change could drift into product-governance rewrite] → Mitigation:
  keep capability scope limited to verification evidence/registry wording plus
  probe helper code.
