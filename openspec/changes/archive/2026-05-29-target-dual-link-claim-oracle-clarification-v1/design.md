## Context

The repository already froze the current hosted and target-adjacent surfaces
that the next target-bearing simultaneous work must build on:

- hosted layer-1 maintained per-band stock stacks in `43B`
- hosted layer-2 orchestration owner in `43C`
- default target node-`5` truth in `59`
- quiet target node-`6` command/file/failover truth in `60`
- quiet target node-`6` suppress/runtime truth in `60A`
- target TCP comparator truth in `66`
- direct target command-path adjacency in `67`
- oracle-bounded non-quiet diagnosis in
  `target-nonquiet-background-tm-stability-v1`

What remains ambiguous is not whether those paths exist. The ambiguity is which
subset of them the next target-bearing simultaneous claim should actually use,
which oracle is authoritative, whether noisy non-quiet `uhf-backup`
observability is a hard failure, and what later implementation-bearing proof
must not silently reopen.

This change therefore stays clarification-only. It does not add a new target
proof, new orchestration owner, runtime redesign, gateway redesign, or
reliable-transfer feature.

## Goals / Non-Goals

**Goals:**

- freeze one exact future target-bearing dual-link claim statement
- freeze one exact in-scope target-path family for that future proof
- freeze a target-first mixed oracle with explicit precedence
- freeze a dual-verdict result model that separates target truth from
  operator-observability truth
- freeze the exact PASS boundary, quiet fallback semantics, optional file
  adjunct boundary, and explicit non-claims
- align delta specs, main specs, and current docs to the same future boundary

**Non-Goals:**

- no new target-bearing simultaneous runtime proof
- no hosted orchestration redesign
- no packet-quiet or beacon-suppress redesign
- no gateway-local multiplexer or one-GDS aggregation design
- no UHF reliable-transfer redesign
- no RF, security/boot, payload, scheduler, or target-flight closure expansion
- no promotion of `target-nonquiet-background-tm-stability-v1` into a reusable
  simultaneous target path

## Decisions

### 1. The future claim is primary-led, not symmetric

The future target-bearing dual-link claim is not a claim that both links carry
simultaneous full-authority commands. It is a bounded proof family where:

- default node-`5` S-band remains the governing bootstrap and primary path
- non-quiet physical node-`6` under `uhf-backup` is exercised concurrently as a
  bounded adjunct only
- formal UHF command truth is judged only after explicit switch to
  `uhf-primary-after-failover`

This keeps the claim target-relevant without pretending the repository already
needs or wants a symmetric dual-authority runtime.

Alternative considered: require simultaneous full-authority command success on
both links. Rejected because it would overclaim beyond current authority roles,
would force `uhf-backup` to act like a second primary path, and would reopen
runtime scope before the claim boundary is even settled.

### 2. Non-quiet node-`6` stays in scope, but only as a non-gating adjunct

The future path family is:

| Phase | Surface | Formal role |
|---|---|---|
| A | default target node-`5` | governing bootstrap and primary target truth |
| B | non-quiet physical node-`6` under `uhf-backup` | concurrent adjunct only |
| C | explicit switched `uhf-primary-after-failover` node-`6` | formal UHF command truth |
| D | quiet node-`6` fallback | adjunct rescue if phase B command attempt fails |

Target TCP remains a comparator only. Direct target `OBC -> GDS` remains an
adjacent path only.

Alternative considered: quiet-only future claim. Rejected because it would
avoid the real target-side observability boundary the repository expects to
encounter next.

Alternative considered: mixed-family claim where quiet and non-quiet node-`6`
are both equal primary surfaces. Rejected because that would blur the current
difference between target truth, quiet control, and non-quiet observability.

### 3. The oracle is target-first and mixed, not ground-first

The future result model uses two formal verdicts:

| Verdict | Primary oracle | Meaning |
|---|---|---|
| `target-claim` | post-switch journal-first target command truth | whether the bounded target claim itself passed |
| `operator-observability` | ground events, channels, gateway captures, beacon/debug artifacts | whether operator-facing live observability was clean, degraded, or failed |

Ground-only surfaces cannot overturn successful target command truth by
themselves. They produce a separate formal observability result.

Alternative considered: ground-first oracle. Rejected because existing
non-quiet evidence already shows that ground noise can misclassify correct
target-side command completion.

Alternative considered: target-only verdict. Rejected because the repo still
needs an explicit formal place to record degraded operator observability rather
than burying it in diagnostics prose.

### 4. `uhf-backup` adjunct commands must stay role-valid

Phase B uses only `uhf-backup`-allowlisted low-authority read/status commands.
It is not allowed to test `uhf-backup` with a command that current authority
policy intentionally denies.

Alternative considered: use any representative command during the concurrent
adjunct window. Rejected because failure under a denied opcode would be a test
design error, not evidence about the future claim boundary.

### 5. Quiet fallback is adjunct rescue only

If the non-quiet node-`6` adjunct command attempt fails, quiet node-`6` may be
used as a rescue path for the UHF adjunct. That rescue:

- may help recover the UHF command-truth adjunct needed for the final target
  claim
- does not convert the non-quiet operator-observability problem into a solved
  non-quiet result
- does not let reviewers restate quiet rescue as non-quiet closure

Alternative considered: quiet fallback as diagnostic-only control. Rejected
because the user explicitly wants it available to rescue the bounded UHF adjunct
when non-quiet commandability degrades.

Alternative considered: quiet fallback as full non-quiet rescue. Rejected
because that would erase the distinction between quiet control and non-quiet
observability.

### 6. Official file/downlink continuity is adjunct-only

The future proof may record an optional official file/downlink adjunct result,
but it is not part of the main PASS boundary.

Alternative considered: make official file/downlink continuity mandatory for the
main PASS. Rejected because this clarification slice is about target-side
simultaneous claim/oracle boundaries first, not about widening the next proof
into a file/downlink closure bundle.

### 7. The later implementation-bearing proof must stay narrowly scoped

The later proof should implement only the harness, oracle, and path closure
needed for this exact family. It should not reopen:

- hosted layer-1 or layer-2 ownership
- packet quiet or beacon suppress semantics
- gateway ownership or second-GDS design
- reliable-transfer ownership

Runtime changes should be reopened only if the target-first oracle shows a real
product-side fault rather than an observability-only problem.

## PASS / FAIL Model

### Exact PASS boundary

| Result | Required conditions |
|---|---|
| `Target claim PASS` | Phase A target truth passes, and phase C switched `uhf-primary-after-failover` target truth passes; phase B may pass or may be rescued by phase D |
| `Target claim FAIL` | phase A fails, or phase C fails, or the UHF adjunct cannot be closed through non-quiet attempt plus quiet rescue |
| `Operator observability PASS` | the declared ground-side live surfaces are clean enough for the future proof's operator-facing minimum |
| `Operator observability DEGRADED` | target claim passes but one or more declared ground-side live surfaces remain noisy, partial, or classifier-only |
| `Operator observability FAIL` | the declared ground-side live surfaces are absent or unusable for the claimed operator-observability boundary |

### Explicit non-claims

- no simultaneous full-authority commands on both links
- no hard PASS requirement that non-quiet `uhf-backup` commandability itself be
  clean
- no one-stock-GDS heterogeneous upstream claim
- no one-gateway simultaneous multiplexer claim
- no UHF reliable-transfer redesign
- no RF claim
- no mandatory official file/downlink continuity inside the main PASS boundary
- no restatement of quiet fallback as non-quiet observability closure

## Risks / Trade-offs

- **[Risk] Reviewers may mistake the concurrent adjunct for symmetric dual-link command authority.**
  Mitigation: repeat that phase B is role-valid `uhf-backup` adjunct only, and
  phase C is the only formal UHF command-truth closure.
- **[Risk] Quiet fallback may be misread as non-quiet success.**
  Mitigation: keep quiet fallback explicitly labeled as adjunct rescue only in
  specs, docs, and final summary wording.
- **[Risk] Ground-side degraded observability may be collapsed back into a single PASS/FAIL.**
  Mitigation: freeze two formal verdicts and make operator observability its
  own result line.
- **[Risk] Future implementation may reopen unrelated hosted or reliable-transfer scope.**
  Mitigation: keep the later-proof recommendation explicit in current docs and
  specs.

## Migration Plan

1. Add proposal, design, tasks, and delta specs for the clarification slice.
2. Update the corresponding main specs with the same frozen future boundary.
3. Update current docs and runbooks so they cite the future target-bearing
   boundary consistently without claiming new proof.
4. Validate the OpenSpec change, touched main specs, and documentation
   consistency.
5. Leave the change as a clarification-only slice with no new verification path
   entry.

Rollback strategy: revert the clarification wording and delta specs together.
This change introduces no runtime behavior that requires a separate rollback
path.

## Open Questions

- None. The future claim shape, oracle precedence, adjunct rescue rule, and
  explicit non-claims are intentionally locked by this design.
