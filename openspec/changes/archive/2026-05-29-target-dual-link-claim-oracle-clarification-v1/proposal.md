## Why

The repository now has a frozen hosted layer-1 simultaneous operator baseline
(`43B`), a hosted-only layer-2 orchestration owner (`43C`), a governed default
target node-`5` path (`59`), bounded quiet target node-`6` paths (`60` and
`60A`), a target TCP comparator (`66`), and a direct target command-path
adjacency (`67`). What it still does not have is one exact formal statement for
the next target-bearing simultaneous claim: which target path is actually in
scope, which oracle wins when ground observability is noisy, what PASS means,
and which adjacent behaviors stay explicit non-claims.

That gap is now a review and implementation risk. Future target-bearing work
should not reopen already-frozen hosted orchestration, UHF quiet semantics,
transport ceilings, or reliable-transfer ownership just because the later proof
target was left ambiguous.

## What Changes

- Freeze the future target-bearing dual-link boundary as a clarification-only
  slice before any implementation-heavy runtime or orchestration work starts.
- Define the exact future claim as a bounded target proof family:
  - default node-`5` S-band truth remains the governing bootstrap and primary
    path
  - non-quiet physical node-`6` under `uhf-backup` is a concurrent adjunct
    only, not a hard PASS gate
  - formal UHF command truth is judged only after explicit switch to
    `uhf-primary-after-failover`
  - quiet node-`6` fallback may rescue the UHF adjunct if the non-quiet
    command attempt fails
- Freeze a target-first mixed oracle with two formal verdicts:
  - `target-claim`
  - `operator-observability`
- Freeze the exact PASS boundary, explicit non-claims, and later-proof target
  without claiming that the repository has already demonstrated a
  target-bearing simultaneous path.
- Add delta specs and current-doc wording that bind reused evidence correctly
  and keep quiet, non-quiet, hosted, target TCP, and direct-target surfaces
  distinct.
- Keep this change evidence-light on purpose: it proves no new verification
  path, adds no new target runtime feature, and reuses existing evidence only
  for the exact boundaries already proven.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: freeze the future target-bearing dual-link claim as a
  primary-led, switch-closed proof family with role-valid `uhf-backup` adjunct
  command selection.
- `ground-ttc-gateway`: keep hosted layer-1 and layer-2 surfaces separate from
  the future target-bearing claim and keep one-GDS or one-gateway shortcuts out
  of scope.
- `interface-contract-index`: require `docs/interfaces.md` to summarize the
  frozen future boundary and dual-verdict model without restating it as current
  proof.
- `verification-evidence`: require target-first mixed-oracle wording, dual
  verdicts, quiet fallback as adjunct rescue only, and explicit “no new path
  proven” wording for clarification-only closeout.
- `verification-path-registry`: add citation guidance for the frozen future
  claim boundary without registering a new simultaneous target path.

## Impact

- Affected formal artifacts:
  - `proposal.md`, `design.md`, `tasks.md`
  - delta specs for the five modified capabilities above
- Affected main specs:
  - `openspec/specs/comm-subsystem/spec.md`
  - `openspec/specs/ground-ttc-gateway/spec.md`
  - `openspec/specs/interface-contract-index/spec.md`
  - `openspec/specs/verification-evidence/spec.md`
  - `openspec/specs/verification-path-registry/spec.md`
- Affected current docs:
  - current architecture, current baseline, next-work, interfaces, target and
    hosted runbooks, and registry citation notes
- Runtime/code impact:
  - none by default; this change is clarification-only and does not add a new
    proof path, orchestrator, runtime feature, gateway multiplexer, or
    reliable-transfer redesign
