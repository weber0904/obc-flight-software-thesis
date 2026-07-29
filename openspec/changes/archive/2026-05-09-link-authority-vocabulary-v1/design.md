## Context

F Prime v4.1.0 in this repository routes decoded command packets through `Svc::FprimeRouter.commandOut(Fw.Com)` into `Svc::CommandDispatcher.seqCmdBuff`. `CmdDispatcher` decodes and dispatches commands; it does not own mission authority policy. `FprimeRouter` also routes file and unknown packets on separate ports, so command classification cannot claim full uplink or file authority.

The project design needs a policy layer that can later be reused by command ingress gating, command sessions, sequence control, scheduler work, file transfer arbitration, and FDIR without redefining command classes each time.

Pinned local references:

- `lib/fprime/Svc/CmdDispatcher/docs/sdd.md`
- `lib/fprime/Svc/FprimeRouter/docs/sdd.md`
- `lib/fprime/Svc/Subtopologies/ComCcsds/docs/sdd.md`
- `lib/fprime/docs/user-manual/overview/03-port-comp-top.md`
- `lib/fprime/docs/reference/fpp-json-dict.md`

## Goals / Non-Goals

**Goals:**

- Establish stable authority vocabulary and command classification for current OBC commands.
- Key policy by fully qualified command name from FPP JSON topology dictionaries.
- Generate or verify a runtime opcode catalog from dictionary data.
- Fail tests when any active command is unclassified.
- Keep UHF backup v1 conservative and read/status only.

**Non-Goals:**

- No runtime command enforcement in this change alone.
- No source attribution, gateway filtering, file/unknown uplink gating, command sessions, sequence windows, auth, replay protection, failover automation, scheduler, or FDIR.

## Decisions

### Decision: Fully Qualified Dictionary Names Own Policy

The policy source uses fully qualified `commands[].name` values from:

- `build-fprime-automatic-native/OBC/TopCcsds/AppTopologyDictionary.json`
- `build-fprime-automatic-native/OBC/Top/AppTopologyDictionary.json`

Runtime lookup uses generated opcode mappings derived from those dictionaries. Short suffixes such as `GET_STATUS` are not policy keys because they collide across components. Numeric opcodes are generated artifacts, not manually maintained policy truth.

### Decision: UHF Backup Is Read/Status Only In V1

Without cryptographic auth, command sessions, and source attribution beyond configured ingress role, UHF backup v1 allows only conservative read/status commands. `emergency_safety` remains a vocabulary class for later work, but this change does not permit broad safety/emergency execution over UHF backup.

### Decision: Future Sequencer Commands Fail Closed

The active OBC dictionaries currently do not include `CmdSequencer` commands. If future topology changes add sequence load/run/control commands, dictionary coverage must classify them, and UHF backup policy must reject them unless a later sequence-authority model explicitly allows them.

### Decision: Vocabulary Is Not Full Link Authority

This change defines `LinkIdentity` and `LinkRole`, but it does not prove physical link provenance or dynamic primary/backup failover. Runtime configured role and command ingress enforcement are handled by the paired `command-ingress-authority-v1` change.

## Traceability

| Requirement | Behavior | Evidence |
|---|---|---|
| Command authority vocabulary exists | Shared types define link identity, link role, command class, resource label, decision, and reason | Policy/unit tests |
| Policy uses FQ command names | Policy source keys match `commands[].name` from both dictionaries | Dictionary coverage test |
| Every command is classified | Unclassified dictionary commands fail tests | Catalog coverage test |
| UHF backup allowlist is conservative | Only the seven read/status commands are allowed for `UHF + BACKUP` | Policy matrix tests |
| Future sequencer bypass fails closed | Current dictionaries assert no active sequencer commands; future commands require classification | Dictionary coverage test |

## Risks / Trade-offs

- **[Risk] Generated catalog may drift from dictionaries.** Mitigation: tests compare generated output with both topology dictionaries.
- **[Risk] Policy source can grow large.** Mitigation: group commands by fully qualified name and class/resource labels; avoid duplicating runtime opcode numbers by hand.
- **[Risk] Vocabulary can be mistaken for enforcement.** Mitigation: OpenSpec and evidence state that runtime enforcement belongs to `command-ingress-authority-v1`.
