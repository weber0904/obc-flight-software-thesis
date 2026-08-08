## Context

The checked-in F Prime v4.1.0 `Svc::FprimeRouter` routes command, file, and unknown packets to separate output ports. The active OBC topologies currently connect:

```text
ComCcsds.fprimeRouter.commandOut -> CdhCore.cmdDisp.seqCmdBuff
CdhCore.cmdDisp.seqCmdStatus -> ComCcsds.fprimeRouter.cmdResponseIn

ComFprime.fprimeRouter.commandOut -> CdhCore.cmdDisp.seqCmdBuff
CdhCore.cmdDisp.seqCmdStatus -> ComFprime.fprimeRouter.cmdResponseIn
```

`Svc::CommandDispatcher` matches `seqCmdBuff` and `seqCmdStatus` port numbers and returns the original `Fw.Com.context` value as the command status context. Any gate inserted between router and dispatcher must preserve those semantics.

Pinned local references:

- `lib/fprime/Svc/CmdDispatcher/docs/sdd.md`
- `lib/fprime/Svc/FprimeRouter/docs/sdd.md`
- `lib/fprime/Svc/Subtopologies/CdhCore/CdhCore.fpp`
- `lib/fprime/Svc/Subtopologies/ComCcsds/docs/sdd.md`
- `lib/fprime/docs/user-manual/overview/03-port-comp-top.md`

## Goals / Non-Goals

**Goals:**

- Enforce OBC-side authority policy for routed `Fw.Com` command packets before `CmdDispatcher`.
- Preserve F Prime command source/status port semantics.
- Fail closed for restricted-link malformed, unknown, and denied commands.
- Provide synthetic command status exactly once for denied commands.
- Add component tests, dictionary/catalog tests, and hosted profile evidence for S-band primary and UHF backup configured roles.

**Non-Goals:**

- No full link authority, full uplink authority, file packet authority, unknown packet authority, gateway enforcement, crypto/session/sequence/replay enforcement, dynamic failover, persistent config store, or CCSDS redesign.

## Decisions

### Decision: Gate Is A Passive Component

`CommandIngressAuthority` is a passive component. It performs decode/classify/decision/forward synchronously in the caller context. If guarded input or internal lock state is used, the implementation keeps the critical section short and does not call downstream output ports while holding the lock.

### Decision: Topology Inserts The Gate Symmetrically

Default CCSDS topology:

```text
ComCcsds.fprimeRouter.commandOut
  -> commandIngressAuthority.seqCmdBuffIn[0]
  -> CdhCore.cmdDisp.seqCmdBuff[0]

CdhCore.cmdDisp.seqCmdStatus[0]
  -> commandIngressAuthority.seqCmdStatusIn[0]
  -> ComCcsds.fprimeRouter.cmdResponseIn
```

Legacy ComFprime topology mirrors the same pattern with `ComFprime.fprimeRouter`.

The gate preserves the input port index and `Fw.Com.context` exactly. It does not infer link identity from `Fw.Com.context`; configured ingress role comes from explicit hosted/topology profile configuration.

### Decision: Denied Commands Produce Synthetic Status

Denied commands are not forwarded to `CmdDispatcher`. The gate emits authority evidence and returns exactly one synthetic `Fw::CmdResponse` through the same upstream status path:

| Case | Response |
|---|---|
| policy deny | `VALIDATION_ERROR` |
| restricted malformed command packet | `FORMAT_ERROR` |
| missing/invalid authority config | `EXECUTION_ERROR` |
| restricted unknown opcode | `INVALID_OPCODE` |

### Decision: Restricted Ingress Fails Closed

Primary/dev configured ingress may preserve existing dispatcher behavior for malformed or unknown commands where needed. UHF backup and unknown/invalid configured roles fail closed at the gate, emit rejection evidence, and do not forward to subsystem handlers.

### Decision: Event Throttles But Counters Do Not

The authority gate emits a throttled rejection event to avoid log flood. Counters and telemetry continue to increment for every rejection. V1 keeps telemetry bounded: total rejects, role/reason/class counters where practical, and last rejected opcode, rather than per-opcode telemetry channels.

### Decision: File And Unknown Packet Paths Are Deferred

`FprimeRouter.fileOut` and `FprimeRouter.unknownDataOut` are not gated in this change. Documentation and evidence must state that UHF backup v1 does not restrict direct file packet uplink or unknown packet routing.

### Decision: Current OBC Has No CmdSequencer Bypass

The active OBC dictionaries do not include `Svc::CmdSequencer` commands. The upstream F Prime reference topology contains a sequencer path, but this project topology does not. This change therefore does not add a sequencer-control probe. It does require dictionary coverage to classify and deny sequencer load/run/control commands if a later topology adds them.

## Traceability

| Requirement | Behavior | Evidence |
|---|---|---|
| Gate precedes `CmdDispatcher` | Topology inserts `CommandIngressAuthority` between router and dispatcher | Component test and hosted probes |
| Source/status semantics preserved | Port index and context are forwarded unchanged | Component test |
| Denied command gets status | Gate returns exactly one synthetic `Fw.CmdResponse` | Component test |
| Restricted ingress fails closed | UHF backup configured-profile malformed/unknown/denied commands do not forward | Component test and hosted profile probe |
| File/unknown paths deferred | Docs and evidence explicitly exclude those paths | OpenSpec/evidence review |

## Risks / Trade-offs

- **[Risk] Configured ingress role is not full physical provenance.** Mitigation: scope states configured ingress only; multi-link simultaneous provenance and dynamic failover are deferred.
- **[Risk] Hosted UHF backup profile evidence is mistaken for UHF serial provenance.** Mitigation: registry and evidence keep this profile-gate proof separate from the existing hosted UHF serial node-6 path.
- **[Risk] Synthetic status can diverge from dispatcher behavior.** Mitigation: responses are specified per case and tested exactly once.
- **[Risk] Authority logs can flood.** Mitigation: throttle events and keep counters unthrottled.
- **[Risk] File/unknown packet bypass is misunderstood.** Mitigation: scope and evidence explicitly state command ingress only.
