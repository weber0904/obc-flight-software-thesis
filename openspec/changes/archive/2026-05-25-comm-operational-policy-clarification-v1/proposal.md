## Why

The active COMM baseline already exposes enough current truth to constrain UHF
session use, beacon behavior, retry ownership, gateway limits, and live-versus-
stored observability, but that truth is still split across evidence records,
current docs, and partial spec wording. This change closes those policy
ambiguities before `reliable-transfer-v1`, UHF follow-up, or dual-link work
accidentally expand from unclear assumptions.

## What Changes

- Freeze the current UHF command-session operational boundary at accepted
  `SESSION_OPEN(seq0)`, while keeping link acquisition as transport fact and
  later command success as follow-on operational confirmation rather than the
  policy-entry boundary itself.
- Freeze `CommController` as the owner of beacon suppress/resume policy, with
  suppress starting at accepted UHF `SESSION_OPEN(seq0)` and resuming after a
  bounded inactivity timeout.
- Keep the beacon suppress policy explicitly clarification-only in this change:
  the governing rule is formalized, but current hosted/target baseline does not
  yet claim session-aware suppress runtime implementation or proof.
- Clarify `uhf-backup` versus `uhf-primary-after-failover` as separate policy
  roles, with UHF primary authority remaining a post-switch state rather than a
  cold-start bootstrap profile.
- Clarify that bounded whole-command retries remain ground/probe helper
  behavior only; packet retry, ARQ/NACK, reliable transfer, and CFDP remain
  future work.
- Clarify that the current gateway and ground orchestration model is single
  active relay path plus explicit switch, not simultaneous S-band/UHF relay or
  gateway-local multiplexing.
- Clarify current observability boundaries: command responses, bounded
  telemetry/events, and live beacon are live operational visibility; official
  `.fdp` is stored mission history; logs, journal, captures, and hosted status
  dumps remain diagnostics-only.
- Update formal specs, current docs, the verification-path registry wording,
  and the formal COMM matrix runbook so current narrative truth and formal
  policy wording align.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: freeze the three-layer COMM policy model, the UHF
  `SESSION_OPEN(seq0)` policy-entry boundary, the beacon-policy owner, the
  current single-path relay truth, and the current retry/observability
  boundaries
- `ground-ttc-gateway`: tighten the current gateway non-claims so it is not
  described as an authority owner, dual-link multiplexer, or reliable-transfer
  engine, and keep retry ownership on the ground/probe side
- `target-comm-node56-migration`: clarify `uhf-backup` versus
  `uhf-primary-after-failover` and keep UHF primary authority as explicit-switch
  semantics only
- `live-beacon-broadcast`: keep live beacon as no-ACK broadcast and record that
  suppress/resume policy belongs to COMM policy rather than beacon-local
  broadcast emission
- `onboard-data-products-and-live-beacon`: keep live-versus-stored state roles
  explicit and distinguish operational visibility from diagnostics-only
  observability
- `interface-contract-index`: require `docs/interfaces.md` to summarize the
  clarified UHF role, retry, gateway, and observability policy boundaries
- `verification-path-registry`: clarify how existing registered paths support
  the policy wording without implying new transport proof
- `verification-evidence`: clarify that this slice reuses existing evidence and
  must keep current non-claims explicit rather than restating new proof

## Impact

- Affected specs: `comm-subsystem`, `ground-ttc-gateway`,
  `target-comm-node56-migration`, `live-beacon-broadcast`,
  `onboard-data-products-and-live-beacon`, `interface-contract-index`,
  `verification-path-registry`, and `verification-evidence`
- Affected docs: `docs/architecture/current-development-architecture.md`,
  `docs/interfaces.md`, `docs/roadmap/current-baseline.md`,
  `docs/roadmap/next-work.md`, `docs/architecture/comm-followup-directions.md`,
  `evidence/verification-path-registry.md`, and
  `docs/operator/formal-comm-verification-matrix-v1-runbook.md`
- Affected systems: hosted S-band node `5`, hosted UHF node `6`, target/lab
  node `5`, target/lab bounded quiet node `6`, and the current
  `ground_ttc_gateway` operational model
- Runtime/code impact: no new public runtime API, command, wire format, or
  session primitive is planned; code changes are out of scope unless a wording
  mismatch makes the checked-in spec/doc set incoherent
