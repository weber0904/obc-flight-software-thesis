## Context

The current baseline already says three important things:

1. target node-`6` proof is bounded to quiet-path acceptance today
2. `ground_ttc_gateway` remains a raw relay, not an authority owner or
   orchestration layer
3. probe-owned quiet egress and journal-first acceptance were valid ways to
   close other target paths without claiming general non-quiet serial closure

The remaining ambiguity is therefore not "whether UHF exists" or "whether
beacon suppress works." It is whether non-quiet target CAN node-`6` failure is:

- only a polluted acceptance/oracle surface
- a real runtime or egress stability problem on the physical path
- or a mixed case that needs tighter ownership and evidence wording

## Goals / Non-Goals

**Goals**

- reproduce a governed target CAN node-`6` non-quiet case
- keep quiet node-`6` control evidence unchanged and adjacent
- compare target CAN non-quiet behavior against a supporting target TCP node-`6`
  comparator
- separate target side effect truth from ground observability truth
- classify the root cause as `oracle`, `runtime`, or `mixed`
- make only the smallest owner-correct implementation change required by that
  classification
- leave residual non-claims explicit if full non-quiet closure is still not
  achieved

**Non-Goals**

- no `comm-dual-link-orchestration-v1`
- no second GDS productization by default
- no gateway authority redesign
- no UHF reliable transfer, RF claim, or quiet-mode operator productization
- no reopening of `uhf-beacon-suppression-runtime-v1`,
  `radio-metrics-v1`, or `reliable-transfer-v1`

## Decisions

### Decision: Start from diagnosis, not orchestration

This change first proves or disproves a residual target node-`6` non-quiet
problem. It does not assume the answer is dual-link orchestration, second GDS,
or a nominal side channel.

### Decision: Quiet control remains separate

Existing quiet target CAN node-`6` cases remain authoritative controls and keep
their current meaning. The non-quiet diagnosis must use its own wrapper and its
own evidence record.

### Decision: `ground_ttc_gateway` remains a non-owner

Even if the diagnosis uses byte captures or two simultaneous ground-side views,
`ground_ttc_gateway` stays a raw relay. The change may improve proof surfaces,
but it must not move policy or authority into the gateway.

### Decision: Oracle-first closure wins by default

If target journal and command side-effect truth remain correct while ground
events/channels become noisy or ordering-sensitive, the default outcome is
oracle/tooling closure only. This change will then stop at proof/oracle,
evidence, and documentation boundaries.

### Decision: Runtime change requires owner-correct evidence

If non-quiet target CAN node-`6` fails even when journal-first oracle and byte
captures are used, the first owner candidate is the OBC egress/COMM runtime
boundary. The change must not jump straight to side-channel or gateway redesign
without evidence that the fault is primarily observability-only.

## Diagnosis Design

### 1. Repository-Owned Wrapper

Add a dedicated `scripts/run_target_can_node6_nonquiet_diagnosis_probe.sh`
wrapper that performs three governed runs:

1. quiet target CAN node-`6` control
2. non-quiet target CAN node-`6` diagnosis
3. target TCP node-`6` supporting comparator

The wrapper must keep artifacts for each run in separate subdirectories and
emit one combined summary with the paths to all truth surfaces.

### 2. Target CAN Matrix Helper Extension

Extend `scripts/comm_verification/lib/run_target_can_matrix_probe.py` with:

- a quiet command roundtrip mode for node-`6`
- a non-quiet diagnosis mode for node-`6`
- artifact capture for:
  - gateway byte captures
  - target journal snapshots
  - `events.log`
  - `channels.log`
  - optional beacon/debug capture when configured

The quiet and non-quiet modes must differ only at the egress-quieting boundary.
They must not redefine session, switch, or authority semantics.

### 3. Oracle Surfaces

For the non-quiet diagnosis, the wrapper must record at least:

- whether accepted `SESSION_OPEN(seq0)` was observed on ground or journal
- whether the follow-on node-`6` command completion was observed on ground or
  journal
- whether expected channel readback queries returned usable output
- whether gateway captures show bounded byte flow in both directions
- whether beacon/debug capture shows background traffic when enabled

The diagnosis artifact should summarize agreement or divergence between these
surfaces without over-classifying success from a single signal.

## Root-Cause Classification

### `oracle`

- quiet control passes
- target CAN non-quiet command side effects and target journal remain correct
- ground events/channels are noisy, ambiguous, delayed, or ordering-sensitive
- target TCP comparator is not required to fail

Chosen isolation boundary:
- proof/oracle only

Rejected boundaries:
- GDS and gateway are not owners; they remain tooling/relay surfaces
- OBC runtime is not changed because product behavior is still correct
- second GDS or side channel is unnecessary in this change

### `runtime`

- quiet control passes
- target CAN non-quiet fails even when journal-first oracle and byte capture are
  used
- supporting comparator indicates the residual is specific to the physical
  target CAN node-`6` path rather than the whole higher-level policy stack

Chosen isolation boundary:
- narrowest owner-correct OBC egress or COMM runtime surface shown by the
  evidence

Rejected boundaries:
- `ground_ttc_gateway` remains raw relay only
- second GDS and side-channel-only fixes would hide a real product fault

### `mixed`

- quiet control passes
- target CAN non-quiet shows a real degraded path
- ground observability is also polluted enough that a ground-only oracle would
  misclassify or hide the fault

Chosen isolation boundary:
- product fix at the proven owner boundary plus proof/oracle correction for the
  diagnostic surface

Rejected boundaries:
- gateway/GDS still do not become policy owners
- second GDS remains deferred unless later work needs a debug-only side channel
  after the runtime boundary is already honest

## Alternatives / Trade-offs

### Second GDS / debug-only side channel

- useful only if the evidence shows the main remaining problem is observability
  pollution rather than product behavior
- rejected as a default implementation path for this change

### Gateway-side split

- could help visibility, but the gateway is still not the runtime owner
- rejected unless the change needs extra diagnostic capture only

### OBC egress suppression or isolation

- correct only if evidence shows the harm originates in runtime egress
- accepted as a possible minimal product fix, but not preselected

### Node-`6` side-channel-only capture

- useful as supporting debug visibility
- not promoted to nominal TT&C truth and not sufficient by itself to claim
  runtime closure

## Current Diagnosis Result

Fresh branch-local evidence now freezes these answers:

- root-cause class: `oracle`
- formal acceptance boundary: post-switch journal-first target command truth
- diagnostic-only side surfaces: ground readback/events, beacon capture, and
  byte captures stay supporting truth, not acceptance truth
- separate environment blocker: unhealthy CAN transport baseline can still
  invalidate runs, but that is kept distinct from the non-quiet node-`6`
  diagnosis verdict

Evidence summary:

- target TCP node-`6` comparator passes
- target CAN quiet-control can pass after removing the old pre-switch
  `CSP ping node 6 success` gate
- target CAN non-quiet now also passes a governed same-session UHF command
  under background TM on the physical serial plus CAN path
- the old non-quiet beacon-growth hard-fail oracle produced a false negative
- subsystem ingress diagnostics and ground checksum noise confirm observability
  pollution remains real even when the target command completes

Current implementation decision:

- keep this change at probe/tooling/evidence scope
- do not claim a second GDS is needed
- do not land a product runtime fix because fresh evidence no longer justifies
  one for this diagnosis
