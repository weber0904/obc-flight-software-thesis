# platform-baseline Specification

## Purpose
Define the platform-wide baseline for profiles, bootstrap path, integration startup order, and the relationship between narrative source documents and formal main specs.
## Requirements
### Requirement: Narrative And Formal Baselines
The project SHALL maintain a two-layer documentation model consisting of narrative source documents in `obc-dev-spec/` and formal main specifications in `openspec/specs/`. When the two layers diverge, the formal main specifications SHALL take precedence and the narrative layer SHALL be reconciled in a later change.

#### Scenario: Formal baseline overrides narrative
- **WHEN** a narrative source statement conflicts with an archived formal main spec
- **THEN** future implementation and validation work SHALL follow the formal main spec

### Requirement: Supported Profiles And Modes
The platform SHALL define the `dev-macos`, `integ-rpi`, and `flight-hw-future` profiles, and profile switching SHALL be expressed through configuration, instance selection, endpoints, or device paths rather than duplicating business logic.

#### Scenario: Profile switch preserves core behavior
- **WHEN** the deployment changes from `dev-macos` to `integ-rpi`
- **THEN** the transport and endpoint configuration MAY change while the core application logic SHALL remain shared

### Requirement: Platform Integration Baseline
The project SHALL use F' v4.1.0 as the future bootstrap target, SHALL use `libcsp` as the internal subsystem network substrate, SHALL allow the hosted `dev-macos` profile to bind that internal network through libcsp's official ZMQHUB-backed transport, SHALL document the default baseline ports `6100`, `7100`, `8080`, and `50000` plus the startup order of the CSP hub or proxy, GDS, simulators, and OBC deployment, SHALL provide hosted `dev-macos` entrypoints for both a local REPL stack and a GDS-connected stack without requiring Raspberry Pi hardware, and SHALL provide repo-local `integ-rpi` entrypoints that can build and launch the same governed workspace on a Raspberry Pi through configuration, portable artifact discovery, and target-specific runtime paths instead of a separate unmanaged deployment tree.

#### Scenario: Near-term multi-host topology uses named roles
- **WHEN** the repository documents or configures the near-term multi-host topology
- **THEN** it SHALL describe `macOS` as the ground host, `obc` / `obc.local` as the OBC target host, and `subsystem-sim` / `subsystem.local` as the subsystem simulator host instead of treating one generic Raspberry Pi hostname as the only governed default

#### Scenario: OBC-target scripts keep backward compatibility while adopting role-based naming
- **WHEN** an existing Raspberry Pi helper script targets the OBC host
- **THEN** it SHALL resolve the OBC target through a canonical `OBC_SSH_TARGET` setting, SHALL continue to accept `RPI_SSH_TARGET` as a compatibility alias, and SHALL fall back to the governed default `operator@obc.local` when neither variable is provided

#### Scenario: Subsystem simulator host has a formal configuration surface
- **WHEN** a future change needs to target the subsystem-side Raspberry Pi host
- **THEN** the platform baseline SHALL already provide a formal `SUBSYSTEM_SIM_SSH_TARGET` configuration name whose default value is `operator@subsystem.local`, even if no full subsystem-host orchestration workflow has been added yet

### Requirement: Near-Term Target Serial Ownership Reflects Active Hardware Paths
The platform baseline SHALL describe direct OBC-attached GPS UART as the active near-term target GPS hardware path on `obc.local`, and it SHALL treat the former OBC-side serial comm path as deferred historical behavior until a later comm migration slice establishes a new active target hardware path.

#### Scenario: Target serial responsibilities stay explicit after GPS bring-up
- **WHEN** the repository describes the active target hardware baseline
- **THEN** it SHALL identify GPS as the active consumer of `obc.local` direct UART bring-up and SHALL keep current comm development on non-OBC-serial paths until later migration work completes

### Requirement: Communication Architecture Stages Are Governed Separately
The platform baseline SHALL treat software-only development, split-host development, and physical-link validation as three distinct but cumulative stages, and later stages SHALL preserve earlier stages as governed regression paths instead of replacing them.

#### Scenario: Physical-link work preserves software-only and split-host bring-up
- **WHEN** a later change introduces a physical carrier such as `CAN FD` or `UART`
- **THEN** the repository SHALL retain software-only and split-host validation paths as reusable governed baselines

### Requirement: Spacecraft-Side Internal Bus Direction Is Shared CAN FD
The near-term spacecraft-side physical carrier direction SHALL treat `EPS`, `ADCS`, and `COMM` as future CSP-facing subsystems whose traffic is intended to converge onto a shared `CAN FD` bus architecture, while direct sensor paths such as GPS remain separate where required.

#### Scenario: First governed physical internal carrier uses a CAN FD-capable SocketCAN bus
- **WHEN** the repository proves the first physical internal CSP carrier
- **THEN** that proof SHALL use a CAN FD-capable Linux SocketCAN bus for `EPS` and `ADCS`, SHALL keep GPS on direct OBC UART, and SHALL keep the direct `GDS -> OBC` path separate from the internal carrier change

#### Scenario: Shared CAN FD direction does not force GPS into the subsystem bus
- **WHEN** the repository plans future physical-carrier migration
- **THEN** it SHALL be able to move `EPS`, `ADCS`, and `COMM` toward shared `CAN FD` without requiring `GPS` to become part of that same subsystem bus baseline

### Requirement: Near-Term Hardware Role Allocation Is Reviewable
The platform baseline SHALL document the near-term host and bus role allocation used for future communication architecture work, including `obc.local` as the OBC host, `subsystem.local` as the subsystem-side host, direct OBC UART for GPS, and dual subsystem-side CAN channel groups where `EPS/ADCS` share one group and `COMM` uses the other.

#### Scenario: First physical internal CSP proof leaves the COMM channel reserved
- **WHEN** the first governed shared-bus proof is recorded
- **THEN** it SHALL identify one subsystem CAN channel as the active `EPS/ADCS` path and SHALL record the second subsystem channel as reserved and self-tested rather than as an active COMM path

#### Scenario: Future changes reuse the agreed host and bus allocation
- **WHEN** a later change begins the physical-link migration work
- **THEN** reviewers SHALL be able to cite one governed baseline statement describing the agreed OBC host, subsystem host, UART role, and subsystem-side CAN channel grouping

### Requirement: In-Place Fprime Bootstrap
The platform baseline SHALL support populating the existing OpenSpec-governed repository in place using `fprime-bootstrap project --populate --path . --tag v4.1.0`, and the resulting workspace SHALL preserve the `openspec/`, `obc-dev-spec/`, and project-local `.codex/skills/` structures.

#### Scenario: Bootstrap populates an existing governed workspace
- **WHEN** the project performs the initial F' bootstrap in this repository
- **THEN** it SHALL populate the current root directory and SHALL preserve the existing governance and narrative-document structures

### Requirement: Minimum Bootstrap Output
After the initial bootstrap, the workspace SHALL contain the generated F' baseline files needed for future work, including the project virtual environment, the root build/config files, and the near-term project directories required by the platform baseline.

#### Scenario: Baseline tree exists after bootstrap
- **WHEN** the bootstrap phase completes successfully
- **THEN** the workspace SHALL contain the generated virtual environment, root project files, and the baseline directories needed for OBC, simulator, script, documentation, and CI-oriented work

### Requirement: Bootstrap Interpreter Compatibility
The bootstrap virtual environment SHALL use a Python interpreter supported by the F' v4.1.0 dependency set. If the host default `python3` resolves to an unsupported version, the project SHALL create `fprime-venv/` with a compatible interpreter such as Python 3.13.

#### Scenario: Host default Python is too new
- **WHEN** the bootstrap host resolves `python3` to a version that cannot install the `v4.1.0` requirements successfully
- **THEN** the project SHALL recreate `fprime-venv/` with a compatible interpreter before running `fprime-util generate` and `fprime-util build`

### Requirement: Raspberry Pi Target Version Metadata
The governed `integ-rpi` build flow SHALL generate correct framework and project version metadata for the Raspberry Pi target even when the synced target workspace omits `.git`, and the target build path SHALL use host-derived version inputs instead of falling back to the framework default release string.

#### Scenario: Synced target workspace builds without git metadata
- **WHEN** the Raspberry Pi bootstrap flow builds the project from a synced workspace that excludes `.git`
- **THEN** the generated version metadata SHALL report the intended framework and project versions rather than the fallback `v3.5.0`

### Requirement: Raspberry Pi Installable Bundle
The governed `integ-rpi` profile SHALL provide a repo-local packaging flow that
emits a reviewable installable bundle for the Raspberry Pi target, and that
bundle SHALL include the Linux OBC runtime, the companion simulator executables
used by the integrated target flow, the deployment dictionary, and bundle
metadata describing the packaged framework and project versions.

#### Scenario: Target bundle is created from governed active artifacts
- **WHEN** the Raspberry Pi packaging flow runs after a successful target build
- **THEN** the repository SHALL produce a reviewable bundle artifact containing
  the installed-stack payload and bundle metadata without requiring the target
  to keep the full source workspace as the deployable unit
- **AND** the governed `bin/OBC` payload SHALL come from the active
  `build-artifacts/.../OBC` deployment rather than silently repackaging
  `OBC_ComFprimeLegacy` under an active name
- **AND** the packaged dictionary SHALL match the active `OBCApp` topology.

### Requirement: Raspberry Pi Installed Release Flow
The governed `integ-rpi` profile SHALL provide a repo-local install flow that can unpack a selected bundle into a fixed user-writable install root on the Raspberry Pi target, SHALL maintain a `current` release pointer, and SHALL support launching the integrated stack from that installed release path.

#### Scenario: Installed stack launches from current release
- **WHEN** an operator installs a governed Raspberry Pi bundle and launches the installed stack
- **THEN** the OBC runtime and simulator processes SHALL start from the installed release tree rather than from the synced source workspace

### Requirement: Raspberry Pi Service-Managed Startup
The governed `integ-rpi` profile SHALL provide a repo-local autostart flow that installs a systemd-managed service for the installed `current` release on the Raspberry Pi target, and that service SHALL launch the governed stack from the install root rather than from the synced source workspace.

#### Scenario: Target service starts the installed current release
- **WHEN** an operator installs and enables the governed Raspberry Pi autostart service
- **THEN** the target boot path SHALL start the OBC runtime and companion simulator stack from `$OBC_HOME/obc-deploy/current` or the configured install-root equivalent instead of the development workspace

### Requirement: Headless Installed Runtime Mode
The governed Raspberry Pi startup path SHALL support a headless runtime mode so the OBC process can run under systemd without depending on interactive stdin input, while the existing interactive launch path remains available for manual operator sessions.

#### Scenario: Service-managed runtime does not require REPL input
- **WHEN** the installed stack runs under the governed systemd service
- **THEN** the OBC runtime SHALL remain active without waiting for or failing on interactive stdin input

### Requirement: Libcsp Integration Base Is Released Through A Mainline Gate
The repository SHALL treat the libcsp integration base as a formal release candidate before it becomes the future mainline development baseline, and that release candidate SHALL pass the shared verification gate plus focused libcsp guardrails before merge to `main`.

#### Scenario: Libcsp base can be reviewed before mainline merge
- **WHEN** the libcsp integration branch is proposed for merge to `main`
- **THEN** reviewers SHALL be able to inspect a release-readiness evidence record that cites the completed CSP foundation, EPS CSP, ADCS CSP, and legacy-ZMQ retirement slices

### Requirement: Raspberry Pi Libcsp Baseline Is Validated Separately From Hosted
The repository SHALL provide a governed Raspberry Pi validation path that checks the libcsp internal subsystem substrate on the target profile after the hosted libcsp migration, without treating that result as ground-path, GPS, or real subsystem hardware validation.

#### Scenario: Target CSP baseline stays distinct from hosted proof
- **WHEN** the Raspberry Pi baseline validation runs
- **THEN** it SHALL identify OBC node `1`, EPS simulator node `2`, ADCS simulator node `3`, and the CSP hub settings used on the target

### Requirement: Internal CSP Carrier Selection Is Configuration-Driven
The platform baseline SHALL keep `libcsp` as the shared internal subsystem network contract while making the carrier or interface binding layer selectable through governed runtime configuration instead of embedding one hosted carrier directly into the runtime core.

#### Scenario: ZMQHUB remains the default development carrier
- **WHEN** the repository launches the existing hosted-local or Raspberry Pi local baselines without overriding the carrier selection
- **THEN** the internal CSP runtime SHALL continue to bind through the governed `zmqhub` carrier and SHALL preserve compatibility with `CSP_HUB_HOST`, `CSP_HUB_SUB_PORT`, and `CSP_HUB_PUB_PORT`

#### Scenario: Carrier abstraction does not change business-layer contracts
- **WHEN** the runtime carrier selection is refactored or extended
- **THEN** EPS and ADCS business traffic SHALL continue to use their subsystem-owned CSP contracts without requiring changes to the F' public command, telemetry, or event surface

#### Scenario: Deployed topology centralizes runtime ownership
- **WHEN** the active `OBC` deployment configures CSP-backed subsystem or COMM clients
- **THEN** the deployment SHALL inject one topology-owned runtime owner into those clients instead of relying on each client to claim direct ownership through `defaultRuntime()`
- **AND** hosted low-level probes MAY still use `defaultRuntime()` where no deployed topology owner exists

#### Scenario: Target OBC service restart re-arms governed SocketCAN bring-up
- **WHEN** `obc-comm-csp-stack.service` starts or restarts on the governed Raspberry Pi target path
- **THEN** it SHALL explicitly restart the governed `obc-lab-can.service` before launching the user-space OBC stack
- **AND** a service-managed `R2` process restart SHALL NOT rely on an earlier boot-time `can0` bring-up invocation to recover the OBC-side SocketCAN path

### Requirement: Remote Pi-To-macOS Internal CSP Topology Is Governed
The platform baseline SHALL allow a governed topology where the Raspberry Pi target runs only the OBC process while a remote macOS development host runs the internal CSP hub plus EPS and ADCS simulator nodes, and that topology SHALL keep internal CSP settings distinct from the remote GDS ground adapter settings.

#### Scenario: Pi target uses remote macOS host for both internal CSP and GDS
- **WHEN** the repository launches the governed remote topology
- **THEN** the Pi OBC process SHALL be able to point `CSP_HUB_HOST` and `GDS_HOST` at the remote macOS host while keeping the internal CSP ports and the GDS adapter port documented as separate configuration domains

### Requirement: Three-Host Split-Host CSP Topology Is Governed
The platform baseline SHALL allow a governed three-host topology where `macOS` runs the shared CSP hub and headless `fprime-gds`, `obc.local` runs only the OBC process, and `subsystem.local` runs only EPS node `2` plus ADCS node `3`.

#### Scenario: Host responsibilities stay explicit in the three-host topology
- **WHEN** the repository launches the governed three-host split-host topology
- **THEN** `macOS` SHALL host `csp_zmqproxy` plus GDS, `subsystem.local` SHALL host the subsystem simulator processes, and `obc.local` SHALL host only the OBC process instead of collapsing those roles back into one remote host

### Requirement: Subsystem Simulator Host Uses A Governed Workspace Flow
The platform baseline SHALL provide a governed workspace-based prep flow for `subsystem.local` using `SUBSYSTEM_SIM_SSH_TARGET` and `SUBSYSTEM_SIM_REMOTE_DIR`, and that flow SHALL remain distinct from the installed-bundle or systemd-managed OBC lifecycle.

#### Scenario: Subsystem simulator host builds from a synced workspace
- **WHEN** a developer prepares `subsystem.local` for the governed three-host topology
- **THEN** the repository SHALL be able to sync the workspace and build the simulator binaries on that host without requiring an installed bundle or autostart service there

### Requirement: Active COMM SocketCAN Channel Role
The platform baseline SHALL allow the subsystem-side `can1` channel to move from reserved-channel evidence into active COMM node `4` participation on the shared CAN FD-capable bus through a governed COMM SocketCAN TT&C change.

#### Scenario: COMM can1 role change is explicit
- **WHEN** the repository proves COMM SocketCAN TT&C participation
- **THEN** it SHALL record that `subsystem.local:can1` is active for COMM node `4`
- **AND** it SHALL preserve the earlier reserved-channel isolation evidence as historical to the EPS/ADCS-only SocketCAN slice

#### Scenario: CAN probes prepare interfaces before verdict
- **WHEN** a repository-owned hardware CAN smoke or probe judges physical SocketCAN connectivity
- **THEN** it SHALL first bring the required CAN interfaces `UP` with explicit bit timing
- **AND** a missing interface or failed privileged bring-up SHALL be reported as setup failure rather than physical bus failure

#### Scenario: Shared bus is not dual-bus redundancy
- **WHEN** COMM joins the shared SocketCAN carrier
- **THEN** the evidence SHALL describe one shared bus with `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1`
- **AND** it SHALL NOT claim dual-bus redundancy

### Requirement: Lab Target COMM CSP Operational Baseline
The platform baseline SHALL provide a lab target operational baseline where `obc.local` can boot into a service-managed installed OBC release that uses the proven COMM SocketCAN path as the ground link, while explicitly excluding final flight deployment claims.

#### Scenario: OBC lab service starts the installed release
- **WHEN** the lab target COMM CSP service is installed and enabled on `obc.local`
- **THEN** the service SHALL start OBC from the installed `current` release or configured install-root equivalent
- **AND** it SHALL run the OBC runtime as a non-root runtime process
- **AND** it SHALL configure `GROUND_LINK_MODE=comm-csp`, COMM node `4`, `CSP_TRANSPORT=socketcan`, `obc.local:can0`, and live GPS UART defaults

#### Scenario: Existing installed service is not left racing the lab service
- **WHEN** the helper enables the lab target COMM CSP OBC service
- **THEN** it SHALL disable and stop the older `obc-installed-stack.service`
- **AND** rollback SHALL be documented as disabling/stopping the lab service and re-enabling the older installed service

#### Scenario: Lab baseline is not final flight deployment
- **WHEN** the repository describes this baseline
- **THEN** it SHALL call it a lab target operational baseline
- **AND** it SHALL state that it is not a final flight deployment baseline

### Requirement: Transitional Runtime Identity Boundary
The lab target operational baseline SHALL keep long-running OBC and subsystem runtime processes non-root, while allowing the current workspace-service stage to run those processes as `operator` because the workspace and native build outputs are owned by that user.

#### Scenario: Workspace service stage uses current workspace owner
- **WHEN** OBC or subsystem runtime processes run under the lab service-managed workflow
- **THEN** they MAY run as `operator` for this workspace-based stage
- **AND** root SHALL be reserved for bounded provisioning actions such as CAN interface bring-up

#### Scenario: Future host-install runtime identity remains separate
- **WHEN** a future change introduces host-install service management beyond the workspace stage
- **THEN** it SHALL prefer dedicated non-login runtime users such as `obc-runtime` and `subsystem-runtime` rather than human workspace owners

### Requirement: Lab CAN Provisioning Is Transitional
The lab target operational baseline SHALL provide CAN bring-up as root-owned oneshot service templates for development and lab repeatability, while keeping final OS-level CAN provisioning out of scope.

#### Scenario: CAN oneshot configures the proven timing by default
- **WHEN** the lab CAN provisioning oneshot runs
- **THEN** it SHALL bring the configured CAN interface up with default timing equivalent to `bitrate 500000 dbitrate 2000000 fd on`
- **AND** the timing SHALL remain configurable through the repo-owned helper or service environment

#### Scenario: CAN oneshot is not described as flight provisioning
- **WHEN** specs, runbooks, or evidence describe the oneshot services
- **THEN** they SHALL describe them as lab/development provisioning helpers
- **AND** they SHALL NOT describe them as the final flight OS networking model

### Requirement: Release And Deployment Responsibility Split
The lab target operational baseline SHALL distinguish the OBC installable bundle from the broader repo-governed lab deployment artifacts.

#### Scenario: OBC package owns only OBC-installed artifacts
- **WHEN** the Raspberry Pi OBC package is created for this baseline
- **THEN** it SHALL include the OBC binary, dictionary, metadata, OBC comm-csp launch profile, and OBC service template
- **AND** it SHALL NOT claim that subsystem service templates, CAN oneshot templates, ground launchers, or operator runbooks are contents of the OBC tarball

#### Scenario: Repo-governed lab artifacts complete the path
- **WHEN** an operator follows the lab target operational runbook
- **THEN** subsystem service templates, CAN oneshot templates, ground launcher, and runbook SHALL be treated as repo-governed lab deployment artifacts outside the OBC installable tarball boundary

### Requirement: Hosted Runtime Refactors Preserve Operator Surface
The platform baseline SHALL preserve the hosted operator command surface and
public component contracts across hosted-runtime refactors without treating the
retired housekeeping archive file format as an active public contract.

#### Scenario: Existing operator commands remain stable
- **WHEN** a hosted runtime command-dispatch refactor is implemented
- **THEN** existing operator commands such as `status`, `mode`, `csp ping`, `eps`, `adcs`, `gps`, `storage`, `comm`, `radio`, `uart`, and `boot` SHALL remain available with their existing valid argument forms
- **AND** existing startup and status output markers used by repository-owned probes SHALL remain stable

#### Scenario: Refactor does not change public component contracts
- **WHEN** hosted runtime dispatch logic is refactored
- **THEN** the change SHALL NOT modify public F' component command opcodes, telemetry channels, event definitions, COMM CSP node IDs, COMM service ports, or COMM request/reply wire layouts

### Requirement: Default Hosted OBC Uses CCSDS Topology
The platform baseline SHALL make the default hosted and target-facing `OBC`
deployment the only maintained OBC deployment, and that deployment SHALL use the
active CCSDS `TopCcsds` topology while preserving currently available hosted OBC
capability surfaces except already-retired fallback surfaces.

#### Scenario: Target-facing helper defaults follow the active OBC deployment
- **WHEN** a governed source-workspace Raspberry Pi helper launches the current
  target-facing `OBC` runtime without an explicit override
- **THEN** that helper SHALL default to the active `OBC` binary
- **AND** it SHALL NOT silently default to, require, or document
  `OBC_ComFprimeLegacy` as a maintained runtime.

#### Scenario: Legacy Top is absent from maintained build registration
- **WHEN** the repository generates or builds the maintained OBC deployment
- **THEN** it SHALL NOT register `OBC_ComFprimeLegacy`, `OBC_Top`, or
  `OBC/Top` as maintained build targets
- **AND** historical evidence MAY cite those retired names only as archived
  context rather than as current build or runtime requirements.

### Requirement: Active TopCcsds Includes Official Sequencing and System Resource Services

The active `TopCcsds` baseline SHALL integrate official `Svc::CmdSequencer`, `Svc::SeqDispatcher`, and `Svc::SystemResources` as governed runtime services instead of adding a repo-native timed-command table.

#### Scenario: Official services are part of active topology

- **WHEN** the active topology is built and initialized
- **THEN** it SHALL instantiate two `CmdSequencer` instances, one `SeqDispatcher`, and one `SystemResources`
- **AND** it SHALL wire sequencer-emitted command traffic through dedicated `CmdDispatcher.seqCmdBuff` / `seqCmdStatus` indices distinct from the external SBAND/UHF ingress slot
- **AND** it SHALL keep one dispatcher sequence slot reserved for repo-owned internal sequence control traffic

### Requirement: Active Sequencing Timing Truth Is Bounded By Hosted Base Tick

The active official sequencing baseline SHALL state its timing truth relative to the configured deployment tick rather than mission time.

#### Scenario: Hosted timing truth is bounded and explicit

- **WHEN** evidence or docs claim absolute or relative sequence timing behavior
- **THEN** they SHALL identify the configured hosted base tick
- **AND** they SHALL state the worst-case dispatch latency bound in ticks
- **AND** they SHALL NOT claim GPS mission-time scheduling or RTC proof

### Requirement: Target Service-Managed R2 Recovery Restart
The governed `integ-rpi` profile SHALL allow the active service-managed OBC release to restart after a `RecoveryExecutor` R2 process-restart request through the same systemd-managed installed or lab service model that owns target startup.

#### Scenario: Systemd restarts OBC after R2 process exit
- **WHEN** the active OBC process exits with the bounded R2 process-restart exit code under the governed Raspberry Pi service
- **THEN** the owning launch script SHALL return a nonzero status to systemd
- **AND** systemd SHALL restart the OBC service through the configured `Restart=on-failure` policy
- **AND** the relaunched process SHALL come from the active `TopCcsds` `OBC` release path rather than the retired legacy topology

#### Scenario: Target R2 restart does not claim hardware reset
- **WHEN** the target service-managed R2 restart path is documented or cited as evidence
- **THEN** the evidence SHALL identify the path as managed OBC process/service restart
- **AND** it SHALL NOT claim Raspberry Pi hardware watchdog reset, Linux reboot, bootloader or partition handoff, power-loss recovery, RF behavior, or final flight deployment behavior

### Requirement: Raspberry Pi Baseline Supports Governed Hardware Watchdog Access

The governed Raspberry Pi baseline SHALL support repo-owned access to
`/dev/watchdog0` for the active non-root OBC service without changing the OBC
service to run as root.

#### Scenario: Service gains watchdog access through governed install config
- **WHEN** the repository installs or refreshes the active Raspberry Pi COMM CSP
  OBC service for hardware watchdog mode
- **THEN** the install path SHALL create or reuse a governed `watchdog` access
  group, install a repo-owned udev rule for `/dev/watchdog0`, and configure
  `obc-comm-csp-stack.service` to run with `SupplementaryGroups=watchdog`
- **AND** the active OBC service SHALL remain non-root

### Requirement: Raspberry Pi Launch Surface Exposes Hardware Watchdog Config

The governed Raspberry Pi launch surface SHALL expose explicit hardware watchdog
configuration while keeping hosted baselines disabled by default.

#### Scenario: Target launch passes hardware watchdog settings
- **WHEN** the active Raspberry Pi COMM CSP service launches the installed OBC
  stack in hardware watchdog mode
- **THEN** the launch path SHALL pass explicit watchdog mode, device path, and
  timeout settings to the OBC runtime
- **AND** hosted launch paths SHALL remain disabled by default unless explicitly
  overridden

### Requirement: Quiet Diagnostic Path SHALL Stay Probe-Owned

The governed Raspberry Pi launch/runtime surface SHALL keep any
diagnostic-only quiet egress control probe-owned, default off, and excluded
from the normal operator-facing baseline. It MAY expose that control only for
repo-owned target watchdog proof execution.

#### Scenario: Temporary quiet override suppresses unsolicited egress only for the proof
- **WHEN** the repository runs the target hardware-watchdog reset proof on the
  active Raspberry Pi service baseline
- **THEN** the proof MAY temporarily enable a diagnostic quiet egress override
  that suppresses unsolicited packet/file downlink on the active ground path
- **AND** the probe SHALL restore the service to normal non-quiet mode before
  exit
- **AND** the default installed service behavior SHALL remain unchanged when the
  override is absent

### Requirement: Target Hardware Watchdog Proof Stays Distinct From Other Reset Claims

The governed Raspberry Pi baseline SHALL record hardware watchdog reset as a
distinct proof boundary rather than collapsing it into service-managed restart,
Linux reboot, or power-loss recovery.

#### Scenario: Target hardware watchdog evidence stays narrowly scoped
- **WHEN** the active Raspberry Pi baseline cites hardware watchdog reset proof
- **THEN** the evidence SHALL identify the path as board reset caused by the
  Raspberry Pi hardware watchdog under the active `TopCcsds` service baseline
- **AND** it SHALL remain distinct from service-managed `R2` process restart,
  generic Linux reboot, bootloader or partition handoff, and power-loss
  recovery claims

### Requirement: Active TopCcsds Includes One Payload Owner

The active `TopCcsds` deployment SHALL include one repo-owned public payload
owner for OV5647-based Raspberry Pi CSI camera operations.

#### Scenario: Payload owner is present in the active runtime

- **WHEN** the active `TopCcsds` topology is built and configured
- **THEN** it SHALL instantiate `PayloadOpsController`
- **AND** it SHALL wire that owner into the normal command, event, telemetry,
  and runtime-support surfaces used by the active deployment

### Requirement: Camera Backend Split Is Truthful Across Hosted And Target Builds

The platform baseline SHALL describe and preserve the hosted-versus-target
camera backend split.

#### Scenario: Hosted builds use a contract-testing backend

- **WHEN** the hosted baseline is built on a development machine without target
  camera access
- **THEN** the payload implementation SHALL use a stub or fake camera backend
  that proves contract behavior without claiming real image-sensor interaction

#### Scenario: Raspberry Pi target builds use libcamera

- **WHEN** the Raspberry Pi target baseline is built for the real camera path
- **THEN** the payload implementation SHALL bind the camera backend through
  `libcamera`
- **AND** the public payload command contract SHALL remain unchanged across the
  hosted and target implementations

### Requirement: Payload CSP Service Lives Inside The OBC Process Before Split

The active baseline SHALL keep the real OV5647 camera local to the OBC Pi while
optionally exposing a payload CSP service inside the OBC process.

#### Scenario: First payload service surface does not imply payload relocation

- **WHEN** the active baseline exposes the first payload CSP service surface
- **THEN** it SHALL state that the camera remains attached to the OBC Pi and the
  service does not claim a separate payload processor

### Requirement: Node Seven Remains Reserved Until Process Split

The active baseline SHALL reserve payload virtual node `7` for future split
deployment while the first implementation uses payload-owned service ports on
local node `1`.

#### Scenario: Local service does not consume reserved node identity

- **WHEN** the repository documents or verifies the in-process payload CSP
  service
- **THEN** it SHALL distinguish the live node `1` service-port implementation
  from the reserved future node `7` identity

### Requirement: Node-One Shim Uses Bindable Local Ports

The first in-process payload CSP service SHALL use bindable local node-`1`
ports that do not overlap the current libcsp outgoing-client source-port band.

#### Scenario: Local payload CSP service avoids connection-source collisions

- **WHEN** the repository implements the node-`1` payload CSP shim
- **THEN** it SHALL use local payload service ports that are bindable on the
  current libcsp runtime and do not collide with client source ports `41+`

### Requirement: Target Camera Backend Uses Helper Isolation For Hard-Hang Recovery

The real target camera backend SHALL execute behind a helper-process boundary
when the active baseline claims bounded timeout and recovery from camera backend
stall behavior.

#### Scenario: Timeout kills the helper rather than hanging the OBC process

- **WHEN** a target camera operation exceeds its bounded deadline
- **THEN** the payload runtime SHALL be able to terminate the target helper and
  return the payload controller to a governed fault or cleanup state

### Requirement: Active Service-Managed Target Timing Profile Is Explicit

The active Raspberry Pi service-managed target baseline SHALL document the
deployed base tick, rate-group divisors, and resulting nominal fast/slow/data
rates used by the installed `obc-comm-csp-stack.service` path.

#### Scenario: Service-managed timing truth is reviewable

- **WHEN** the active service-managed target baseline is documented or cited as
  current timing truth
- **THEN** the documentation and evidence SHALL identify the deployed base tick
- **AND** they SHALL identify the configured rate-group divisors
- **AND** they SHALL state the resulting nominal fast, slow, and data-group
  rates for that deployed baseline

### Requirement: Target Missed-Tick Policy Uses Upstream ActiveRateGroup Semantics

The active service-managed target timing contract SHALL derive missed-tick
policy from upstream `Svc::ActiveRateGroup` cycle-slip semantics rather than
from repo-local guessed scheduler language.

#### Scenario: Zero-slip service-managed window is explicit

- **WHEN** the repository claims a passing service-managed target timing window
- **THEN** the claim SHALL mean that no `RateGroupCycleSlip` event was observed
  for the active rate groups during the declared observation window
- **AND** it SHALL NOT be described as broader final flight real-time closure
  unless a later governed change proves that stronger claim

### Requirement: Internal CSP Runtime Metrics Reads Stay Bounded Under Blocking Traffic

The active internal CSP runtime baseline SHALL let observability-only metrics
readers obtain a bounded runtime snapshot without waiting behind blocking
`ping` or request/reply traffic on the shared runtime instance.

#### Scenario: Metrics reads reuse cached state during blocking CSP traffic

- **WHEN** the maintained runtime is already inside a blocking CSP `ping` or
  `requestReply` operation on the active target node-`5` path
- **THEN** observability-only metrics readers SHALL return the latest cached
  runtime metrics snapshot instead of waiting for that blocking transaction
  critical section to complete
- **AND** the runtime SHALL preserve the existing serialization for the actual
  business-traffic operation that currently owns the runtime mutex

#### Scenario: Cached metrics refresh after runtime activity

- **WHEN** the runtime completes `init`, `ping`, `sendRaw`, `requestReply`, or
  `shutdown`
- **THEN** it SHALL refresh the cached metrics snapshot from the current runtime
  state before later observability-only readers reuse that snapshot
