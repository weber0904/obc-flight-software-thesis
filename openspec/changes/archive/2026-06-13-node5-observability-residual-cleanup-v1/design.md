## Context

The first node-`5` S-band tier-selection slice already narrowed the scheduled
family set to curated keep-live summary and moved detailed family observation
behind fresh `GET_*` readback. What remained open was the residual runtime
surface outside those selected families: `SystemResources`, transport facts,
queue counters, owner counters, egress counters, and a mix of COMM policy and
COMM internal telemetry/events.

That residual set cannot stay under one vague `non-baseline live` label
because current branch truth now has four distinct observability roles:

1. formal pass-time keep-live truth
2. formal reviewable proof / transport / policy observability
3. bounded fresh readback / review command
4. diagnostics-only / non-baseline live

The branch also already has the owned replacement for formal CPU/RSS truth:
`WatchdogSupervisor` `SYS_*` surfaces under `core-system-contracts`. The
remaining work is therefore a narrow boundary clarification, not a runtime
redesign.

Fresh hosted evidence on this branch adds one adjacent fact that this same
change must now own: ambient post-auth summary and reviewable residual surfaces
are still visible, but the representative detailed `GET_*` proof oracle is no
longer trustworthy as written. A fresh hosted run observed:

- `SystemResources.*`, queue depths, `UART_*`, and `UHF_SUPPRESSED_*` still
  present on the node-`5` live surface
- `SYS_*`, `GROUND_LINK_TX_BYTES`, `GROUND_LINK_HEALTH_S_BAND_*`, and S-band
  egress counters still present as intended reviewable or keep-live surfaces
- `EPS_STATUS_RECEIVED` after authenticated `EPS_GET_STATUS`
- no matching passive ground-side `EPS_IBAT` line in the existing hosted proof
  oracle

That means this change now has two deliverables inside the same scope:

1. residual governance inventory and wording closure
2. same-change requalification of the representative detailed `GET_*`
   packetized proof path if the issue is real runtime drift or stale oracle

## Governance Model

### Resource truth

- Formal node-`5` resource keep-live truth:
  - `SYS_CPU_USAGE`
  - `SYS_MEM_RSS_MB`
  - `SYS_RESOURCE_DEGRADED`
  - `SYS_LOW_MEMORY`
- Supplemental but not pass-time truth:
  - `SystemResources.*`
- Governed runtime-config surface retained:
  - `SystemResources.ENABLE`

Rationale:

- `SYS_*` is already the owned operator-visible CPU/RSS review surface under
  `WatchdogSupervisor`.
- `SystemResources` remains a valid integrated runtime service, but its broad
  live telemetry cannot keep competing with the formal node-`5` operator truth
  after the tier-selection work.

### Transport and queue truth

- Formal reviewable transport/proof observability:
  - `GROUND_LINK_UP`
  - `GROUND_LINK_DOWN`
  - `GROUND_LINK_TX_BYTES`
  - `GROUND_LINK_TX_ERRORS`
  - `GROUND_LINK_RX_ERRORS`
  - `GROUND_LINK_HEALTH_S_BAND_AVAILABLE`
  - `GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS`
  - `GROUND_LINK_HEALTH_S_BAND_REASON`
  - `ComCcsds.comQueue.QueueOverflow`
  - `OBCComCcsds.comQueue.QueueOverflow`
  - `CSP_OWNER_TIMEOUT(...)`
  - `CSP_OWNER_TOTAL_TIMEOUTS`
  - `SBAND_ROUTED_EVENT_PACKETS`
  - `SBAND_ROUTED_TLM_PACKETS`
  - `SBAND_SUPPRESSED_EVENT_PACKETS`
  - `SBAND_SUPPRESSED_TLM_PACKETS`

- Diagnostics-only / non-baseline live:
  - `GROUND_LINK_MODE`
  - `GROUND_LINK_CONNECTED`
  - `GROUND_LINK_TX_CHUNKS`
  - `GROUND_LINK_RX_CHUNKS`
  - `GROUND_LINK_RX_BYTES`
  - `GROUND_LINK_ERROR`
  - `GROUND_LINK_HEALTH_UHF_*`
  - `comQueueDepth`
  - `buffQueueDepth`
  - `CSP_OWNER_QUEUE_DEPTH`
  - `CSP_OWNER_INFLIGHT`
  - `CSP_OWNER_TOTAL_COALESCED`
  - `CSP_OWNER_LAST_LATENCY_USEC`
  - `CSP_OWNER_LAST_RESULT`
  - `UART_*`
  - currently emitted `UHF_SUPPRESSED_*`

Rationale:

- Existing COMM TT&C and target/lab evidence already require reviewable
  `GROUND_LINK_TX_BYTES` and transport-error growth, so those cannot be swept
  into diagnostics-only.
- `QueueOverflow` is a formal backpressure evidence boundary in current timing
  and stability records.
- S-band egress counters are the smallest reviewable proof surface for
  auth-gated live curation; they are not pass-time summary truth, but they are
  not mere noise either.

### COMM residual truth

- Formal pass-time truth:
  - `COMM_BAND_SWITCH`
  - `COMM_PRIMARY_LINK_CHANGED`
  - `COMM_LINK_AVAILABILITY_CHANGED`
  - `COMM_DOWNLINK_STATE_CHANGED`
  - `COMM_RECOVERY_FAILOVER_RESULT`
  - `COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED`
  - current primary-band state telemetry:
    `COMM_ACTIVE_BAND`, `COMM_PRIMARY_COMMAND_LINK`,
    `COMM_PRIMARY_TELEMETRY_LINK`, `COMM_PRIMARY_FILE_LINK`,
    `COMM_S_BAND_LIVE_OBSERVABILITY_ACTIVE`,
    `COMM_S_BAND_LIVE_OBSERVABILITY_REASON`

- Formal reviewable policy observability:
  - availability and age surfaces:
    `COMM_S_BAND_AVAILABLE`, `COMM_UHF_AVAILABLE`,
    `COMM_S_BAND_ACTIVITY_AGE_TICKS`, `COMM_UHF_ACTIVITY_AGE_TICKS`,
    `COMM_S_BAND_AVAILABILITY_REASON`, `COMM_UHF_AVAILABILITY_REASON`
  - downlink/overflow-adjacent state:
    `COMM_DOWNLINK_ACTIVE_OWNER`, `COMM_DOWNLINK_PENDING_OWNER`,
    `COMM_DOWNLINK_REJECT_TOTAL`
  - FDIR / failover counters and status:
    `COMM_SESSION_REVOKE_TOTAL`, `COMM_FDIR_FAULT_LATCHED`,
    `COMM_FDIR_FAULT_KIND`, `COMM_FDIR_CONSEC_PRIMARY_UNAVAILABLE`,
    `COMM_FDIR_CONSEC_PRIMARY_TRANSPORT`,
    `COMM_RECOVERY_FAILOVER_TOTAL`, `COMM_RECOVERY_OWNER_CLEAR_TOTAL`
  - detailed S-band live-gate state:
    `COMM_S_BAND_LIVE_OBSERVABILITY_INGRESS_PORT`,
    `COMM_S_BAND_LIVE_OBSERVABILITY_ROLE`,
    `COMM_S_BAND_LIVE_OBSERVABILITY_SESSION_ID`,
    `COMM_S_BAND_LIVE_OBSERVABILITY_LAST_SEQUENCE`

- Diagnostics-only / non-baseline live:
  - pass counters:
    `COMM_PASS_ACTIVE`, `COMM_PASS_REMAINING`, `COMM_TOTAL_PASSES`,
    `COMM_PASS_START`, `COMM_PASS_END`
  - reliable-transfer internals:
    `COMM_RT_*`
  - UHF beacon suppress internals on this node-`5` residual pass:
    `COMM_UHF_BEACON_SUPPRESS_*`,
    `COMM_UHF_BEACON_SUPPRESS_STARTED`,
    `COMM_UHF_BEACON_SUPPRESS_REFRESHED`,
    `COMM_UHF_BEACON_SUPPRESS_CLEARED`

Rationale:

- The pass-time operator truth must stay focused on active-link state and the
  node-`5` live-gate boundary.
- Reviewable policy observability keeps the supporting counters/status needed
  to explain transport-growth, failover, and downlink decisions without
  promoting every counter into keep-live truth.
- Reliable-transfer and UHF suppress internals are not part of the current
  node-`5` residual closure goal.

## Exact Residual Inventory

### `SystemResources`

| Surface | Owner/component | Current exposure | Final bucket | Replacement / boundary | Reason |
|---|---|---|---|---|---|
| `OBCApp.systemResources.MEMORY_TOTAL` | `Svc.SystemResources` | live tlm | diagnostics-only | `SYS_MEM_RSS_MB` + `STORAGE_*` | broad hosted runtime stat; not formal pass-time truth |
| `OBCApp.systemResources.MEMORY_USED` | `Svc.SystemResources` | live tlm | diagnostics-only | `SYS_MEM_RSS_MB` | same |
| `OBCApp.systemResources.NON_VOLATILE_TOTAL` | `Svc.SystemResources` | live tlm | diagnostics-only | `STORAGE_*` / `STORAGE_GET_STATUS` | storage truth already lives elsewhere |
| `OBCApp.systemResources.NON_VOLATILE_FREE` | `Svc.SystemResources` | live tlm | diagnostics-only | `STORAGE_*` / `STORAGE_GET_STATUS` | same |
| `OBCApp.systemResources.CPU` | `Svc.SystemResources` | live tlm | diagnostics-only | `SYS_CPU_USAGE` | broad runtime stat; superseded by owned `SYS_*` |
| `OBCApp.systemResources.CPU_00` .. `CPU_15` | `Svc.SystemResources` | live tlm | diagnostics-only | none; supplemental review only | per-core detail is not pass-time operator truth |
| `OBCApp.systemResources.ENABLE` | `Svc.SystemResources` | command | governed runtime config | `core-system-contracts` | stays writable only through current authority boundary |

### `GroundLinkDriver` / `GroundLinkHealthProvider`

| Surface | Owner/component | Current exposure | Final bucket | Replacement / boundary | Reason |
|---|---|---|---|---|---|
| `GROUND_LINK_UP`, `GROUND_LINK_DOWN` | `GroundLinkDriver` | live events | reviewable proof / transport | none | required by current ground-link proof/readiness interpretation |
| `GROUND_LINK_ERROR` | `GroundLinkDriver` | live event | diagnostics-only | target journals / driver diagnostics | too low-level to be pass-time truth |
| `GROUND_LINK_MODE`, `GROUND_LINK_CONNECTED` | `GroundLinkDriver` | live tlm | diagnostics-only | policy surfaces on `CommController` | raw backend state only |
| `GROUND_LINK_TX_CHUNKS`, `GROUND_LINK_RX_CHUNKS` | `GroundLinkDriver` | live tlm | diagnostics-only | none | transport detail not used as baseline proof |
| `GROUND_LINK_TX_BYTES` | `GroundLinkDriver` | live tlm | reviewable proof / transport | none | required by current TT&C / file-downlink evidence |
| `GROUND_LINK_RX_BYTES` | `GroundLinkDriver` | live tlm | diagnostics-only | none | not a current baseline proof oracle |
| `GROUND_LINK_TX_ERRORS`, `GROUND_LINK_RX_ERRORS` | `GroundLinkDriver` | live tlm | reviewable proof / transport | none | transport-error growth remains reviewable |
| `GROUND_LINK_HEALTH_S_BAND_AVAILABLE` | `GroundLinkHealthProvider` | live tlm | reviewable proof / availability | none | explains provider-owned availability |
| `GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS` | `GroundLinkHealthProvider` | live tlm | reviewable proof / availability | none | same |
| `GROUND_LINK_HEALTH_S_BAND_REASON` | `GroundLinkHealthProvider` | live tlm | reviewable proof / availability | none | same |
| `GROUND_LINK_HEALTH_UHF_AVAILABLE`, `GROUND_LINK_HEALTH_UHF_ACTIVITY_AGE_TICKS`, `GROUND_LINK_HEALTH_UHF_REASON` | `GroundLinkHealthProvider` | live tlm | diagnostics-only / unchanged | bounded UHF docs/evidence elsewhere | out of this node-`5` cleanup scope |

### `ComCcsds` / `OBCComCcsds`

| Surface | Owner/component | Current exposure | Final bucket | Replacement / boundary | Reason |
|---|---|---|---|---|---|
| `ComCcsds.comQueue.comQueueDepth` | `ComCcsds` | live tlm | diagnostics-only | none | internal queue detail |
| `ComCcsds.comQueue.buffQueueDepth` | `ComCcsds` | live tlm | diagnostics-only | none | same |
| `ComCcsds.comQueue.QueueOverflow` | `ComCcsds` | live event | reviewable backpressure proof | none | current timing/stability evidence depends on it |
| `OBCComCcsds.comQueue.comQueueDepth` | `OBCComCcsds` | live tlm | diagnostics-only | none | internal queue detail |
| `OBCComCcsds.comQueue.buffQueueDepth` | `OBCComCcsds` | live tlm | diagnostics-only | none | same |
| `OBCComCcsds.comQueue.QueueOverflow` | `OBCComCcsds` | live event | reviewable backpressure proof | none | same |

### `CspRuntimeOwner`

| Surface | Owner/component | Current exposure | Final bucket | Replacement / boundary | Reason |
|---|---|---|---|---|---|
| `CSP_OWNER_QUEUE_DEPTH` | `CspRuntimeOwner` | live tlm | diagnostics-only | none | success-path detail only |
| `CSP_OWNER_INFLIGHT` | `CspRuntimeOwner` | live tlm | diagnostics-only | none | same |
| `CSP_OWNER_TOTAL_TIMEOUTS` | `CspRuntimeOwner` | live tlm | reviewable owner-timeout proof | none | timeout growth matters for operator review |
| `CSP_OWNER_TOTAL_COALESCED` | `CspRuntimeOwner` | live tlm | diagnostics-only | none | internal efficiency counter |
| `CSP_OWNER_LAST_LATENCY_USEC` | `CspRuntimeOwner` | live tlm | diagnostics-only | none | internal timing detail |
| `CSP_OWNER_LAST_RESULT` | `CspRuntimeOwner` | live tlm | diagnostics-only | none | same |
| `CSP_OWNER_TIMEOUT(...)` | `CspRuntimeOwner` | live event | reviewable owner-timeout proof | none | explicit timeout event remains reviewable |

### `CommEgressMux`

| Surface | Owner/component | Current exposure | Final bucket | Replacement / boundary | Reason |
|---|---|---|---|---|---|
| `SBAND_ROUTED_EVENT_PACKETS` | `CommEgressMux` | live tlm | reviewable egress proof | none | proves curated node-`5` S-band egress decisions |
| `SBAND_ROUTED_TLM_PACKETS` | `CommEgressMux` | live tlm | reviewable egress proof | none | same |
| `SBAND_SUPPRESSED_EVENT_PACKETS` | `CommEgressMux` | live tlm | reviewable egress proof | none | same |
| `SBAND_SUPPRESSED_TLM_PACKETS` | `CommEgressMux` | live tlm | reviewable egress proof | none | same |
| current emitted `UHF_SUPPRESSED_EVENT_PACKETS`, `UHF_SUPPRESSED_TLM_PACKETS` | `CommEgressMux` | live tlm | diagnostics-only / unchanged | bounded UHF policy evidence elsewhere | do not widen UHF baseline here |

### `UartDriver`

| Surface | Owner/component | Current exposure | Final bucket | Replacement / boundary | Reason |
|---|---|---|---|---|---|
| `UART_OPEN`, `UART_ERROR` | `UartDriver` | live events | diagnostics-only | UART journals / probe diagnostics | raw transport noise, not node-`5` operator truth |
| `UART_TX_BYTES`, `UART_RX_BYTES`, `UART_TX_ERRORS`, `UART_RX_ERRORS`, `UART_CONNECTED` | `UartDriver` | live tlm | diagnostics-only | none | retained as low-level diagnostics only |

### `CommController`

| Surface | Owner/component | Current exposure | Final bucket | Replacement / boundary | Reason |
|---|---|---|---|---|---|
| `COMM_BAND_SWITCH`, `COMM_PRIMARY_LINK_CHANGED`, `COMM_LINK_AVAILABILITY_CHANGED`, `COMM_DOWNLINK_STATE_CHANGED`, `COMM_RECOVERY_FAILOVER_RESULT`, `COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED` | `CommController` | live events | pass-time keep-live truth | none | current operator-facing state/transition truth |
| `COMM_ACTIVE_BAND`, `COMM_PRIMARY_COMMAND_LINK`, `COMM_PRIMARY_TELEMETRY_LINK`, `COMM_PRIMARY_FILE_LINK`, `COMM_S_BAND_LIVE_OBSERVABILITY_ACTIVE`, `COMM_S_BAND_LIVE_OBSERVABILITY_REASON` | `CommController` | live tlm | pass-time keep-live truth | none | current live node-`5` policy state |
| `COMM_S_BAND_AVAILABLE`, `COMM_UHF_AVAILABLE`, `COMM_S_BAND_ACTIVITY_AGE_TICKS`, `COMM_UHF_ACTIVITY_AGE_TICKS`, `COMM_S_BAND_AVAILABILITY_REASON`, `COMM_UHF_AVAILABILITY_REASON`, `COMM_DOWNLINK_ACTIVE_OWNER`, `COMM_DOWNLINK_PENDING_OWNER`, `COMM_DOWNLINK_REJECT_TOTAL`, `COMM_SESSION_REVOKE_TOTAL`, `COMM_FDIR_FAULT_LATCHED`, `COMM_FDIR_FAULT_KIND`, `COMM_FDIR_CONSEC_PRIMARY_UNAVAILABLE`, `COMM_FDIR_CONSEC_PRIMARY_TRANSPORT`, `COMM_RECOVERY_FAILOVER_TOTAL`, `COMM_RECOVERY_OWNER_CLEAR_TOTAL`, `COMM_S_BAND_LIVE_OBSERVABILITY_INGRESS_PORT`, `COMM_S_BAND_LIVE_OBSERVABILITY_ROLE`, `COMM_S_BAND_LIVE_OBSERVABILITY_SESSION_ID`, `COMM_S_BAND_LIVE_OBSERVABILITY_LAST_SEQUENCE` | `CommController` | live tlm | reviewable policy observability | none | supporting policy explanation for truth and failover decisions |
| `COMM_PASS_ACTIVE`, `COMM_PASS_REMAINING`, `COMM_TOTAL_PASSES`, `COMM_PASS_START`, `COMM_PASS_END` | `CommController` | live tlm/events | diagnostics-only | pass management remains adjacent | not current node-`5` pass-time truth |
| `COMM_RT_*` events and telemetry | `CommController` | live tlm/events | diagnostics-only | reliable-transfer evidence remains separate | reliable-transfer internals are not part of current residual cleanup |
| `COMM_UHF_BEACON_SUPPRESS_*` events and telemetry | `CommController` | live tlm/events | diagnostics-only / unchanged for node-`5` scope | bounded UHF operator docs/evidence | do not broaden UHF semantics here |

## Documentation And Runtime Boundaries

- No second command plane is introduced.
- No generic packet filter or central live-surface classifier is introduced.
- Cleanup stays owner-local and wording-local:
  - `WatchdogSupervisor` owns formal resource keep-live truth
  - `SystemResources` remains integrated but supplemental
  - `GroundLinkDriver` / `GroundLinkHealthProvider` own raw and derived
    transport facts
  - `CommController` owns policy truth, not raw transport truth
  - `ComCcsds` queue overflow remains a backpressure proof surface
  - `CommEgressMux` S-band counters remain egress-proof surfaces

## Same-Change Delivery Phases

This change now proceeds in four phases instead of assuming immediate closeout:

1. fresh inventory / drift diagnosis
   - capture current emitted node-`5` hosted and, if needed, target surfaces
   - classify each surface by owner, bucket, and implementation action
   - determine whether detailed `GET_*` drift is oracle-only or product-path
     drift
2. OpenSpec + canonical-doc alignment
   - update change artifacts, delta specs, main specs, `docs/interfaces.md`,
     runbooks, and verification registry wording so they match the fresh
     diagnosis
3. runtime / probe / proof-chain implementation
   - repair probes first if component ownership and packetized delivery path
     still match the intended summary/detail split
   - repair runtime only if the summary/detail split or live gate itself is no
     longer behaving as designed
4. final verification + closeout
   - rerun hosted/target proof, secure-auth regression, touched UTs, OpenSpec
     validation, then archive and reconcile

Until phase 4 completes, this change does not claim `local-ready`.

## Detailed GET_* Drift Handling

Representative detailed `GET_*` readback now follows these decision rules:

- If component code still performs summary/detail split and explicit `GET_*`
  still writes detailed telemetry, prefer proof-chain repair first.
- Proof-chain repair means:
  - keep the existing hosted/target wrapper names and registry path identities
  - stop relying only on passive long-running `channels.log`
  - add deterministic bounded channel observation after explicit authenticated
    `GET_*`
  - retain a passive ambient-summary oracle so the proof still catches broad
    live chatter regressions
- Runtime repair is only justified when:
  - component detailed publication no longer occurs at all, or
  - `CommController` / `CommEgressMux` policy suppresses the explicit detailed
    readback contrary to the intended contract

The target success condition is not "detailed fields stay forever readable as
ambient live". It is:

- detailed fields stay absent from ambient summary-only live traffic
- explicit authenticated `GET_*` produces one reviewable detailed observation
- the path does not regress into broad detailed chatter

## Verification

Phase 1 and phase 2 validation:

- fresh local verification gate may be run diagnostically before the final
  rerun, but it is not by itself the closeout gate
- fresh hosted logs are the current authoritative diagnosis input for residual
  presence and the existing detailed `GET_*` oracle drift
- target logs are added when hosted evidence is insufficient to distinguish
  proof drift from product-path drift

Phase 3 implementation validation:

- rerun the hosted node-`5` observability proof after probe/runtime changes
- rerun the target node-`5` observability proof after hosted proof is aligned
- rerun the target secure-auth control regression after any proof/runtime
  change touching the node-`5` authenticated path
- rerun touched UTs for any component or probe-support logic changed

Phase 4 closeout validation:

- fresh local verification gate per repo workflow
- aligned hosted and target node-`5` observability proofs
- secure-auth control regression
- `openspec validate node5-observability-residual-cleanup-v1`
- `openspec validate --specs`
- spec sync, archive, and post-archive reconciliation checks before declaring
  `local-ready`
