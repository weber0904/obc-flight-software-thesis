## Context

The current COMM baseline already split byte/backend ownership from derived
health and policy. `GroundLinkDriver` owns low-level transport counters and
connection events, `GroundLinkHealthProvider` reduces raw observation into
availability and freshness, and `CommController` owns policy-facing link role,
downlink ownership, and COMM FDIR state. The remaining gap is not architectural
layering but contract clarity: raw counters are split between stats and
observation state, radio status caching has no frozen freshness or unavailable
semantics, and the checked-in docs do not yet present one reusable field-by-
field current truth.

The change must stay below reliable-transfer, RF claims, beacon/session policy
rewrite, and dual-link orchestration redesign. It must also avoid widening the
contract into onboard snapshot or beacon payloads because those would create a
larger public-state migration than the current review goal needs.

## Goals / Non-Goals

**Goals:**

- Freeze one bounded raw ground-link observation contract owned by
  `GroundLinkDriver`.
- Freeze one bounded cached radio observation contract owned by
  `RadioController`.
- Preserve `GroundLinkHealthProvider` as the sole derived-health owner and
  `CommController` as the sole policy/runtime owner.
- Make hosted runtime status and probe evidence show raw observation, derived
  health, and policy state separately.
- Prove the contract on hosted node-`5`, hosted node-`6`, hosted direct-TCP,
  and fresh target/lab default node-`5`.

**Non-Goals:**

- No RSSI/SNR-driven health or failover policy
- No SNR field
- No reliable-transfer, ARQ/NACK, CFDP, or file-protocol redesign
- No UHF beacon suppression, handshake, or dual-link runtime redesign
- No snapshot, reduced-state, or beacon-payload expansion
- No new verification-path registry entry unless a genuinely new reusable path
  is proven beyond the existing hosted/target identities

## Decisions

### Decision: Freeze raw ground-link observation separately from derived health

`GroundLinkObservationState` will become the complete raw driver/backend-facing
contract for reviewable transport observation. It will carry mode, configured
health semantics, connection state, chunk counters, byte counters, error
counters, and successful status-observation count. This keeps raw transport
truth in the byte/backend owner while avoiding the previous split where some raw
state lived only in `GroundLinkStats`.

Alternative considered: leave byte counters in `GroundLinkStats` only and keep
`GroundLinkObservationState` minimal. Rejected because the user asked for one
reusable current baseline truth, and operators should not need to merge two
overlapping structs to understand raw observation ownership.

### Decision: Keep `CommLinkHealthView` derived-only

`CommLinkHealthView` remains the provider-owned reduction surface. It will not
gain raw counters or radio signal fields. Availability, freshness ages,
transport-growth indication, and availability reason stay here because those are
already derived from the raw observation contract and are consumed by policy.

Alternative considered: move raw counters into `CommLinkHealthView` for one-stop
readback. Rejected because it would blur raw observation ownership and derived
health ownership again.

### Decision: Introduce cached radio observation with explicit freshness

`RadioController` will own a new runtime observation type that records whether a
sample exists, how old the cached sample is in fast-group ticks, the last poll
result, and the last successful raw status fields. Raw RSSI stays in this
contract only, with units frozen as `dBm`. Failed or unsupported radio status
polls will not silently reuse the last numeric RSSI as if it were fresh.

Alternative considered: keep only the existing live `RadioStatus` request path
and infer staleness from command success. Rejected because the contract needs a
reviewable cached-state interpretation for hosted and target baselines that do
not always have a real RF backend.

### Decision: Keep overflow-adjacent COMM scope narrow

This change will treat `CommController` downlink active owner, pending owner,
and reject total as the COMM-owned overflow-adjacent metrics in scope. Queue
depth and `QueueOverflow` remain documented as `ComCcsds`-owned adjacent
observability rather than being copied into the COMM-owned contract.

Alternative considered: pull queue depth into `CommController` runtime state.
Rejected because the queue is owned by `ComCcsds`, and duplicating it here would
broaden this change into queue-policy redesign rather than observability
clarification.

### Decision: Reuse existing path identities for proof

Hosted proof will use one dedicated probe script that exercises hosted node-`5`,
hosted node-`6`, and hosted direct-TCP fallback. Target proof will wrap the
existing service-managed node-`5` command-path probe and add assertions for the
new observability contract. This keeps the verification boundary aligned with
existing registered path identities.

Alternative considered: rerun fresh target node-`6` as part of the first slice.
Rejected because the user explicitly kept fresh target node-`6` out of scope for
the first reviewable version.

## Risks / Trade-offs

- [Risk] Hosted runtime status may diverge from component telemetry wording
  after the new fields are added. → Mitigation: derive hosted status directly
  from the frozen runtime observation getters and cover formatting in
  `HostedRuntime` tests.
- [Risk] Radio freshness semantics could accidentally imply RF truth on
  unsupported transports such as `transparent-passive`. → Mitigation: define an
  explicit unavailable-result enum and assert unsupported behavior in unit tests
  and evidence.
- [Risk] Adding raw byte counters to observation state could tempt later code to
  treat observation state as policy input. → Mitigation: keep policy code
  consuming `CommLinkHealthView` only and retain explicit spec language that
  `CommController` has no direct driver-type coupling.
- [Risk] Target proof could accidentally over-claim node-`6` or RF coverage if
  the evidence summary is vague. → Mitigation: name the exact node-`5`
  command-path boundary in the evidence record and restate the explicit non-
  claims.
