## Context

The current baseline already proves several narrow COMM truths:

- `CommandIngressAuthority` accepts explicit `SESSION_OPEN(seq0)` and uses it as
  the only v1 session open/replace/resync surface.
- `uhf-backup` can open a session and continue allowlisted read/status traffic
  without gaining full authority.
- target/lab `uhf-primary` is only proven after explicit switch from default
  node `5`.
- `ground_ttc_gateway` is a raw relay with one northbound GDS connection and
  one southbound path.
- live beacon is a no-ACK broadcast surface, while official `.fdp` remains the
  only active stored mission-history path.
- the radio/link observability contract is frozen, but broader beacon/session
  arbitration and dual-link runtime remain future work.

What is still missing is one formal, reviewable policy layer that says how
those facts should be interpreted operationally. Without that layer, later
changes could quietly treat transport acquisition, `SESSION_OPEN`, first
successful command/response, retry behavior, dual-link coexistence, or debug
surfaces as if they were already settled current truth.

This change stays clarification-only. It does not implement session-aware
beacon suppression runtime, reliable transfer, or a new ground multiplexer.

## Goals / Non-Goals

**Goals:**

- freeze one explicit three-layer COMM policy model
- freeze accepted `SESSION_OPEN(seq0)` as the current UHF command-session
  policy-entry boundary
- freeze `CommController` as the owner of beacon suppress/resume policy
- freeze `uhf-backup` versus `uhf-primary-after-failover` as separate policy
  roles
- freeze current retry ownership as ground/probe whole-command helper behavior
- freeze current gateway/orchestration truth as single active relay path plus
  explicit switch
- freeze live operational visibility versus stored history versus
  diagnostics-only observability
- align current docs and formal spec wording to the same bounded policy truth

**Non-Goals:**

- no session-aware beacon suppress runtime implementation
- no reliable transfer, ARQ, NACK, CFDP, or packet retry design
- no gateway-local dual-link multiplexing implementation
- no COMM runtime rewrite or new link-state machine
- no RF closure
- no new transport proof beyond current evidence references
- no new runtime public API, wire format, or session primitive

## Boundary Model

The clarified model is intentionally split into three layers:

```text
1. Raw observation / transport fact
   - link acquisition
   - raw relay byte movement
   - node-5 / node-6 path identity
   - raw driver/provider/radio observations

2. Session / link-role runtime policy
   - accepted SESSION_OPEN(seq0) as command-session policy entry
   - sband-primary / uhf-backup / uhf-primary-after-failover role semantics
   - CommController ownership of suppress/resume policy
   - single active relay path plus explicit switch

3. Future reliable-transfer behavior
   - packet retry
   - ARQ / NACK / CFDP
   - reliable file transfer under loss
   - any broader link/runtime retry engine
```

The main rule is that layer `2` may depend on layer `1`, but layer `2` must
not silently inherit layer `3` guarantees.

## Decisions

### Decision: UHF policy-entry boundary is accepted `SESSION_OPEN(seq0)`

Current formal policy will treat accepted UHF `SESSION_OPEN(seq0)` as the point
where UHF enters a bounded command-session window. This is the earliest
existing repo truth that is:

- explicit in current specs
- backed by existing evidence
- specific to command-session policy rather than mere transport attachment

Transport acquisition remains a raw transport fact only. The first later
successful command/response remains useful operational confirmation, but it is
not the policy-entry boundary for this slice.

Alternative considered: use first successful command/response. Rejected because
it would make command-session policy depend on a later behavioral milestone than
the current formal lifecycle contract already proves.

### Decision: Beacon suppress/resume policy belongs to `CommController`

`BeaconPublisher` remains the bounded broadcast emitter. It does not become the
owner of COMM session arbitration. `CommController` owns the suppress/resume
policy because it already owns active link role, shared downlink state, and the
current COMM-facing role-policy boundary.

The policy rule is:

- suppress starts at accepted UHF `SESSION_OPEN(seq0)`
- suppress resumes after bounded inactivity timeout

This change records that rule as governing design truth only. It does not claim
that current hosted or target runtime already enforces it.

Alternative considered: leave suppress owner unclaimed. Rejected because the
user explicitly wanted the owner frozen now to prevent later follow-up work from
expanding on ambiguous ownership.

### Decision: `uhf-backup` and `uhf-primary-after-failover` remain distinct

The current repo already proves two separate UHF policy situations:

- `uhf-backup`: bounded backup ingress that may open a session and continue
  allowlisted read/status traffic only
- `uhf-primary-after-failover`: full-authority UHF role reached only after
  explicit switch away from default S-band

The target proof surface name `TARGET_COMM_PROFILE=uhf-primary` stays acceptable
as operator/probe shorthand, but formal wording will describe the runtime role
it exercises as `uhf-primary-after-failover`.

Alternative considered: collapse both into one generic “UHF primary” role.
Rejected because it would erase the explicit-switch boundary already frozen in
target and hosted evidence.

### Decision: Retry ownership remains ground-side and whole-command only

The only retry behavior this change will treat as current truth is bounded
ground/probe whole-command retry. That includes current probe wrappers that
resend a full command or rerun a bounded command step after timeout or absent
observation.

This change will explicitly not reinterpret those helpers as:

- packet retransmission
- link-layer retry
- runtime retry engine
- reliable transfer

Alternative considered: describe a future “narrow runtime retry slice” now.
Rejected because this clarification slice should close ambiguity, not pre-bake a
follow-up design that lacks current proof.

### Decision: Current relay/orchestration truth is single-path plus explicit switch

The current baseline supports one active ground relay path at a time, plus
explicit operator/runtime switching between S-band-default and UHF-primary
roles. It does not support simultaneous S-band/UHF relay inside one current
gateway process, and it does not prove simultaneous dual-link runtime on OBC.

Formal wording will therefore say:

- `ground_ttc_gateway` is not an authority owner
- `ground_ttc_gateway` is not a dual-link multiplexer
- `ground_ttc_gateway` is not a reliable-transfer engine
- simultaneous S-band/UHF relay requires either two relay instances or a future
  higher-level orchestrator, neither of which is current baseline truth

Alternative considered: only restate gateway-local limits. Rejected because the
policy ambiguity also exists at the broader current baseline level, not only at
the process-local gateway level.

### Decision: Live operational visibility stays separate from stored history and diagnostics

The clarified observability split is:

- live operational visibility:
  - command responses
  - bounded telemetry/events
  - live beacon
- official stored history:
  - `.fdp` products and governed catalog/downlink behavior
- diagnostics-only:
  - local logs
  - journal
  - raw captures
  - hosted status dumps
  - probe diagnostics

This preserves the current baseline truth that live surfaces are operationally
useful without confusing them with the only active stored mission-history path.

Alternative considered: treat telemetry/events as diagnostics-only. Rejected
because current formal and narrative docs already treat them as bounded
operator-visible live surfaces.

## Spec / Doc Strategy

The change will update only existing capabilities and current docs:

- formal specs freeze the policy truth and current non-claims
- current docs paraphrase that truth in reviewable baseline language
- verification wording cites existing evidence rather than implying a new proof
  run

No new capability is introduced because the repo already has the necessary
primitives, roles, and evidence surfaces; the gap is policy clarification, not
feature absence.

## Risks / Trade-offs

- **[Risk] Clarification wording could overstate implementation closure**
  → Mitigation: every beacon suppress statement must pair governing policy with
  explicit current non-claim for runtime enforcement.
- **[Risk] Session policy wording could be mistaken for reliable transfer**
  → Mitigation: keep retry, packet recovery, and reliable transfer in a
  dedicated future layer and repeat the exclusion in spec and docs.
- **[Risk] `uhf-primary` shorthand could obscure the post-switch boundary**
  → Mitigation: keep operator/probe shorthand but write formal policy as
  `uhf-primary-after-failover`.
- **[Risk] Observability wording could demote telemetry/events too far**
  → Mitigation: preserve them as live operational visibility while still
  excluding them from stored history and diagnostics-only surfaces.

## Open Questions

- None for this change. The user chose:
  - `SESSION_OPEN(seq0)` as the UHF command-session policy-entry boundary
  - `CommController` as beacon suppress/resume owner
  - bounded inactivity timeout as beacon suppress release rule
  - ground/probe whole-command retry as the only current retry ownership claim
  - single-path relay plus explicit switch as the current dual-link boundary
  - live-versus-stored-versus-diagnostics observability split as formal current
    truth
