## Context

The repository is already operating with two different command-session stories:

- the maintained hosted and target operator baseline uses secure-auth plus
  secure command v2
- the codebase and some adjacent wrappers still retain legacy v1
  command-envelope / `SESSION_OPEN` surfaces

That split is now more than wording debt. The retained public legacy runtime
path means later work can still accidentally couple itself to a command-open
surface that current secure baseline does not use. At the same time, current
secure-auth still intentionally reuses internal runtime session-open/session-
revoke/session-sequence semantics, so the cleanup cannot be "delete every
session concept with SESSION in its name".

This change therefore retires the external legacy operator/runtime boundary
while preserving the internal secure-session semantics already synthesized by
secure-auth.

## Goals / Non-Goals

**Goals:**
- Remove public legacy `SESSION_OPEN` from the current runtime command surface.
- Fail closed on legacy command-envelope v1 lifecycle traffic before it can
  create session or sequence state.
- Preserve `authGranted -> opened-session` runtime synthesis plus existing
  `COMMAND_SESSION_OPENED`, `COMMAND_SESSION_REVOKED`, `SESSION_*`, and secure
  reject telemetry families.
- Re-qualify verification scripts before deciding which ones can still appear
  in closeout gates.
- Rewrite current specs/docs so secure-auth-only is the current maintained
  command baseline.

**Non-Goals:**
- No secure-auth redesign.
- No secure command v2 format redesign.
- No renaming of `COMMAND_SESSION_OPENED` or `SESSION_*` telemetry in this
  slice.
- No migration of timing wrappers onto a new secure-auth timing harness.
- No new Mission Console API or UI contract.

## Decisions

### Decision: Retire only the external legacy lifecycle boundary

We will remove the current public legacy v1 lifecycle surface
`OBCApp.commandIngressAuthority.SESSION_OPEN` and reject legacy lifecycle
traffic before dispatch. We will not remove internal runtime opened-session
evidence that secure-auth currently synthesizes and that current operator flows
still observe.

Why:
- current operator surfaces already bootstrap through secure-auth
- current runtime still needs a session/opened abstraction for suppress,
  observability, sequence accounting, and revoke handling
- renaming every internal surface in the same slice would add churn without
  changing baseline behavior

Alternative rejected:
- keep public `SESSION_OPEN` as a hidden compatibility path
  Rejected because it keeps the baseline contract ambiguous and leaves current
  runtime behavior coupled to a path the maintained operator baseline no longer
  uses.

### Decision: Legacy v1 fail-closed means zero runtime mutation

Legacy outer opcode / lifecycle traffic must now fail closed before it can:
- establish session
- consume sequence
- update reopen-floor or persistence state
- reach `Svc::CmdDispatcher`

Why:
- the retirement is only meaningful if legacy traffic stops mutating current
  runtime state
- partial rejection that still advances sequence or persistence would keep a
  hidden compatibility contract alive

Alternative rejected:
- leave sequence/persistence side effects but reject dispatch
  Rejected because it would preserve a live legacy contract under a different
  name.

### Decision: Verification paths are qualified before rerun

The change will classify candidate scripts into:
- current maintained secure-baseline gates
- supplemental / historical wrappers
- retired-now surfaces

Only scripts explicitly confirmed by current registry/runbook authority will be
allowed into local-ready.

Why:
- the repo already has many nearby wrappers, and several are historical,
  bounded, or superseded
- this change specifically removes one old boundary, so blindly rerunning every
  adjacent wrapper would produce false blockers

Alternative rejected:
- run all nearby wrappers and sort it out after failures
  Rejected because that recreates the exact drift the user wants avoided.

### Decision: Timing wrappers retire now instead of being migrated

`target_timing_empirical_ceiling_freeze_v1_probe` and
`target_timing_wcet_profile_proof_v1_probe` will be treated as retired
historical surfaces in current baseline docs/specs.

Why:
- the user explicitly decided timing should be rebuilt later rather than
  migrated here
- these probes are no longer suitable current closeout authority for this
  command-ingress retirement slice

Alternative rejected:
- secure-auth migrate the timing probes in this same tranche
  Rejected because it expands scope into a second proof family with different
  product questions.

### Decision: Current maintained secure baseline remains unchanged

The following current operator/runtime surfaces must keep working unchanged:
- manual secure ops
- Mission Console secure actions
- challenge-handshake secure-command path
- target secure-auth proof family
- S-band auth-gated observability

Why:
- these are the current maintained baseline surfaces the retirement is supposed
  to protect
- if any of them regress, the change is not acceptable regardless of how clean
  the legacy cleanup looks

## Risks / Trade-offs

- [Risk] Adjacent wrappers still import legacy helpers indirectly.
  → Mitigation: do the script qualification inventory first, then only rerun
  maintained secure-baseline probes.

- [Risk] Removing public `SESSION_OPEN` may break old dictionary or authority
  surfaces still referenced by docs/tests.
  → Mitigation: update authority catalog, policy JSON, UTs, and spec/doc
  wording in the same change.

- [Risk] Historical registry/evidence wording may still sound current even
  after runtime retirement.
  → Mitigation: update main specs and current docs together; archived evidence
  remains but is explicitly historical.

- [Risk] Timing path wording may still be cited as active because it already
  exists in current registry/docs.
  → Mitigation: explicitly downgrade timing wrapper authority in
  `verification-path-registry`, `verification-evidence`, and roadmap docs.

## Migration Plan

1. Create the formal change and write proposal/design/spec/task artifacts.
2. Inventory runtime/spec/doc/script references to legacy v1 and
   `SESSION_OPEN`.
3. Re-qualify verification scripts into maintained / supplemental / retired.
4. Remove public runtime `SESSION_OPEN` surface and fail-close legacy ingress.
5. Update UTs and authority/dictionary surfaces.
6. Update main specs and current docs to secure-auth-only truth.
7. Run fresh local gate, then only the re-qualified maintained secure-baseline
   probes.
8. Record evidence, validate OpenSpec, sync/archive, reconcile docs, and
   declare local-ready only after the worktree is clean.

## Open Questions

- Qualification result:
  - `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
  - `bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh`
  - `bash scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh`
  - `bash scripts/run_sband_observability_governance_hosted_probe.sh`
  - `bash scripts/run_target_secure_auth_command_path_probe.sh`
  - `bash scripts/run_target_secure_auth_proof.sh`
  are current maintained secure-baseline gates for this slice because current
  registry and operator runbooks still name them as the governing hosted/target
  secure-auth proof family.
- Qualification result:
  - `run_official_sequencing_system_resources_v1_probe.sh`
  - legacy QoS / old TT&C file-downlink wrappers
  - payload compat wrappers that still send public `SESSION_OPEN`
  - historical beacon-suppression / packet-quiet wrappers that still depend on
    legacy lifecycle wording
  are supplemental or historical only and SHALL NOT appear in this slice's
  current local-ready gate.
- Qualification result:
  - `target_timing_empirical_ceiling_freeze_v1_probe`
  - `target_timing_wcet_profile_proof_v1_probe`
  are retired-now timing surfaces for this change; the archived records remain
  reviewable but the wrappers are not current maintained closeout authority.
- Mission Console status:
  - repo-owned hosted and target probes exist, but current registry/runbook
    authority does not elevate Mission Console into the secure-baseline gate
    set for this retirement slice
  - treat Mission Console as adjacent maintained operator evidence, not a
    required closeout gate for this change
