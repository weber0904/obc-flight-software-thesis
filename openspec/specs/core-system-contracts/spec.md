# core-system-contracts Specification

## Purpose
Define the shared types, identifiers, node allocations, and core public contracts that are reused across all OBC capabilities.
## Requirements
### Requirement: Shared Identifiers And Types
The core system contracts capability SHALL define the shared base ID ranges, SHALL reserve CSP node IDs `1`, `2`, and `3` for OBC, EPS, and ADCS respectively, and SHALL publish the shared `SatMode`, `AdcsMode`, `CommBand`, and `BootSlot` types for reuse by other capabilities. The primary `SatMode` contract SHALL use the v2 values `SAFE = 0`, `IDLE = 1`, `HELL = 2`, `PAYLOAD = 3`, and `TTC = 4`.

#### Scenario: Subsystem capability reuses shared types
- **WHEN** a subsystem capability needs to reference a mode, band, or boot slot
- **THEN** it SHALL reference the shared type owned by `core-system-contracts` instead of redefining that type

#### Scenario: Primary mode enum uses v2 values
- **WHEN** the project generates the shared `SatMode` dictionary and component interfaces
- **THEN** the generated enum SHALL expose `SAFE`, `IDLE`, `HELL`, `PAYLOAD`, and `TTC` with the governed v2 numeric values
- **AND** it SHALL NOT expose `NOMINAL`, `LOW_POWER`, `DEBUG`, or `UPDATE` as primary mission modes

### Requirement: Core Control Interface
The system SHALL expose the `MODE_*`, `HEALTH_*`, and `CSP_*` core command families and SHALL provide the `SYS_*` and `CSP_*` telemetry required to observe system state, uptime, resource pressure, and real libcsp connectivity.

#### Scenario: Core CSP operation is diagnosable
- **WHEN** an operator issues `CSP_INIT` followed by `CSP_PING`
- **THEN** the deployment SHALL initialize the local libcsp node, attempt the ping over the governed internal CSP substrate, and surface enough telemetry to diagnose the connectivity result

### Requirement: CspBridge Owns Runtime Foundation
`CspBridge` SHALL own the repository's internal libcsp runtime foundation for the OBC process, including local node initialization, interface binding, diagnostic ping, bounded raw debug send, and runtime counter publication.

#### Scenario: CspBridge initializes hosted node 1
- **WHEN** the hosted OBC runtime starts the governed internal CSP foundation path
- **THEN** `CspBridge` SHALL initialize libcsp node `1`, bind the hosted interface, and publish runtime counters from the real libcsp state

#### Scenario: CspBridge remains diagnostic rather than subsystem business transport
- **WHEN** later subsystem bridges use the internal CSP network for business traffic
- **THEN** `CspBridge` SHALL remain the owner of runtime bring-up and diagnostics while the subsystem-specific business requests stay owned by the corresponding subsystem bridges

### Requirement: CspBridge Selects The Active Runtime Carrier
`CspBridge` SHALL keep owning libcsp runtime bring-up and diagnostics, and that ownership SHALL include selecting and initializing the configured internal CSP carrier or interface backend for the OBC process.

#### Scenario: CspBridge initializes node 1 through the configured carrier
- **WHEN** the OBC process starts the governed internal CSP runtime
- **THEN** `CspBridge` SHALL initialize libcsp node `1` through the configured carrier or backend, publish runtime counters from the real libcsp state, and keep subsystem business traffic ownership with the subsystem bridges

#### Scenario: Linux OBC runtime binds through SocketCAN
- **WHEN** the OBC process runs with `CSP_TRANSPORT=socketcan`
- **THEN** `CspBridge` SHALL initialize node `1` through the governed SocketCAN backend, SHALL require an explicit `CSP_CAN_DEVICE`, and SHALL continue to keep subsystem business traffic ownership with the subsystem bridges

#### Scenario: SocketCAN init fails fast on invalid interface state
- **WHEN** a later change or operator points the OBC runtime at a missing, non-CAN, or down Linux network device
- **THEN** the runtime SHALL fail before later CSP traffic attempts and SHALL not silently degrade into an ambiguous ping or request timeout

### Requirement: Single Owner For Public Symbol Families
Each public command, telemetry, and event family SHALL have exactly one owning capability. `core-system-contracts` SHALL own only the shared types and core system symbol families, while subsystem-specific public symbols SHALL be owned by their respective subsystem capabilities.

#### Scenario: Subsystem contracts are not duplicated centrally
- **WHEN** EPS, ADCS, comms, or boot public symbols are defined
- **THEN** those symbols SHALL be owned by their subsystem capability and SHALL NOT be duplicated in the central core contract capability

### Requirement: Project-Local Shared Core Types
The project SHALL define the shared `SatMode`, `AdcsMode`, `CommBand`, `BootSlot`, and `HealthItem` types in a project-local FPP module, and it SHALL define the shared base ID range constants plus CSP node constants for OBC, EPS, and ADCS in the same capability-owned area. The project-local `SatMode` type SHALL be the only primary spacecraft mode enum used by command, telemetry, event, HK trend, and live beacon mode fields.

#### Scenario: Later components reuse a shared project-owned type
- **WHEN** a later project component needs a shared mode, band, slot, or health-item type
- **THEN** it SHALL import the project-local shared type instead of redefining the symbol

#### Scenario: Public mode surfaces stay aligned
- **WHEN** `ModeManager.MODE_SET`, `SYS_MODE`, `SYS_MODE_CHANGE`, runtime status text, HK trend records, or live beacon encode/decode logic expose spacecraft mode
- **THEN** they SHALL use the project-local `SatMode` v2 values and names

### Requirement: Concrete Core Component Interfaces
The project SHALL provide concrete F' component interfaces for `ModeManager`, `WatchdogSupervisor`, and `CspBridge`, and those components SHALL own the `MODE_*`, core resource-monitoring `HEALTH_*`, watchdog status/config, `SYS_*`, and `CSP_*` public symbol families assigned to this capability.

#### Scenario: Core component ownership is visible in code
- **WHEN** the project builds the core contract capability
- **THEN** the source tree SHALL contain project-local component definitions for `ModeManager`, `WatchdogSupervisor`, and `CspBridge`
- **AND** the separate `HealthMonitor` owner SHALL no longer remain as the active core owner for resource-monitoring public symbols

### Requirement: Core Resource Monitoring Survives Owner Migration
The core-system-contracts capability SHALL preserve the existing
resource-monitoring behavior while moving that behavior under
`WatchdogSupervisor`.

#### Scenario: Resource monitoring remains operator-visible
- **WHEN** an operator uses the active baseline after watchdog-v1
- **THEN** the deployment SHALL still expose resource-monitoring enable and
  threshold commands
- **AND** it SHALL still expose CPU and RSS review surfaces through the owned
  `SYS_*` telemetry and warning events
- **AND** those surfaces SHALL now be owned by `WatchdogSupervisor` rather than
  a separate `HealthMonitor`

#### Scenario: Hosted RSS telemetry reflects current resident memory
- **WHEN** the hosted runtime publishes `SYS_MEM_RSS_MB`
- **THEN** it SHALL sample the current resident memory of the running `OBC`
  process rather than a historical high-water RSS value
- **AND** the published value SHALL remain suitable for direct comparison
  against the configured RSS warning threshold

#### Scenario: Resource warnings are emitted on threshold crossing
- **WHEN** `SYS_CPU_USAGE` or `SYS_MEM_RSS_MB` rises from at-or-below the
  configured threshold to above the configured threshold while monitoring is
  enabled
- **THEN** `WatchdogSupervisor` SHALL emit the corresponding warning event once
- **AND** it SHALL NOT re-emit the same warning on later samples that remain
  above threshold without first dropping back to at-or-below threshold

### Requirement: Node-5 Resource Keep-Live Truth Uses WatchdogSupervisor SYS Surfaces

The core-system-contracts capability SHALL define `WatchdogSupervisor` `SYS_*`
as the formal node-`5` resource keep-live truth while keeping
`SystemResources` as a supplemental integrated diagnostics surface.

#### Scenario: Formal node-5 resource truth uses SYS CPU and RSS surfaces
- **WHEN** current docs, runbooks, or observability proofs describe node-`5`
  pass-time resource truth
- **THEN** they SHALL use `SYS_CPU_USAGE`, `SYS_MEM_RSS_MB`,
  `SYS_RESOURCE_DEGRADED`, and `SYS_LOW_MEMORY`
- **AND** they SHALL describe `SYS_MEM_RSS_MB` as current resident memory and
  `SYS_RESOURCE_DEGRADED` / `SYS_LOW_MEMORY` as threshold-crossing warnings
- **AND** they SHALL NOT describe `SystemResources.*` as the preferred or
  formal node-`5` pass-time resource truth.

#### Scenario: SystemResources remains available but supplemental
- **WHEN** the active baseline still instantiates `Svc::SystemResources`
- **THEN** current docs MAY keep it as hosted or targeted diagnostics/review
  telemetry
- **AND** they SHALL state that the surface is supplemental rather than
  baseline node-`5` operator truth.

### Requirement: Core Watchdog Status And Config Surface Is Bounded
The core-system-contracts capability SHALL expose a bounded watchdog public surface on the active baseline while keeping shared-recovery ownership separate from detector-local watchdog truth.

#### Scenario: Watchdog status is reviewable
- **WHEN** the operator issues `GET_WATCHDOG_STATUS`
- **THEN** the active runtime SHALL expose aggregate watchdog state together with per-source freshness, suppression, and detector-local escalation truth through reviewable telemetry and/or dedicated status events
- **AND** that status SHALL remain bounded to watchdog-local truth rather than replacing the separate shared-recovery status surface

#### Scenario: Watchdog config is mutable at runtime
- **WHEN** the operator issues `SET_WATCHDOG_CONFIG`
- **THEN** the active runtime SHALL validate the requested watchdog source and threshold values
- **AND** it SHALL update the in-memory watchdog config for that source only if the request is valid
- **AND** it SHALL emit reviewable config-update evidence

#### Scenario: V1 still does not claim unsupported generic restart executors
- **WHEN** `recovery-executors-v1` updates the core watchdog and recovery public surfaces
- **THEN** the active baseline SHALL NOT claim generic `FORCE_PROCESS_RESTART` or `FORCE_SUBSYSTEM_RESET` unless a later governed change adds truthful runtime executors for those operator-driven actions

### Requirement: Minimum Verified Core Behaviors
The first implementation slice SHALL verify mode changes, health-threshold handling, and CSP initialization/ping/send behaviors with automated tests before the change is archived.

#### Scenario: Core contract change is archived
- **WHEN** `core-system-contracts-v1` is ready to archive
- **THEN** the change SHALL include automated verification evidence covering the core command and telemetry paths for the three new components

### Requirement: Core Contracts Distinguish Ground, Subsystem, And Sensor Paths
The core system contracts capability SHALL distinguish the direct GDS development path, the future omitted-RF TT&C comm path, the internal subsystem CSP path, and the direct GPS sensor path as separate architecture domains that later capabilities may interact with but SHALL NOT silently collapse.

#### Scenario: Future subsystem work cannot reuse the wrong domain boundary
- **WHEN** a later change extends comm, GPS, or internal CSP behavior
- **THEN** the project SHALL be able to identify which architecture domain that change belongs to without reusing an adjacent path's contract by implication

#### Scenario: Reused framing does not collapse direct GDS and COMM TT&C
- **WHEN** the repository reuses stock F' framing across both the direct GDS baseline and the first COMM-backed omitted-RF path
- **THEN** the contracts SHALL still treat those as separate architecture domains instead of treating shared framing alone as proof that the paths are interchangeable

### Requirement: Future COMM CSP Identity Is Governed Before Implementation
Before the repository implements the COMM CSP-facing subsystem path, the core system contracts capability SHALL require that COMM node identity and shared-bus participation be explicitly governed instead of being introduced ad hoc inside implementation code or scripts, and the first governed implementation SHALL reserve node `4` plus application service ports `30` through `39` for COMM-owned traffic.

#### Scenario: COMM node identity is formalized before physical-bus implementation
- **WHEN** a later change begins implementing COMM as a CSP-facing subsystem
- **THEN** that change SHALL define the COMM node identity and participation rules through governed contracts rather than by implicit script defaults alone

#### Scenario: COMM service ownership is formalized together with node identity
- **WHEN** the first governed COMM CSP path is implemented
- **THEN** the repository SHALL reserve node `4` and application service ports `30` through `39` for COMM instead of introducing those values only inside scripts, probes, or simulator source defaults

### Requirement: Mode Shell Boundary
The first mode-model-v2 implementation SHALL make `HELL`, `PAYLOAD`, and `TTC` primary modes without adding unrelated deferred behaviors, and the ttc-pass-window-mode-v1 implementation SHALL upgrade `TTC` from a pure manual shell to a bounded pass-window-driven mode policy.

#### Scenario: Mode shells do not imply unrelated deferred subsystems
- **WHEN** an operator command or internal safety path changes the current mode to `HELL`, `PAYLOAD`, or `TTC`
- **THEN** the system SHALL update the public mode state
- **AND** it SHALL NOT claim generic scheduler behavior, payload execution, ADCS tracking, COMM architecture redesign, storage-policy behavior, or broader FDIR from that transition alone

#### Scenario: PAYLOAD shell remains free of payload mission side effects
- **WHEN** an operator enters `PAYLOAD`
- **THEN** the runtime SHALL update only the approved system mode state, event, and telemetry contract
- **AND** it SHALL NOT start camera, recorder, payload power sequencing, payload data products, or payload mission execution

#### Scenario: TTC mode is policy-driven but remains bounded
- **WHEN** TTC pass policy enters or retains `TTC`
- **THEN** the runtime SHALL update the approved system mode state, event, telemetry, and TTC policy status surfaces
- **AND** it SHALL NOT claim generic scheduling, TLE parsing, orbital propagation, ADCS ground tracking, link authority, auth session, or CCSDS routing changes from this change alone

### Requirement: Guarded Operator Mode Transitions
The core system contracts capability SHALL route every operator-requested primary mode transition through the v1 operator transition guard instead of treating `MODE_SET` or the hosted mode shell as arbitrary mode setters.

#### Scenario: Operator transition matrix is enforced
- **WHEN** an operator requests a transition through `MODE_SET` or hosted `mode <target>`
- **THEN** the runtime SHALL evaluate the request against this v1 matrix:

| From \ To | SAFE | IDLE | PAYLOAD | TTC | HELL |
|---|---:|---:|---:|---:|---:|
| SAFE | no-op | guarded allow | reject | reject | reject |
| IDLE | allow | no-op | guarded allow | allow | reject |
| PAYLOAD | allow | allow | no-op | reject | reject |
| TTC | allow | allow | reject | no-op | reject |
| HELL | guarded allow | reject | reject | reject | no-op |

#### Scenario: MODE_SET uses the guarded operator path
- **WHEN** an operator sends `ModeManager.MODE_SET` with a valid `SatMode` argument
- **THEN** `ModeManager` SHALL use the guarded operator request path before changing the runtime mode
- **AND** `ModeManager.MODE_SET` SHALL NOT call the internal safety/test apply path directly

#### Scenario: Hosted shell uses the guarded operator path
- **WHEN** an operator sends `mode safe`, `mode idle`, `mode payload`, `mode ttc`, or `mode hell` through the hosted runtime shell
- **THEN** the hosted runtime SHALL use the same guarded operator request path as `MODE_SET`
- **AND** the hosted runtime shell SHALL NOT call the internal safety/test apply path directly

#### Scenario: Invalid F Prime enum payload is rejected before the guard
- **WHEN** a `MODE_SET` command payload contains an out-of-range `SatMode` serialized value
- **THEN** generated F Prime deserialization SHALL reject the payload with `FORMAT_ERROR`
- **AND** `ModeManager` SHALL NOT invoke the transition guard or emit transition events for that malformed command

### Requirement: Operator Mode Transition Outcomes
The core system contracts capability SHALL define deterministic response, event, and telemetry semantics for accepted, no-op, rejected, and parser-rejected operator mode requests.

#### Scenario: Accepted mode-changing transition publishes the existing mode-change surface
- **WHEN** an operator transition is accepted and the target mode differs from the current mode
- **THEN** the runtime SHALL update the current mode to the target mode
- **AND** it SHALL emit exactly one existing `SYS_MODE_CHANGE(mode)` event using the target mode
- **AND** it SHALL eventually publish `SYS_MODE` telemetry reflecting the target mode
- **AND** it SHALL return `Fw::CmdResponse::OK` on the F Prime command path

#### Scenario: Same-mode request is an OK no-op
- **WHEN** an operator requests the current mode
- **THEN** the runtime SHALL leave the current mode unchanged
- **AND** it SHALL emit zero `SYS_MODE_CHANGE` events
- **AND** it SHALL emit zero `SYS_MODE_TRANSITION_REJECTED` events
- **AND** it SHALL return `Fw::CmdResponse::OK` on the F Prime command path

#### Scenario: Rejected transition preserves current mode
- **WHEN** an operator transition is rejected by the v1 transition guard
- **THEN** the runtime SHALL leave the current mode unchanged
- **AND** it SHALL emit zero `SYS_MODE_CHANGE` events
- **AND** it SHALL emit exactly one `SYS_MODE_TRANSITION_REJECTED(fromMode, toMode, reasonCode)` event
- **AND** it SHALL NOT publish `SYS_MODE` telemetry with the rejected target mode

#### Scenario: Guard-denied transition maps to validation error
- **WHEN** an operator transition is rejected because the requested transition is not allowed or because a configured SoC guard denies the request
- **THEN** `MODE_SET` SHALL return `Fw::CmdResponse::VALIDATION_ERROR`

#### Scenario: Missing guard wiring maps to execution error
- **WHEN** an operator transition request reaches `ModeManager` without a configured transition guard
- **THEN** `MODE_SET` SHALL return `Fw::CmdResponse::EXECUTION_ERROR`
- **AND** it SHALL emit `SYS_MODE_TRANSITION_REJECTED` with reason code `GUARD_UNCONFIGURED`

### Requirement: Mode Transition Rejection Reasons
The core system contracts capability SHALL publish stable `U32` reason codes for `SYS_MODE_TRANSITION_REJECTED`.

#### Scenario: Reason codes are stable
- **WHEN** a mode transition is rejected by the v1 operator transition guard
- **THEN** `SYS_MODE_TRANSITION_REJECTED.reasonCode` SHALL use these stable values:

| Code | Name |
|---:|---|
| 1 | `DISALLOWED_OPERATOR_TRANSITION` |
| 2 | `INTERNAL_ONLY_TARGET` |
| 3 | `SOC_GUARD_UNAVAILABLE` |
| 4 | `SOC_GUARD_NOT_MET` |
| 5 | `GUARD_UNCONFIGURED` |

#### Scenario: Internal-only HELL target is distinguishable
- **WHEN** an operator requests `SAFE -> HELL`, `IDLE -> HELL`, `PAYLOAD -> HELL`, or `TTC -> HELL`
- **THEN** the request SHALL be rejected with reason code `INTERNAL_ONLY_TARGET`

#### Scenario: Disallowed topology transitions are distinguishable
- **WHEN** an operator requests `SAFE -> PAYLOAD`, `SAFE -> TTC`, `PAYLOAD -> TTC`, `TTC -> PAYLOAD`, `HELL -> IDLE`, `HELL -> PAYLOAD`, or `HELL -> TTC`
- **THEN** the request SHALL be rejected with reason code `DISALLOWED_OPERATOR_TRANSITION`

#### Scenario: Shared SoC guard codes cover recovery and admission
- **WHEN** an operator `HELL -> SAFE`, `SAFE -> IDLE`, or `IDLE -> PAYLOAD` request is rejected because cached EPS status is unavailable
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_UNAVAILABLE`

#### Scenario: Shared SoC guard threshold failures are distinguishable
- **WHEN** an operator `HELL -> SAFE`, `SAFE -> IDLE`, or `IDLE -> PAYLOAD` request is rejected because cached EPS SoC does not meet the required strict threshold
- **THEN** the request SHALL be rejected with reason code `SOC_GUARD_NOT_MET`

### Requirement: Hosted Mode Shell Parser Boundary
The core system contracts capability SHALL keep hosted mode shell parsing separate from transition-guard rejection.

#### Scenario: Canonical hosted mode spellings parse
- **WHEN** an operator enters `mode safe`, `mode idle`, `mode payload`, `mode ttc`, or `mode hell`
- **THEN** the hosted parser SHALL parse the target as the corresponding `SatMode`
- **AND** the transition guard SHALL determine whether the parsed request is accepted or rejected

#### Scenario: Retired mode spellings stay rejected by the parser
- **WHEN** an operator enters `mode nominal`, `mode low-power`, `mode debug`, or `mode update`
- **THEN** the hosted parser SHALL reject the input as an unknown mode
- **AND** the runtime SHALL emit zero transition events for that parser error

#### Scenario: Unknown or mixed-case mode spellings stay parser errors
- **WHEN** an operator enters an unknown spelling or a mixed-case spelling such as `mode PAYLOAD`
- **THEN** the hosted parser SHALL reject the input as an unknown mode
- **AND** the runtime SHALL emit zero transition events for that parser error

### Requirement: Command Authority Vocabulary Is Defined
The core system contract SHALL define a reusable command authority vocabulary for routed F Prime command packets, including link identity, configured link role, command class, resource label, authority decision, and rejection reason.

#### Scenario: Vocabulary separates identity from role
- **WHEN** command authority policy is described
- **THEN** link identity such as `SBAND`, `UHF`, `DEV_DIRECT`, `INTERNAL`, or `UNKNOWN` SHALL remain distinct from configured role such as `PRIMARY`, `BACKUP`, `PRIMARY_AFTER_FAILOVER`, `DEV_FULL`, or `INTERNAL`
- **AND** the vocabulary SHALL NOT hard-code `UHF` as permanently low authority or `SBAND` as permanently primary.

#### Scenario: Vocabulary does not claim runtime enforcement
- **WHEN** `link-authority-vocabulary-v1` is cited
- **THEN** it SHALL be cited only as vocabulary and policy-source definition
- **AND** it SHALL NOT be cited as runtime command ingress enforcement, full link authority, file authority, uplink authority, crypto authentication, session sequencing, replay protection, or failover enforcement.

### Requirement: Command Authority Policy Uses Fully Qualified Command Names
The command authority policy source SHALL key command classification by fully
qualified FPP JSON dictionary command names and SHALL derive runtime opcode
lookup from dictionary data for maintained active topology dictionaries.

#### Scenario: Short command names are not policy keys
- **WHEN** policy classifies commands from the active topology dictionaries
- **THEN** it SHALL use `commands[].name` values such as
  `OBCApp.modeManager.MODE_GET`
- **AND** it SHALL NOT rely on suffix-only names such as `GET_STATUS`
- **AND** it SHALL NOT hand-maintain global opcode numbers as the policy source
  of truth.

#### Scenario: Every active command is classified
- **WHEN** repository tests inspect the maintained active CCSDS topology
  dictionary
- **THEN** every active command entry SHALL have a command authority
  classification
- **AND** missing classifications SHALL fail the focused policy/catalog tests
- **AND** retired `OBCAppComFprimeLegacy.*` command names SHALL NOT be required
  or retained as current policy entries.

### Requirement: UHF Backup Command Policy Is Conservative In V1
The v1 command authority policy SHALL allow UHF backup role to execute only the
explicit read/status command allowlist and SHALL reject other active command
classes until later governed authority changes expand that role.

#### Scenario: UHF backup allows only read/status commands
- **WHEN** a command is evaluated under `UHF + BACKUP`
- **THEN** only current-mode, EPS status, ADCS attitude, GPS state, radio status, storage status, and boot status commands SHALL be allowed
- **AND** mode changes, ADCS control, EPS writes or resets, radio configuration, COMM active/pass control, boot update/reset operations, current file/downlink commands, retired housekeeping file/downlink commands if present in legacy dictionaries, sequencer commands, CSP commands, configuration updates, payload control, and unclassified commands SHALL be denied.

### Requirement: Command Ingress Authority Gates Routed Commands
The core system contract SHALL provide OBC-side command ingress authority
enforcement for routed `Fw.Com` command packets before they reach
`Svc::CommandDispatcher` on the maintained active topology.

#### Scenario: Gate is inserted before command dispatch
- **WHEN** the default CCSDS topology routes F Prime command packets
- **THEN** `ComCcsds.fprimeRouter.commandOut` SHALL connect to
  `CommandIngressAuthority`
- **AND** allowed commands SHALL be forwarded from `CommandIngressAuthority` to
  `CdhCore.cmdDisp.seqCmdBuff`.

#### Scenario: Retired legacy topology is not a current gate obligation
- **WHEN** archived legacy `OBC/Top` or `OBC_ComFprimeLegacy` evidence is
  reviewed
- **THEN** it SHALL NOT create a current command-ingress authority wiring or
  policy obligation for the retired topology.

### Requirement: Command Status Semantics Are Preserved
The command ingress authority gate SHALL preserve F Prime command source/status semantics for both forwarded and denied commands.

#### Scenario: Forwarded commands preserve context
- **WHEN** an allowed command is forwarded to `Svc::CommandDispatcher`
- **THEN** the original `Fw.Com.context` SHALL be forwarded unchanged
- **AND** the status returned from `CmdDispatcher.seqCmdStatus` SHALL be returned to the same upstream status sink.

#### Scenario: Denied commands return synthetic status
- **WHEN** the gate denies a command before dispatch
- **THEN** it SHALL NOT forward the command to `Svc::CommandDispatcher`
- **AND** it SHALL return exactly one synthetic `Fw.CmdResponse` through the same upstream status path.

#### Scenario: Synthetic status response mapping is deterministic
- **WHEN** a command is denied by policy
- **THEN** the synthetic response SHALL be `VALIDATION_ERROR`
- **WHEN** a restricted ingress receives a malformed command packet
- **THEN** the synthetic response SHALL be `FORMAT_ERROR`
- **WHEN** authority configuration is missing or invalid
- **THEN** the synthetic response SHALL be `EXECUTION_ERROR`
- **WHEN** restricted ingress receives an unknown opcode
- **THEN** the synthetic response SHALL be `INVALID_OPCODE`.

### Requirement: Restricted Command Ingress Fails Closed
The command ingress authority gate SHALL fail closed for restricted or unknown configured ingress roles.

#### Scenario: UHF backup denied commands do not reach subsystem handlers
- **WHEN** a `UHF + BACKUP` configured ingress receives a command outside the v1 allowlist
- **THEN** the command SHALL be denied before `Svc::CommandDispatcher`
- **AND** the target subsystem command handler SHALL NOT run.

#### Scenario: Missing config does not default to S-band primary
- **WHEN** authority configuration is missing, invalid, or unknown
- **THEN** the gate SHALL deny command ingress by default
- **AND** it SHALL NOT silently assume `SBAND + PRIMARY`.

### Requirement: Authority Evidence Is Bounded And Observable

The command-security baseline SHALL continue to keep command, file, and unknown
uplink authority claims explicit instead of silently inferring repo-wide link
closure from command-only enforcement.

#### Scenario: Comm-managed authority scope now extends beyond command packets in a bounded way
- **WHEN** the current hosted secure-auth baseline is documented or cited
- **THEN** it SHALL be valid to claim command authority over routed
  `Fw.Com` command packets, handshake-only authority over comm-managed unknown
  uplink on APID `0x00FE`, and staged file-uplink authority over the active
  `.sequence-staging/<leaf>` file path
- **AND** it SHALL still NOT claim generic arbitrary file-uplink authority,
  repo-wide unknown-packet authority, full link authority, encryption, target
  proof, or hardware-backed security.

### Requirement: Comm-Managed Auth Material Uses A Shared Tracked Keystore Contract

The active hosted secure-auth baseline SHALL source comm-managed root key
material from one tracked keystore asset rather than from runtime-injected key
bytes.

#### Scenario: Secure auth uses the tracked keystore contract
- **WHEN** the hosted baseline starts comm-managed S-band or UHF auth
- **THEN** `SecureLinkAuthorizer` SHALL load the secure-auth root key and module
  serial from the tracked keystore contract
- **AND** that tracked contract SHALL describe `module_serial` plus per-service
  root-key material without requiring legacy `source_id` or `key_slot` tuple
  fields for the current secure baseline.

#### Scenario: Hosted runtime does not accept command-auth key injection
- **WHEN** an operator or script starts the hosted OBC runtime on the active
  baseline
- **THEN** the runtime SHALL NOT require or accept `--command-auth`,
  `--command-auth-source-id`, `--command-auth-key-slot`,
  `--command-auth-key-hex`, or `--command-auth-keystore` for comm-managed auth
  defaults
- **AND** repo-owned helper tooling SHALL read the tracked keystore contract
  instead.

### Requirement: Command Authority Uses Configured Ingress Source Indexes
The command ingress authority gate SHALL support topology-configured authority source mapping by input port index.

#### Scenario: Each ingress port has independent configured source
- **WHEN** a command enters `CommandIngressAuthority.seqCmdBuffIn[N]`
- **THEN** the gate SHALL evaluate that command using only the authority config assigned to ingress port `N`
- **AND** it SHALL NOT infer source identity from `Fw.Com.context`, gateway metadata, opcode, or packet body.

#### Scenario: Legacy component configuration targets port zero only
- **WHEN** `CommandIngressAuthority.configure(config)` is called
- **THEN** it SHALL clear all ingress source configs
- **AND** it SHALL configure only ingress port `0` with `config`.

#### Scenario: Unconfigured ingress ports fail closed
- **WHEN** a command enters an unconfigured or invalid ingress port
- **THEN** the gate SHALL NOT forward the command to `Svc::CommandDispatcher`
- **AND** it SHALL return exactly one synthetic `Fw.CmdResponse` through the same port-indexed status path
- **AND** the synthetic response SHALL be `EXECUTION_ERROR`.

### Requirement: Source Index Evidence Is Observable Without Claiming Trusted Provenance
The command ingress authority gate SHALL expose bounded rejection evidence that identifies the configured ingress source index and configured link identity.

#### Scenario: Rejection event includes configured source index
- **WHEN** the gate rejects a command
- **THEN** `COMMAND_AUTHORITY_REJECTED` SHALL include the ingress port index and configured link identity
- **AND** it SHALL include the configured link role, command class, rejection reason, and command response.

#### Scenario: Context remains command status correlation only
- **WHEN** a command is forwarded or rejected
- **THEN** `Fw.Com.context` SHALL be preserved unchanged for status correlation
- **AND** it SHALL NOT be treated as mission source identity, session identity, sequence number, or authority evidence.

#### Scenario: Hosted proof remains single ingress
- **WHEN** `command-ingress-source-index-v1` evidence cites the hosted default CCSDS path
- **THEN** the evidence SHALL describe that hosted topology as proving only configured ingress port `0`
- **AND** it SHALL NOT claim trusted source, per-packet provenance, physical UHF provenance, or simultaneous dual-link runtime proof.

### Requirement: Command Session Sequence Foundation Is Helper-Only
The core system contract SHALL provide a helper-only strict-monotonic command sequence primitive without wiring it into the active runtime command path.

#### Scenario: Session key separates configured source and authority epoch
- **WHEN** command sequence state is evaluated by the helper
- **THEN** the session key SHALL include ingress port, link identity, link role, and caller-supplied session ID
- **AND** link role changes SHALL be treated as a separate authority epoch.

#### Scenario: Strict monotonic sequence policy is defined
- **WHEN** a sequence number is evaluated for a session key
- **THEN** the first sequence for that key SHALL be accepted
- **AND** later sequences SHALL be accepted only when the new sequence number is greater than the last accepted value for the same key
- **AND** duplicate, lower, and wraparound sequence numbers SHALL be rejected unless an explicit reset has occurred.

#### Scenario: Runtime enforcement is deferred
- **WHEN** `command-session-sequence-foundation-v1` is cited
- **THEN** it SHALL NOT be cited as active session enforcement, replay protection, authentication, command envelope, or wire-format behavior
- **AND** it SHALL state that session IDs and sequence numbers are not generated, parsed, transmitted, or persisted by this change.

### Requirement: Command Envelope Metadata Is Carried In The Routed Command Path
The command ingress authority gate SHALL support a project-owned command envelope v1 carried as an outer F Prime command packet before `Svc::CommandDispatcher`.

#### Scenario: Envelope uses a project pseudo-opcode
- **WHEN** a routed `Fw.Com` command packet reaches `CommandIngressAuthority`
- **AND** the outer command opcode is `0x0BC10001`
- **THEN** the packet SHALL be treated as a mission command envelope v1 candidate
- **AND** it SHALL NOT be forwarded to `Svc::CommandDispatcher` as that pseudo-opcode.

#### Scenario: Envelope header is parsed before authority evaluation
- **WHEN** an envelope candidate contains magic `0x0BC0DE01`, version `1`, flags `0`, header length `20`, and reserved field `0`
- **THEN** the gate SHALL parse `sessionId`, `sequenceNumber`, `innerLength`, and the complete inner serialized `Fw.CmdPacket`
- **AND** authority evaluation SHALL use the inner command opcode.

#### Scenario: Legacy commands remain supported
- **WHEN** a routed command packet does not use the envelope pseudo-opcode
- **THEN** `CommandIngressAuthority` SHALL process it through the existing legacy authority path
- **AND** the existing `fprime-cli -> GDS -> CCSDS -> OBC` command path SHALL remain supported.

### Requirement: Envelope Metadata Is Observable Before Sequence Enforcement
The command envelope v1 SHALL expose mission metadata before any later authority or sequence outcome, while active sequence enforcement is governed by `command-session-sequence-v1`.

#### Scenario: Valid envelope metadata is observed
- **WHEN** a valid envelope is accepted for authority evaluation
- **THEN** the gate SHALL emit `COMMAND_ENVELOPE_OBSERVED` with ingress port, configured link identity, configured link role, session ID, sequence number, and inner opcode
- **AND** it SHALL update bounded envelope telemetry with the latest session ID, sequence number, and inner opcode.

#### Scenario: Metadata-only behavior is limited to command-envelope-metadata-v1
- **WHEN** `command-envelope-metadata-v1` evidence is cited by itself
- **THEN** it SHALL describe sequence numbers as observed metadata only
- **AND** it SHALL NOT be cited as active sequence enforcement.

#### Scenario: Valid envelopes enter active sequence enforcement after command-session-sequence-v1
- **WHEN** `command-session-sequence-v1` is part of the baseline
- **THEN** valid envelope sequence values SHALL be evaluated by the active sequence enforcement contract after authority allows the inner command.

#### Scenario: Context remains status correlation only
- **WHEN** an envelope command is forwarded or rejected
- **THEN** `Fw.Com.context` SHALL be preserved unchanged for command-status correlation
- **AND** it SHALL NOT be treated as session identity, sequence number, source identity, or authority evidence.

### Requirement: Malformed Envelopes Fail Closed
Recognized command envelope candidates that cannot be parsed as valid v1 envelopes SHALL fail closed before `Svc::CommandDispatcher`.

#### Scenario: Malformed envelope is rejected before dispatch
- **WHEN** an envelope candidate has bad magic, unsupported version, nonzero flags, wrong header length, nonzero reserved field, truncated header, truncated inner command, or oversized inner length
- **THEN** the gate SHALL NOT forward the candidate to `Svc::CommandDispatcher`
- **AND** it SHALL return exactly one synthetic `Fw.CmdResponse::FORMAT_ERROR` through the same port-indexed status path
- **AND** it SHALL emit `COMMAND_ENVELOPE_REJECTED`.

#### Scenario: Malformed envelope response opcode is deterministic
- **WHEN** a malformed envelope contains enough bytes to safely decode the inner command opcode
- **THEN** the synthetic status SHALL use the inner opcode
- **OTHERWISE** the synthetic status SHALL use `0xFFFFFFFF`.

### Requirement: Envelope Authority Uses Existing Ingress Policy
The command envelope v1 SHALL not introduce a new authority source or bypass existing command ingress authority policy.

#### Scenario: Envelope source is not packet metadata
- **WHEN** an envelope is evaluated
- **THEN** the configured authority source SHALL be selected only from the `CommandIngressAuthority` input port index
- **AND** the envelope SHALL NOT carry or override source identity.

#### Scenario: Restricted envelope command is denied by existing policy
- **WHEN** `uhf-backup` ingress receives an envelope whose inner command is outside the v1 allowlist
- **THEN** the gate SHALL deny the command before `Svc::CommandDispatcher`
- **AND** it SHALL produce the same authority rejection semantics as a legacy command with that inner opcode.

### Requirement: Enveloped Commands Enforce Session Sequence Before Dispatch

The command ingress authority gate SHALL enforce strict-monotonic sequence state for valid command envelope v1 packets before forwarding an authority-allowed inner command to `Svc::CmdDispatcher`.

#### Scenario: Valid envelope uses authority-then-sequence ordering
- **WHEN** `CommandIngressAuthority` receives a valid command envelope v1 packet
- **THEN** it SHALL parse and observe the envelope metadata
- **AND** it SHALL evaluate existing command authority policy against the inner command opcode before evaluating sequence state
- **AND** it SHALL evaluate sequence state only when the inner command is allowed by authority policy.

#### Scenario: Accepted sequence forwards inner command
- **WHEN** an authority-allowed valid envelope carries the first or an increasing sequence number for its session key
- **THEN** `CommandIngressAuthority` SHALL accept the sequence
- **AND** it SHALL forward exactly one inner command buffer to `Svc::CmdDispatcher`
- **AND** it SHALL preserve the original `Fw.Com.context` value for command status correlation.

#### Scenario: Rejected sequence does not dispatch
- **WHEN** an authority-allowed valid envelope carries a duplicate, lower, wraparound, or table-full sequence result
- **THEN** `CommandIngressAuthority` SHALL reject the command before `Svc::CmdDispatcher`
- **AND** it SHALL forward zero inner command buffers
- **AND** it SHALL emit exactly one synthetic command response.

### Requirement: Command Sequence Enforcement Uses Configured Source Key

Active command sequence enforcement SHALL key runtime sequence state by configured ingress source and envelope session ID.

#### Scenario: Session key is derived from configured ingress source
- **WHEN** `CommandIngressAuthority` evaluates sequence state for a valid envelope
- **THEN** the session key SHALL include ingress port, configured link identity, configured link role, and envelope session ID
- **AND** it SHALL NOT use `Fw.Com.context`, gateway metadata, packet body source claims, or inner command arguments as source identity.

#### Scenario: Source/session/role keys are independent
- **WHEN** the same sequence number appears under a different ingress port, configured link identity, configured link role, or session ID
- **THEN** sequence enforcement SHALL treat it as a separate key.

### Requirement: Command Sequence Rejection Has Dedicated Evidence

Active command sequence rejection SHALL expose bounded event, telemetry, and command-response evidence without reusing command authority rejection reasons.

#### Scenario: Duplicate or lower sequence rejection is validation error
- **WHEN** a valid envelope is rejected because its sequence number is not greater than the last accepted sequence for the same key
- **THEN** the synthetic command response SHALL be `Fw::CmdResponse::VALIDATION_ERROR`
- **AND** the sequence rejection reason SHALL be `NOT_INCREASING`.

#### Scenario: Window full rejection is execution error
- **WHEN** a valid envelope is rejected because the sequence window cannot allocate a new session entry
- **THEN** the synthetic command response SHALL be `Fw::CmdResponse::EXECUTION_ERROR`
- **AND** the sequence rejection reason SHALL be `WINDOW_FULL`.

#### Scenario: Sequence rejection evidence is bounded
- **WHEN** sequence rejection occurs
- **THEN** `CommandIngressAuthority` SHALL emit `COMMAND_SEQUENCE_REJECTED` with ingress port, link identity, link role, session ID, sequence number, inner opcode, reason, and response
- **AND** it SHALL update bounded sequence rejection telemetry including total count, per-reason counts, and last rejected source/session/sequence/opcode/reason.

### Requirement: Sequence Enforcement Does Not Add Session Lifecycle Or Replay Protection Claims

`command-session-sequence-v1` SHALL remain a runtime duplicate/lower sequence rejection primitive and SHALL NOT claim full replay protection or session lifecycle management.

#### Scenario: Legacy and denied commands do not update sequence state
- **WHEN** a command is a legacy non-envelope command, a malformed envelope, an invalid-config command, an unknown/restricted command, or an authority-denied envelope
- **THEN** the command SHALL NOT update active sequence state.

#### Scenario: Runtime reset remains deferred
- **WHEN** `command-session-sequence-v1` is cited
- **THEN** it SHALL state that no production runtime session reset, session-open, resync, persistent session state, authentication, crypto, nonce, MAC, reliable transfer, file authority, unknown packet authority, trusted source, physical UHF provenance, or dual-link simultaneous proof is provided.

### Requirement: SoC-Guarded Operator Admissions
The core system contracts capability SHALL keep the existing guarded operator transition matrix while letting specific accepted transitions require cached-EPS SoC approval from the transition guard.

#### Scenario: Operator transition matrix still defines topology
- **WHEN** an operator requests a transition through `MODE_SET` or hosted `mode <target>`
- **THEN** the runtime SHALL evaluate the request against this v1 matrix:

| From \ To | SAFE | IDLE | PAYLOAD | TTC | HELL |
|---|---:|---:|---:|---:|---:|
| SAFE | no-op | guarded allow | reject | reject | reject |
| IDLE | allow | no-op | guarded allow | allow | reject |
| PAYLOAD | allow | allow | no-op | reject | reject |
| TTC | allow | allow | reject | no-op | reject |
| HELL | guarded allow | reject | reject | reject | no-op |

#### Scenario: SAFE to IDLE denial maps to validation error
- **WHEN** an operator requests `SAFE -> IDLE`
- **AND** the transition guard rejects the request because cached EPS SoC is unavailable or not strictly greater than `50%`
- **THEN** `MODE_SET` SHALL return `Fw::CmdResponse::VALIDATION_ERROR`

#### Scenario: IDLE to PAYLOAD denial maps to validation error
- **WHEN** an operator requests `IDLE -> PAYLOAD`
- **AND** the transition guard rejects the request because cached EPS SoC is unavailable or not strictly greater than `70%`
- **THEN** `MODE_SET` SHALL return `Fw::CmdResponse::VALIDATION_ERROR`

#### Scenario: IDLE to TTC remains free of new SoC gate
- **WHEN** an operator requests `IDLE -> TTC`
- **THEN** the transition guard SHALL evaluate only the existing topology/manual shell rule in this change
- **AND** it SHALL NOT deny that request due to a new SoC admission threshold introduced by this change

### Requirement: Enveloped Commands Require Explicit Session Open

The command ingress authority gate SHALL require active secure authorization
state synthesized into internal opened-session truth before a secure command v2
packet may use sequence enforcement or reach `Svc::CmdDispatcher`, and current
runtime legacy command-envelope v1 lifecycle traffic SHALL fail closed before
dispatch, session mutation, or sequence mutation.

#### Scenario: Secure command v2 requires active auth state
- **WHEN** `CommandIngressAuthority` receives a valid secure command v2 packet
- **AND** there is no active secure authorization state for that
  `(ingressPort, serviceId)`
- **THEN** the gate SHALL reject the command before `Svc::CmdDispatcher`
- **AND** it SHALL NOT infer an active secure session from packet contents
  alone.

#### Scenario: Legacy v1 envelope traffic is unsupported on the current runtime
- **WHEN** `CommandIngressAuthority` receives a legacy command envelope v1
  packet on the current baseline
- **THEN** the gate SHALL reject that packet before `Svc::CmdDispatcher`
- **AND** it SHALL NOT create, replace, clear, or refresh session state
- **AND** it SHALL NOT consume or advance secure or legacy sequence state.

### Requirement: Session Open Is The V1 Lifecycle And Recovery Surface

Current runtime session establishment SHALL be driven by secure-auth grant
events rather than by public `SESSION_OPEN`, and any retained legacy
`SESSION_OPEN` semantics SHALL remain historical compatibility evidence only
instead of an accepted current lifecycle or recovery surface.

#### Scenario: Auth success synthesizes secure runtime open
- **WHEN** `SecureLinkAuthorizer` reports `authGranted(ingressPort, serviceId,
  sKey)`
- **THEN** `CommandIngressAuthority` SHALL establish an active secure session
  for that `(ingressPort, serviceId)`
- **AND** it SHALL establish secure sequence baseline `0`
- **AND** it SHALL emit the same runtime opened-session side effects needed by
  the current observer path without requiring a wire-level `SESSION_OPEN`.

#### Scenario: Public legacy SESSION_OPEN is unsupported
- **WHEN** an operator, helper, or probe attempts to use public
  `SESSION_OPEN` as a current runtime lifecycle surface
- **THEN** the current baseline SHALL reject that path
- **AND** it SHALL NOT establish session, persistence, or reopen-floor state
  from that request.

### Requirement: Session Lifecycle Remains Distinct From Legacy Commands

Current runtime session lifecycle SHALL remain an internal secure-session
contract driven by secure-auth grant and revoke, and legacy command-path
traffic SHALL remain outside that lifecycle state.

#### Scenario: Auth revoke clears the active secure session
- **WHEN** `SecureLinkAuthorizer` reports secure-session revocation or timeout
- **THEN** `CommandIngressAuthority` SHALL clear the active secure session for
  the affected `(ingressPort, serviceId)`
- **AND** it SHALL preserve the current runtime revoke side effects and
  telemetry or event evidence.

#### Scenario: Legacy commands remain outside current session lifecycle
- **WHEN** `CommandIngressAuthority` receives legacy lifecycle or non-lifecycle
  command traffic on the current baseline
- **THEN** it SHALL reject that traffic before dispatch
- **AND** it SHALL NOT create, replace, clear, or consume current session
  lifecycle state.

### Requirement: Session Lifecycle Evidence Is Dedicated And Bounded

Session lifecycle SHALL expose dedicated secure-session open, revoke, and
reject evidence for the current secure baseline without presenting legacy
persistent freshness or reopen-floor behavior as active runtime truth.

#### Scenario: Secure-session lifecycle evidence stays reviewable
- **WHEN** the current secure baseline opens, revokes, or rejects secure
  command activity
- **THEN** it SHALL emit dedicated reviewable event and telemetry evidence
  including opened-session, revoked-session, accepted-sequence, and reject
  counters
- **AND** it SHALL keep that evidence distinct from `BootManager` boot metadata
  truth.

#### Scenario: Historical legacy lifecycle claims stay archived only
- **WHEN** historical compatibility evidence cites legacy `SESSION_OPEN` or
  reopen-floor persistence
- **THEN** that citation SHALL remain reviewable as archived evidence only
- **AND** it SHALL NOT be treated as current runtime replay or lifecycle truth.

### Requirement: Reboot Requires Fresh Session Open

Runtime restart SHALL clear active secure session state and require a fresh
secure-auth cycle before later secure commands may dispatch, and the current
baseline SHALL NOT depend on persisted legacy reopen-floor semantics after
restart.

#### Scenario: Restart clears active secure session state
- **WHEN** the hosted runtime or equivalent topology process restarts
- **THEN** all active in-memory secure session state and secure sequence state
  SHALL be cleared
- **AND** later secure command traffic SHALL fail closed until a fresh secure
  auth grant re-establishes the session.

#### Scenario: Legacy reopen-floor persistence is not part of current baseline
- **WHEN** reviewers inspect current restart behavior
- **THEN** they SHALL find current runtime restart closure described in terms
  of fresh secure-auth re-bootstrap
- **AND** they SHALL NOT find persisted legacy `SESSION_OPEN` reopen-floor
  semantics described as current maintained runtime contract.

### Requirement: Command Envelope V1 Supports Authenticated Source Binding

The repository SHALL keep `source_id`, `key_slot`, and `session_id` as
historical legacy v1 envelope vocabulary only, while the current secure command
v2 property SHALL bind trust to the active secure authorization state keyed by
ingress and service.

#### Scenario: Secure command v2 omits source and key-slot fields
- **WHEN** `CommandIngressAuthority` receives a secure command v2 candidate
- **THEN** the secure command v2 contract SHALL include only secure sequence
  number, inner command payload, and MAC material
- **AND** it SHALL bind trust to the previously granted active secure auth
  state for that `(ingressPort, serviceId)`
- **AND** it SHALL NOT require wire-level `source_id`, `key_slot`, or
  `session_id` fields.

#### Scenario: Legacy tuple fields remain historical only
- **WHEN** current runtime docs, configs, or proofs describe command-ingress
  trust anchors
- **THEN** they SHALL use the secure-auth state keyed by ingress and service
- **AND** any legacy `source_id`, `key_slot`, or `session_id` discussion SHALL
  be explicitly historical or compatibility-only.

### Requirement: Auth Verification Precedes Policy And State Mutation

Authenticated command processing SHALL remain parse-auth-policy ordered on both
legacy v1 and secure command v2 paths, with secure command v2 additionally
requiring a previously granted secure authorization state before policy or
dispatch may proceed.

#### Scenario: Secure command v2 uses parse-auth-policy ordering
- **WHEN** `CommandIngressAuthority` receives a secure command v2 candidate
- **THEN** it SHALL process that traffic in this order:
  1. parse secure command v2
  2. verify an active secure authorization state exists
  3. verify secure command MAC
  4. evaluate authority
  5. evaluate sequence
  6. dispatch

#### Scenario: Secure MAC failure rejects before policy or sequence mutation
- **WHEN** secure command v2 parse fails, active auth state is absent, or MAC
  verification fails
- **THEN** `CommandIngressAuthority` SHALL reject the command before
  authority, sequence, or dispatch
- **AND** it SHALL forward zero inner command buffers.

### Requirement: Auth, Lifecycle, And Sequence Mutations Stay Separate

Authenticated rejection paths SHALL keep secure auth state, secure sequence
state, and legacy lifecycle state boundaries explicit.

#### Scenario: Secure auth revoke clears secure state without mutating legacy v1
- **WHEN** `CommandIngressAuthority` receives `authRevoked(ingressPort,
  serviceId, reason)`
- **THEN** it SHALL clear the active secure session for that
  `(ingressPort, serviceId)`
- **AND** it SHALL leave any legacy v1 lifecycle state on adjacent ingress
  paths unchanged.

#### Scenario: Secure authority denial does not consume secure sequence
- **WHEN** secure command v2 MAC verification succeeds but authority denies the
  inner command
- **THEN** the gate SHALL reject before dispatch
- **AND** it SHALL NOT consume secure sequence state.

### Requirement: Authenticated Envelope Evidence Is Distinct From Raw Observation

Authenticated command acceptance SHALL be distinguishable from low-level envelope detection or parse-only observation.

#### Scenario: Auth-failed traffic is not presented as authenticated acceptance
- **WHEN** auth verification fails
- **THEN** runtime evidence SHALL NOT report that command as authority-accepted, lifecycle-valid, or session-observed in a way that can be confused with authenticated acceptance.

### Requirement: Legacy Routed Commands Remain Explicit Compatibility Path

Legacy non-envelope routed commands MAY remain supported during authenticated envelope adoption, but they SHALL remain outside the authenticated command-security claim.

#### Scenario: Legacy compatibility does not inherit authenticated privileges
- **WHEN** `CommandIngressAuthority` receives a legacy non-envelope routed command
- **THEN** it SHALL preserve the existing legacy authority behavior
- **AND** it SHALL NOT treat that traffic as authenticated
- **AND** it SHALL NOT silently inherit authenticated session semantics or authenticated-evidence meaning.

### Requirement: Authenticated Envelope Claims Stay Bounded

The new challenge-authenticated secure command path SHALL provide authenticated
secure session bootstrap plus per-session command integrity and SHALL NOT
over-claim encryption, hardware-backed storage, or full replay protection.

#### Scenario: Secure challenge auth stays bounded
- **WHEN** the secure challenge-authenticated command path is cited
- **THEN** it SHALL be valid to claim service-bound challenge/response session
  bootstrap, active-session HMAC command verification, and strict-monotonic
  per-session sequence rejection
- **AND** it SHALL NOT be cited as encryption, persistent secure key storage,
  hardware-backed security, file authority, unknown packet authority, or full
  replay protection outside the active secure session.

### Requirement: Core Recovery Status Surface Is Bounded
The core-system-contracts capability SHALL expose a bounded shared-recovery status surface on the active baseline.

#### Scenario: Shared recovery status covers EPS, ADCS, and COMM incidents
- **WHEN** the operator issues `GET_RECOVERY_STATUS` after `multi-subsystem-fdir-v1`
- **THEN** the active runtime SHALL be able to report bounded shared recovery source, current and highest level, last action, relatch truth, and pending reboot truth for `EPS`, `ADCS`, and `COMM` incidents in addition to watchdog incidents

#### Scenario: COMM recovery action truth is reviewable without implying reset cause
- **WHEN** the shared recovery owner performs bounded COMM failover actuation
- **THEN** the active runtime SHALL expose that COMM recovery action through recovery status, events, and/or telemetry
- **AND** it SHALL NOT imply that a COMM failover action alone rewrote persisted boot reset-cause truth

#### Scenario: Only reboot intent changes persisted reset-cause truth
- **WHEN** the operator later inspects `GET_RESET_CAUSE`
- **THEN** the persisted reset-cause value SHALL change only after a bounded shared recovery reboot intent
- **AND** non-reboot actions such as restart intent, subsystem interface reset, `SAFE` fallback, or COMM failover SHALL remain outside persisted reset-cause truth

### Requirement: TTC Pass Policy Runtime Surface
The core system contracts capability SHALL provide a bounded runtime TTC pass-policy surface instead of leaving `TTC` as a pure manual shell.

#### Scenario: TTC config is bounded
- **WHEN** TTC pass policy is configured at runtime
- **THEN** the active contract SHALL expose `enabled` and `loss_of_lock_timeout_sec`
- **AND** it SHALL NOT require generic scheduler payloads, queued activities, TLE uploads, or payload-operation plans

#### Scenario: Pass-window contract uses one epoch interval
- **WHEN** TTC pass policy accepts pass-window truth at runtime
- **THEN** it SHALL accept one active window using `start_unix_sec` and `end_unix_sec`
- **AND** it SHALL provide an explicit clear-window action
- **AND** it SHALL NOT claim a generic multi-window scheduler or onboard pass-prediction engine

#### Scenario: Invalid pass windows are rejected fail-closed
- **WHEN** a TTC pass window is configured with `start_unix_sec == 0`, `end_unix_sec == 0`, or `end_unix_sec <= start_unix_sec`
- **THEN** the runtime SHALL reject that window input
- **AND** the runtime SHALL NOT auto-enter `TTC` from that invalid input

### Requirement: TTC Pass Policy Entry And Exit
The core system contracts capability SHALL define deterministic TTC pass-policy entry and exit behavior for the active baseline.

#### Scenario: TTC auto-entry requires bounded guards
- **WHEN** current mode is `IDLE`
- **AND** TTC config `enabled == true`
- **AND** a pass window is configured
- **AND** cached GPS time basis is valid
- **AND** current GPS-derived Unix epoch time is inside the configured window using `start <= now < end`
- **THEN** the TTC pass-policy owner SHALL request `TTC`

#### Scenario: TTC auto-entry does not bypass non-IDLE modes
- **WHEN** the pass window is active and GPS time basis is valid
- **AND** current mode is not `IDLE`
- **THEN** the TTC pass-policy owner SHALL NOT force a cross-mode takeover in this change

#### Scenario: TTC auto-exit occurs when window ends
- **WHEN** current mode is `TTC`
- **AND** the configured pass window is no longer active
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

#### Scenario: TTC auto-exit occurs when TTC is disabled
- **WHEN** current mode is `TTC`
- **AND** TTC config `enabled == false`
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

#### Scenario: TTC auto-exit occurs when GPS validity is lost
- **WHEN** current mode is `TTC`
- **AND** the cached GPS time basis becomes invalid
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

#### Scenario: TTC auto-exit occurs on bounded COMM loss timeout
- **WHEN** current mode is `TTC`
- **AND** both `sbandAvailable == false` and `uhfAvailable == false`
- **AND** that condition persists for longer than `loss_of_lock_timeout_sec`
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

### Requirement: GPS Time Basis For TTC Policy
The core system contracts capability SHALL use bounded cached GPS truth for TTC pass-window evaluation.

#### Scenario: GPS time basis requires cached fix truth
- **WHEN** TTC pass policy evaluates GPS validity
- **THEN** it SHALL require `hasSample == true`
- **AND** it SHALL require `fixValid == true`
- **AND** it SHALL require nonzero UTC date and second-of-day fields

#### Scenario: GPS time basis requires freshness
- **WHEN** TTC pass policy evaluates cached GPS validity
- **THEN** it SHALL require `acceptedSentenceCount` to have advanced within a fixed bounded freshness threshold
- **AND** it SHALL fail closed if freshness cannot be proven

#### Scenario: TTC policy converts cached UTC to epoch internally
- **WHEN** TTC pass policy compares the current time to the configured epoch window
- **THEN** it SHALL convert cached GPS `utcDateYmd` and `utcSecondsOfDay` to Unix epoch time inside the TTC pass-policy logic
- **AND** it SHALL fail closed on impossible UTC fields or conversion failure
- **AND** it SHALL NOT treat component wall-clock time as the authoritative source for pass-window comparison

### Requirement: TTC Policy Coexists With Manual TTC Path
The core system contracts capability SHALL keep manual TTC entry available while making TTC retention policy-owned.

#### Scenario: Manual entry remains available
- **WHEN** an operator requests `IDLE -> TTC` through the existing guarded operator mode path
- **THEN** the operator path SHALL remain available in this change

#### Scenario: Active TTC remains governed by policy
- **WHEN** current mode is `TTC`
- **THEN** the TTC pass-policy owner SHALL evaluate the same exit guards whether `TTC` was entered manually or automatically

#### Scenario: Invalid manual TTC does not persist
- **WHEN** an operator manually enters `TTC`
- **AND** TTC policy conditions are not satisfied on the next policy cycle
- **THEN** the TTC pass-policy owner SHALL request `IDLE`

#### Scenario: Manual exit remains available
- **WHEN** an operator requests `TTC -> IDLE` or `TTC -> SAFE`
- **THEN** the manual request SHALL remain available through the existing guarded operator path

### Requirement: External Sequence Execution Requires Repo-Owned Admission

Externally requested sequence execution on the active baseline SHALL be admitted by a repo-owned owner before any official sequence engine runs the file.

#### Scenario: External stock sequence commands are denied directly

- **WHEN** comm-managed ingress submits `SeqDispatcher.RUN`, `SeqDispatcher.RUN_ARGS`, or direct `CmdSequencer` `CS_RUN` / `CS_START` / `CS_STEP` / `CS_AUTO` / `CS_MANUAL` / `CS_JOIN_WAIT` / `CS_CANCEL`
- **THEN** the active command authority path SHALL reject the request before stock sequence execution begins

#### Scenario: Wrapper-controlled sequence admission checks the full file contents

- **WHEN** a caller uses the repo-owned sequence wrapper surface
- **THEN** the admission owner SHALL validate official sequence format and CRC
- **AND** it SHALL deserialize each record into a complete `Fw::CmdPacket`
- **AND** it SHALL verify opcode presence, legal argument serialization, and per-inner-command authority against the caller profile
- **AND** it SHALL reject the sequence if any inner command violates caller authority

### Requirement: Sequence Execution Uses Immutable Admitted Copies

The active sequence wrapper SHALL prevent execution-time dependence on mutable staged files.

#### Scenario: Admitted copy is executed instead of staged source

- **WHEN** a staged sequence file passes admission
- **THEN** the wrapper SHALL copy it into an admission-owned path before execution
- **AND** later run or manual control SHALL reference the admitted copy, not the original staged path

### Requirement: Backup Sequence Control Is Ownership Bound

`uhf-backup` sequence control SHALL remain bounded by both inner-command authority and context ownership.

#### Scenario: Backup can only act on backup-owned admitted contexts

- **WHEN** `uhf-backup` requests validate, run, prepare-manual, start, step, or cancel
- **THEN** the wrapper SHALL allow those operations only for contexts admitted under backup ownership and only when every inner command is backup-allowed
- **AND** it SHALL reject requests that target a context owned by a higher-authority ingress

### Requirement: SystemResources Enable Control Is Not Backup-Writable

The active `SystemResources.ENABLE` surface SHALL be governed as a runtime configuration control, not a backup-writable status surface.

#### Scenario: Backup cannot change SystemResources enable state

- **WHEN** `uhf-backup` attempts to invoke `SystemResources.ENABLE`
- **THEN** the command authority path SHALL reject it

### Requirement: Resource-Truth Reclassification Does Not Weaken SystemResources Enable Governance

The core-system-contracts capability SHALL preserve the governed
runtime-configuration status of `SystemResources.ENABLE` after
`SystemResources.*` is removed from formal node-`5` pass-time truth.

#### Scenario: Enable control remains governed after the observability split
- **WHEN** current docs or authority policy describe `SystemResources.ENABLE`
- **THEN** they SHALL keep it as a controlled runtime configuration surface
- **AND** they SHALL NOT reinterpret the command as a baseline status or
  backup-writable observability surface.

### Requirement: Historical Persistent Freshness Evidence Stays Archived

The active secure-auth baseline SHALL NOT depend on a dedicated persisted
legacy command-freshness store, and any older persistent-freshness records
SHALL remain archived historical evidence only.

#### Scenario: Current baseline does not require the old freshness store
- **WHEN** reviewers inspect the current secure command-ingress runtime
- **THEN** they SHALL find restart closure described through fresh secure-auth
  re-bootstrap
- **AND** they SHALL NOT find a maintained requirement that
  `CommandIngressAuthority` load or enforce persisted legacy reopen floors.

#### Scenario: Archived persistence records remain reviewable only
- **WHEN** historical persistent-freshness evidence is cited
- **THEN** it SHALL remain reviewable as archived compatibility ancestry
- **AND** it SHALL NOT be treated as current maintained replay or lifecycle
  truth.

### Requirement: Payload Commands Use The Shared Authority Model

The core system contracts capability SHALL extend the governed command
authority vocabulary to cover the public camera payload contract.

#### Scenario: Payload resource labeling is explicit

- **WHEN** the command authority catalog classifies `PAYLOAD_*` commands
- **THEN** it SHALL use a governed payload command class and payload resource
  label instead of overloading an unrelated subsystem resource

#### Scenario: Payload readback is distinguishable from mutating control

- **WHEN** the authority catalog classifies `PAYLOAD_GET_STATUS`
- **THEN** it SHALL be treated as read/status behavior
- **AND** payload mutating commands such as prepare, capture, abort, shutdown,
  and defaults update SHALL remain governed as payload-control behavior

### Requirement: Payload Mode Entry Remains Free Of Automatic Side Effects

The core mode shell contract SHALL continue to keep `PAYLOAD` mode entry
separate from real payload execution even after the first payload operation
slice lands.

#### Scenario: PAYLOAD mode does not auto-start the camera contract

- **WHEN** an operator or internal safety path enters `PAYLOAD`
- **THEN** the runtime SHALL update only the approved mode state, event, and
  telemetry contract
- **AND** it SHALL NOT implicitly invoke `PAYLOAD_PREPARE`, camera power-on,
  image capture, or payload mission execution

### Requirement: Payload Operations Reuse Existing Runtime Ownership Boundaries

The active baseline SHALL keep payload admission and execution ownership
separate from mode truth, sequence truth, and resource observability truth.

#### Scenario: Payload owner does not replace existing mode or sequence owners

- **WHEN** the payload operation path runs in the active topology
- **THEN** `PayloadOpsController` SHALL own payload admission and execution
- **AND** `ModeManager` SHALL remain the owner of current mode truth
- **AND** `SequenceAdmissionController` SHALL remain the owner of official
  sequence admission and control
- **AND** `SystemResources` SHALL remain observability only rather than a new
  payload arbitration owner

### Requirement: Raw Sensor Register Writes Stay On High-Authority Payload Surfaces

Raw sensor register writes SHALL remain bounded to the high-authority payload
control surface.

#### Scenario: Read-only or backup ingress cannot write registers

- **WHEN** a read-only or backup ingress surface requests a raw payload sensor
  register write
- **THEN** the system SHALL reject the request before payload execution

### Requirement: Payload Virtual CSP Identity Is Governed

The shared CSP contract SHALL reserve node `7` and application service ports
`40` through `49` for the payload virtual subsystem service.

#### Scenario: Payload virtual node is not ad hoc

- **WHEN** the payload subsystem-style service is implemented
- **THEN** it SHALL use governed payload-owned node and service identifiers

### Requirement: Node-One Shim Ports Stay Distinct From Future Node-Seven Allocation

The first in-process payload CSP service implementation SHALL use node-`1`
payload-owned shim ports that stay distinct from the future node-`7`
reservation.

#### Scenario: First implementation does not consume future split ports

- **WHEN** the repository hosts the first payload CSP service inside the OBC
  process
- **THEN** it SHALL keep node `7` and ports `40..49` reserved for future split
  deployment and use dedicated node-`1` shim ports instead

