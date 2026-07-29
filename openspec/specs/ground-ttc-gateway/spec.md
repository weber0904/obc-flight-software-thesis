# ground-ttc-gateway Specification

## Purpose
Define the governed omitted-RF ground TT&C gateway capability that will sit between ground-side tooling and the spacecraft-side comm subsystem while remaining distinct from the direct `GDS -> TCP -> OBC` development baseline.
## Requirements
### Requirement: Ground TT&C Gateway Capability Exists
The repository SHALL define a dedicated `ground-ttc-gateway` capability for the omitted-RF ground test chain, and that capability SHALL remain distinct from both the stock direct `fprime-gds -> TCP -> OBC` development path and the spacecraft-side `comm` subsystem.

#### Scenario: Gateway capability does not replace the direct GDS baseline
- **WHEN** the project adds the first omitted-RF TT&C architecture
- **THEN** it SHALL keep the existing direct `GDS -> TCP -> OBC` path available as a separate governed development baseline

#### Scenario: First gateway stays GDS-facing instead of bypassing GDS
- **WHEN** the project implements the first gateway-backed omitted-RF path
- **THEN** the repository SHALL keep stock `fprime-gds` as the ground-facing operator surface
- **AND** the governed gateway SHALL act as a repository-owned adapter between that GDS-side TCP path and the lab-side comm ingress

### Requirement: First Gateway Architecture Is Bidirectional
The first governed omitted-RF TT&C gateway architecture SHALL support both uplink and downlink traffic between ground-side tooling and the spacecraft-side comm path, and the first formal proof SHALL be bounded to `command`, `event`, and `telemetry` traffic.

#### Scenario: Gateway supports bounded uplink and downlink
- **WHEN** the first omitted-RF TT&C path is validated
- **THEN** the repository SHALL capture evidence for a bounded uplink path and a bounded downlink path instead of validating only one direction

#### Scenario: First gateway proof stays out of file/downlink scope
- **WHEN** the first gateway-backed omitted-RF path is recorded as formal evidence
- **THEN** that evidence SHALL identify the validated scope as bounded `command`, `event`, and `telemetry`
- **AND** it SHALL keep file/downlink behavior explicit as future scope

### Requirement: First Gateway Uses Lab-Side Serial Ingress
The first omitted-RF TT&C gateway SHALL be allowed to use a lab-side serial ingress that represents the RF-omitted boundary, and that ingress SHALL remain explicitly described as a lab transport rather than as a claim about flight RF behavior.

#### Scenario: Lab ingress does not over-claim RF equivalence
- **WHEN** the first gateway-backed omitted-RF TT&C path uses `macOS` to `subsystem.local` serial wiring
- **THEN** the repository SHALL describe that wiring as a lab ingress path and SHALL NOT describe it as RF validation

### Requirement: Gateway-First Integration Precedes Custom GDS Plugin Work

The repository SHALL continue to allow the active omitted-RF TT&C baseline to
use stock `fprime-gds` plus repo-owned gateway processes before any custom
multi-band GDS communication plugin becomes required.

#### Scenario: First TT&C slice avoids immediate GDS plugin coupling
- **WHEN** the project implements the first omitted-RF TT&C path
- **THEN** it MAY use a repository-owned ground gateway without first shipping
  a custom `fprime-gds` communication plugin

#### Scenario: First gateway reuses stock F' framing
- **WHEN** the repository implements the first governed ground gateway
- **THEN** that gateway SHALL reuse stock F' framing across the northbound GDS
  connection instead of requiring a custom GDS communication plugin for the
  first slice

#### Scenario: Near-term simultaneous multi-band operations use maintained separate stock stacks
- **WHEN** the current baseline needs simultaneous operator access to S-band
  and UHF before a custom orchestrated ground surface exists
- **THEN** the maintained hosted-first baseline SHALL use separate stock
  `fprime-gds` plus `ground_ttc_gateway` stacks per band
- **AND** the repository SHALL treat that as a smaller baseline step than a new
  custom GDS communication plugin or a new orchestration owner in the same
  change

#### Scenario: One gateway instance remains one-southbound
- **WHEN** reviewers inspect the current `ground_ttc_gateway` boundary
- **THEN** they SHALL see it described as one northbound GDS relay bound to one
  current southbound path
- **AND** they SHALL NOT treat one gateway instance as the current simultaneous
  S-band/UHF multiplexer baseline

### Requirement: Physical Lab Serial Gateway Validation Is Staged
The ground TT&C gateway SHALL support a staged physical lab serial validation flow that separates gateway-to-COMM uplink ingress from full bidirectional TT&C downlink proof.

#### Scenario: Gateway physical ingress is tested before full TT&C
- **WHEN** the repository validates the gateway over the physical macOS-to-`subsystem.local` serial link
- **THEN** the probe SHALL first verify that gateway-origin command bytes traverse the serial ingress into COMM and produce bounded OBC readback

#### Scenario: Physical serial acquisition preamble is explicit
- **WHEN** the physical lab serial gateway validation requires a gateway-origin acquisition preamble
- **THEN** `ground_ttc_gateway` SHALL only emit that preamble when explicitly configured
- **AND** the probe evidence SHALL record the preamble line count and delay before claiming useful TT&C traffic

#### Scenario: Full TT&C claim requires event and telemetry visibility
- **WHEN** the staged probe also observes command events through `fprime-cli events` and telemetry through `fprime-cli channels`
- **THEN** the evidence MAY describe the result as bounded physical lab serial TT&C
- **AND** without those observations it SHALL NOT describe the result as full TT&C

### Requirement: Physical Lab Serial Gateway Downlink Validation
The ground TT&C gateway SHALL support a focused physical lab serial validation flow where bounded downlink proof is registered only after gateway-mediated physical serial uplink has produced OBC command readback and ground-side tooling observes bounded event and telemetry output.

#### Scenario: Gateway downlink proof follows physical ingress
- **WHEN** the repository validates the gateway over the physical macOS-to-`subsystem.local` serial link
- **THEN** the downlink-focused probe SHALL first verify that gateway-origin command bytes traverse the serial ingress into COMM and produce bounded OBC readback

#### Scenario: Full physical TT&C claim requires ground-side observations
- **WHEN** the physical lab serial downlink probe observes both command events through `fprime-cli events` and telemetry through `fprime-cli channels`
- **THEN** the evidence MAY describe the result as bounded physical lab serial TT&C
- **AND** without those observations it SHALL NOT describe the result as full physical lab serial TT&C

#### Scenario: Gateway framing and preamble behavior stay explicit
- **WHEN** the physical lab serial gateway validation uses a gateway-origin acquisition preamble
- **THEN** `ground_ttc_gateway` SHALL emit that preamble only when explicitly configured
- **AND** the downlink evidence SHALL record the preamble line count and delay before claiming useful TT&C traffic

### Requirement: Gateway-Backed COMM Official Data-Product File Downlink Validation
The ground TT&C gateway SHALL support governed file/downlink validation where stock F' file downlink traffic traverses the existing gateway-backed COMM path and lands in the configured GDS file-storage directory.

#### Scenario: File proof follows bounded TT&C readiness
- **WHEN** the physical lab serial COMM file/downlink probe runs
- **THEN** it SHALL first verify gateway-backed command/event/channel TT&C readiness before claiming file/downlink success

#### Scenario: Ground storage receives bounded official data-product files
- **WHEN** `DpCatalog.START_XMIT_CATALOG` sends selected `.fdp` files through the gateway-backed COMM path
- **THEN** the configured GDS file-storage directory SHALL receive the expected official data-product files

#### Scenario: Gateway file evidence records transport settings
- **WHEN** the gateway-backed COMM file/downlink evidence is recorded
- **THEN** it SHALL record the gateway transport, serial endpoint settings, preamble settings when used, GDS ports, file-storage directory, and selected `.fdp` file identifiers

#### Scenario: Gateway contract remains stock F' northbound
- **WHEN** the file/downlink validation runs
- **THEN** the gateway SHALL keep stock F' framing toward `fprime-gds`
- **AND** the change SHALL NOT require a custom GDS communication plugin

### Requirement: Gateway-Backed COMM SocketCAN TT&C Validation
The ground TT&C gateway SHALL support governed validation where bounded command/event/channel TT&C traverses lab serial ingress into COMM node `4`, then reaches target OBC over the shared SocketCAN carrier.

#### Scenario: Gateway path continues through SocketCAN COMM
- **WHEN** the COMM SocketCAN TT&C probe runs
- **THEN** `ground_ttc_gateway` SHALL remain the stock F' framing adapter between GDS and the lab serial ingress
- **AND** COMM SHALL forward the ground-link chunks to OBC through SocketCAN rather than hosted ZMQHUB

#### Scenario: Gateway verdict requires downlink observations
- **WHEN** the probe claims bounded TT&C over COMM SocketCAN
- **THEN** it SHALL observe command events through `fprime-cli events`
- **AND** it SHALL observe `GROUND_LINK_TX_BYTES` through `fprime-cli channels`

#### Scenario: Gateway evidence records both ingress and internal carrier
- **WHEN** COMM SocketCAN TT&C evidence is recorded
- **THEN** it SHALL record the serial ingress endpoint settings, COMM node id, CAN interface mapping, CAN timing, GDS ports, and active CAN health

### Requirement: Gateway-Backed COMM SocketCAN Official Data-Product File Downlink Validation
The ground TT&C gateway SHALL support governed file/downlink validation where stock F' file downlink traffic traverses lab serial ingress into COMM node `4`, reaches target OBC over the shared SocketCAN carrier, and returns files to the configured GDS file-storage directory.

#### Scenario: File proof follows SocketCAN TT&C readiness
- **WHEN** the COMM SocketCAN file/downlink probe runs
- **THEN** it SHALL first verify gateway-backed command/event/channel TT&C readiness over the same SocketCAN-backed COMM path before claiming file/downlink success

#### Scenario: Ground storage receives bounded official data-product files
- **WHEN** `DpCatalog.START_XMIT_CATALOG` sends selected `.fdp` files through the SocketCAN-backed COMM path
- **THEN** the configured GDS file-storage directory SHALL receive the selected official data-product files

#### Scenario: Gateway accepts bounded transfer retries
- **WHEN** a received `.fdp` file does not byte-match its target OBC runtime source snapshot during bounded SocketCAN-backed file/downlink validation
- **THEN** the probe MAY retry the same existing `DpCatalog` transmit command within the configured attempt limit
- **AND** success SHALL be claimed only for the final received file that byte-matches the snapshot

#### Scenario: Gateway retry boundary stays whole-command only
- **WHEN** SocketCAN-backed COMM file/downlink validation uses bounded retries
- **THEN** those retries SHALL be treated as whole-command retries
- **AND** the verdict SHALL NOT claim missing file-packet retransmission, NACK/ARQ recovery, or reliable file transfer under packet loss

#### Scenario: Gateway file evidence records SocketCAN transport settings
- **WHEN** the SocketCAN-backed COMM file/downlink evidence is recorded
- **THEN** it SHALL record gateway serial endpoint settings, preamble settings, GDS ports, GDS file-storage directory, COMM node id, CAN interface mapping, CAN timing, and selected `.fdp` file identifiers

#### Scenario: Gateway contract remains stock F' northbound
- **WHEN** SocketCAN-backed file/downlink validation runs
- **THEN** the gateway SHALL keep stock F' framing toward `fprime-gds`
- **AND** the change SHALL NOT require a custom GDS communication plugin

### Requirement: Lab Target Ground Launcher
The ground TT&C gateway SHALL provide a repo-owned launcher for the lab target COMM CSP operational path that starts stock `fprime-gds` and `ground_ttc_gateway` with reviewable operator settings.

#### Scenario: Ground launcher records operator-facing settings
- **WHEN** the lab target ground launcher starts
- **THEN** it SHALL print the GDS bind address, GDS IP port, GDS TTS port, GDS file-storage directory, gateway serial endpoint, baudrate, preamble line count, and preamble delay
- **AND** it SHALL keep stock F' framing toward `fprime-gds`

#### Scenario: Ground launcher stays a lab operator surface
- **WHEN** the launcher is documented or used as evidence
- **THEN** it SHALL be described as the macOS lab ground surface for the RF-omitted path
- **AND** it SHALL NOT claim RF validation or a custom GDS communication plugin

### Requirement: Lab Target Operator Runbook
The ground TT&C gateway capability SHALL include an operator runbook for the lab target COMM CSP path.

#### Scenario: Runbook describes complete lab operations
- **WHEN** an operator follows the runbook
- **THEN** it SHALL cover OBC package/install, OBC service migration, CAN provisioning, subsystem services, ground launcher startup, status and journal inspection, rollback to the older installed service, and E2E command/file verification

#### Scenario: Runbook preserves proof boundaries
- **WHEN** the runbook describes expected results
- **THEN** it SHALL distinguish lab operational readiness from final flight deployment
- **AND** it SHALL keep RF, no-preamble first-byte-clean behavior, reliable retransmission, arbitrary file downlink, ScenarioBridge/pass automation, and final OS CAN provisioning out of scope

### Requirement: Gateway Supports S-band TCP Southbound Segment
The ground TT&C gateway SHALL support a hosted S-band simulated TCP southbound segment while keeping stock F' framing toward `fprime-gds`.

#### Scenario: Gateway connects to S-band TCP COMM endpoint
- **WHEN** the hosted S-band TCP ground-link probe runs
- **THEN** `ground_ttc_gateway` SHALL connect southbound to the configured S-band TCP endpoint owned by `sband_comm_csp_node`
- **AND** it SHALL keep the northbound GDS connection as stock F' framing

#### Scenario: Gateway evidence separates GDS TCP from S-band TCP
- **WHEN** S-band TCP gateway evidence is recorded
- **THEN** it SHALL record the GDS IP/TTS ports separately from the S-band simulated TCP endpoint
- **AND** it SHALL not describe direct `GDS -> TCP -> OBC` as the S-band-through-COMM verdict

### Requirement: Gateway-Backed S-band TCP Official Data-Product File Downlink Validation
The ground TT&C gateway SHALL support bounded official data-product file/downlink validation over the hosted S-band TCP COMM path.

#### Scenario: File proof follows S-band TT&C readiness
- **WHEN** the hosted S-band TCP file/downlink probe runs
- **THEN** it SHALL first verify bounded command/event/channel TT&C over the same S-band TCP path before claiming file/downlink success

#### Scenario: Ground storage receives bounded official data-product files
- **WHEN** `DpCatalog.START_XMIT_CATALOG` sends selected `.fdp` files through the S-band TCP COMM path
- **THEN** the configured GDS file-storage directory SHALL receive one or more selected official data-product files

#### Scenario: S-band file evidence records byte matches
- **WHEN** hosted S-band TCP file/downlink evidence is recorded
- **THEN** it SHALL record source snapshots, received file paths, byte counts, and byte-comparison verdicts for the selected `.fdp` files
- **AND** the verdict SHALL NOT claim arbitrary file downlink or reliable retransmission under packet loss

### Requirement: Gateway Supports UHF Hosted Serial Southbound Segment
The ground TT&C gateway SHALL support a hosted UHF serial southbound segment while keeping stock F' framing toward `fprime-gds`.

#### Scenario: Gateway connects to UHF serial COMM endpoint
- **WHEN** the hosted UHF UART backup probe runs
- **THEN** `ground_ttc_gateway` SHALL use its configured serial southbound endpoint to exchange stock F' uplink/downlink bytes with `uhf_comm_csp_node` node `6`
- **AND** it SHALL keep the northbound GDS connection as stock F' framing
- **AND** gateway logs/evidence SHALL identify the link identity as `uhf`

#### Scenario: Gateway evidence separates UHF serial from adjacent paths
- **WHEN** UHF gateway evidence is recorded
- **THEN** it SHALL record the GDS IP/TTS ports separately from the hosted UHF serial endpoints
- **AND** it SHALL not describe S-band TCP, direct `GDS -> TCP -> OBC`, or generic node `4` COMM as the UHF verdict

#### Scenario: UHF gateway scope remains bounded
- **WHEN** bounded backup command ingress is validated through the gateway
- **THEN** the evidence SHALL be limited to command ingress, command events, and channel observations over the hosted UHF serial path
- **AND** it SHALL NOT claim full command authority, failover policy, file/downlink, reliable transfer, RF, CCSDS, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation

### Requirement: Gateway Relays Default CCSDS Hosted Bytes Transparently
The ground TT&C gateway SHALL support default hosted CCSDS S-band adoption only as transparent raw-byte relay between GDS and the S-band COMM endpoint, with optional verification capture that does not change gateway semantics.

#### Scenario: GDS uses CCSDS framing for default hosted adoption
- **WHEN** the CCSDS hosted adoption proof runs
- **THEN** `fprime-gds` SHALL run with `--framing-selection space-packet-space-data-link`
- **AND** it SHALL use SCID `0x44`, VCID `1`, and TM frame size `1024`

#### Scenario: Gateway does not parse CCSDS
- **WHEN** CCSDS-framed traffic traverses `ground_ttc_gateway`
- **THEN** the gateway SHALL relay raw bytes between its northbound GDS TCP connection and southbound S-band TCP connection
- **AND** it SHALL NOT parse or validate CCSDS Space Packets, TC frames, TM frames, APIDs, SCID, VCID, sequence counts, commands, events, telemetry, or files

#### Scenario: Optional capture is verification-only
- **WHEN** raw byte capture is enabled on `ground_ttc_gateway`
- **THEN** the gateway SHALL tee relayed northbound and southbound bytes to configured capture files
- **AND** capture SHALL NOT alter relayed bytes, link identity, framing, command authority, retry behavior, or gateway parsing responsibilities

#### Scenario: Gateway verdict remains bounded
- **WHEN** gateway CCSDS compatibility is recorded
- **THEN** the verdict SHALL state that gateway compatibility proves transparent byte movement only
- **AND** it SHALL NOT claim gateway-level CCSDS semantic compatibility, reliable transfer, RF compatibility, command authority, or failover policy

### Requirement: Ground TT&C Evidence Keeps S-band And UHF COMM Policy Paths Distinct

The ground TT&C gateway documentation and evidence model SHALL keep the hosted S-band node-`5` and hosted UHF node-`6` COMM policy paths distinct even when one governed change proves them together as part of a COMM policy runtime closure.

#### Scenario: Command-policy evidence names the ingress boundary

- **WHEN** the COMM session-and-downlink QoS hosted probe records a command-policy verdict
- **THEN** the evidence SHALL identify whether the command traversed the S-band node-`5` or UHF node-`6` gateway boundary
- **AND** it SHALL not restate that verdict as a generic single-link ground path

#### Scenario: File/downlink evidence names the egress boundary

- **WHEN** the COMM session-and-downlink QoS hosted probe records a file/downlink verdict
- **THEN** the evidence SHALL identify the active primary file-transfer link used for that transfer
- **AND** it SHALL keep S-band and UHF file/downlink proof language separate

### Requirement: Gateway Process Remains A Single-Link Raw Relay

`ground_ttc_gateway` SHALL remain a raw relay process with one northbound GDS
connection and one active southbound link per process on the current baseline.

#### Scenario: One gateway process does not imply dual-link multiplexing
- **WHEN** current hosted or target/lab gateway behavior is described
- **THEN** one `ground_ttc_gateway` process SHALL be described as relaying
  either one serial southbound link or one TCP southbound link
- **AND** the repository SHALL NOT claim that one current gateway process is a
  simultaneous S-band/UHF multiplexer

### Requirement: Gateway Is Not The COMM Authority Or Reliable-Transfer Owner

`ground_ttc_gateway` SHALL remain outside command authority ownership, session
lifecycle ownership, link-role policy ownership, and reliable-transfer
ownership on the current baseline, including the default node-`5` and
explicit-switched node-`6` reliable-transfer slices.

#### Scenario: Gateway boundaries remain explicit
- **WHEN** gateway-backed COMM paths or reliable-transfer proofs are documented
- **THEN** `CommandIngressAuthority` and `CommController` SHALL remain the
  bounded OBC-side owners for command-session and link-role policy truth
- **AND** `ground_ttc_gateway` SHALL NOT be described as the authority owner,
  reliable-transfer engine, or dual-link policy orchestrator

#### Scenario: Reliable-transfer proofs keep the relay boundary path-specific
- **WHEN** reviewers inspect the bounded node-`5` or explicit-switched node-`6`
  reliable-transfer proof
- **THEN** they SHALL see `ground_ttc_gateway` cited only as the framed-byte
  relay between stock GDS and the current southbound link
- **AND** they SHALL NOT see gateway-owned resend, ARQ, NACK, or CFDP policy
  claimed for either path

### Requirement: Gateway Retry Helpers Stay Whole-Command Only

Any bounded retry behavior documented on current gateway-backed paths SHALL
remain ground-side whole-command helper behavior and SHALL NOT be treated as
packet retry or reliable-transfer logic inside `ground_ttc_gateway`.

#### Scenario: Gateway file or command retries do not expand gateway semantics
- **WHEN** a current gateway-backed proof repeats a command or downlink trigger
- **THEN** the repository SHALL describe that repeat as a bounded ground or
  probe resend of the same command surface
- **AND** it SHALL NOT describe `ground_ttc_gateway` as providing packet
  recovery, retransmission windows, ARQ, NACK handling, or CFDP behavior

### Requirement: Hosted Per-Band Stock Ground Stacks Are Maintained

The ground TT&C gateway capability SHALL provide maintained hosted-first
launcher surfaces for the current near-term simultaneous multi-band operator
baseline using one shared hosted runtime plus distinct stock S-band and UHF
ground stacks.

#### Scenario: Per-band launchers expose distinct operator surfaces
- **WHEN** an operator starts the maintained hosted S-band or UHF stock stack
- **THEN** the launcher SHALL report the stack-specific GDS port, TTS port,
  southbound endpoint, file-storage directory, and process-log locations
- **AND** it SHALL keep the S-band stock surface distinct from the UHF stock
  surface rather than presenting one stock GDS or one gateway process as the
  simultaneous baseline

#### Scenario: Combined wrapper remains composition-only
- **WHEN** an operator starts the maintained combined hosted wrapper
- **THEN** it SHALL compose one shared hosted runtime with the maintained
  S-band and UHF stock stacks
- **AND** it SHALL report startup order and coordinated shutdown or cleanup for
  the owned processes
- **AND** it SHALL NOT claim that the wrapper is a new runtime policy owner,
  multiplexer, or higher-level orchestration surface

#### Scenario: Shared simulator cleanup does not reap unrelated active hosted runs
- **WHEN** a maintained per-band launcher preflights or tears down hosted EPS
  or ADCS simulator processes whose command-line identity is shared across
  independent hosted runs
- **THEN** it SHALL limit stale cleanup for those shared simulator identities
  to orphaned leftovers unless a unique ownership marker is also present
- **AND** it SHALL NOT reap another active hosted stack or probe's EPS/ADCS
  simulator run solely because the other run uses the same simulator binary and
  node id on different CSP hub ports

### Requirement: Hosted Per-Band Operator Runbook Is Reviewable

The ground TT&C gateway capability SHALL include a dedicated hosted operator
runbook for the maintained per-band stock-stack baseline.

#### Scenario: Runbook records current operator truth
- **WHEN** an operator or reviewer follows the hosted per-band runbook
- **THEN** it SHALL identify which stack is the nominal S-band high-authority
  path, which stack exposes the bounded UHF node-`6` surface, the shared hosted
  runtime root, per-stack logs and artifacts, startup order, and shutdown or
  cleanup steps

#### Scenario: Runbook preserves current non-claims
- **WHEN** reviewers inspect the runbook boundary
- **THEN** it SHALL state that the maintained baseline uses two stock GDS
  processes and two gateway processes with separate southbound paths
- **AND** it SHALL keep explicit non-claims for one-GDS heterogeneous upstream
  handling, one-gateway multiplexer behavior, target-bearing simultaneous
  dual-link closure, and future orchestration completion

#### Scenario: Runbook distinguishes owned teardown from orphan-only simulator cleanup
- **WHEN** the runbook documents launcher cleanup ownership
- **THEN** it SHALL distinguish launcher-owned runtime roots and owned-process
  teardown from orphan-only cleanup of shared EPS/ADCS simulator identities
- **AND** it SHALL NOT imply that starting one maintained launcher may
  terminate another still-active hosted stack or probe on different CSP hub
  ports

### Requirement: Hosted Dual-Link Orchestration Owner Is Distinct From The Maintained Baseline

The ground TT&C gateway capability SHALL allow a hosted-only layer-2 dual-link
orchestration owner above the maintained per-band stock-stack baseline, and
that owner SHALL remain distinct from both the layer-1 composition-only wrapper
and the underlying per-band stock surfaces.

#### Scenario: Orchestration owner declares lifecycle ownership only
- **WHEN** the hosted orchestration owner starts
- **THEN** it SHALL declare itself as a hosted-only lifecycle owner for the
  combined operator surface
- **AND** it SHALL own lifecycle, startup-failure, and cleanup-summary state
- **AND** it SHALL NOT claim command authority ownership, gateway relay
  ownership, stock-GDS plugin behavior, COMM runtime ownership, or reliable
  transfer ownership

#### Scenario: Orchestration owner records delegated adjacent state explicitly
- **WHEN** reviewers inspect the hosted orchestration owner artifact
- **THEN** they SHALL see the maintained per-band stock-stack baseline recorded
  as a delegated prerequisite
- **AND** they SHALL see per-band TT&C semantics, gateway relay behavior,
  stock GDS behavior, and COMM runtime policy listed as non-owned adjacent
  state rather than as orchestration-owned state

### Requirement: Hosted Dual-Link Orchestration Runbook Is Reviewable

The ground TT&C gateway capability SHALL include a dedicated hosted runbook for
the layer-2 dual-link orchestration owner.

#### Scenario: Runbook records the three-layer boundary
- **WHEN** an operator or reviewer follows the hosted orchestration runbook
- **THEN** it SHALL distinguish:
  - the layer-1 maintained per-band stock-stack baseline
  - the layer-2 hosted orchestration owner
  - deferred future target-bearing simultaneous work
- **AND** it SHALL state that the orchestration surface is not a gateway
  multiplexer, not a stock-GDS plugin, and not a target-bearing proof surface

### Requirement: Future Target-Bearing Dual-Link Claim Stays Above Hosted Layers And One-Southbound Gateway

The future target-bearing simultaneous dual-link claim SHALL remain separate
from the current hosted layer-1 per-band stock baseline, the hosted layer-2
orchestration owner, and the one-southbound-per-process `ground_ttc_gateway`
boundary.

#### Scenario: Hosted layer-1 and layer-2 remain adjacent prerequisites only
- **WHEN** the repository describes the future target-bearing dual-link claim
- **THEN** it SHALL treat the hosted per-band stock stacks and the hosted
  orchestration owner as adjacent prerequisite layers only
- **AND** it SHALL NOT restate hosted layer-1 or hosted layer-2 proof as the
  target-bearing simultaneous claim itself

#### Scenario: Future target claim does not imply one-GDS or one-gateway closure
- **WHEN** reviewers inspect the same future target-bearing claim boundary
- **THEN** they SHALL see that one stock `fprime-gds` heterogeneous upstream
  aggregation and one `ground_ttc_gateway` simultaneous S-band/UHF multiplexing
  remain explicit non-claims
- **AND** they SHALL NOT treat a future target-bearing claim as proof that the
  current gateway or stock-GDS boundary already solved those ground-software
  problems

### Requirement: Target Dual-Link Proof Keeps Gateway Surfaces In The Operator Verdict Only

The ground TT&C gateway capability SHALL keep gateway-owned artifacts for the
first target-bearing dual-link proof limited to operator observability support.

#### Scenario: Gateway artifacts do not override target claim truth
- **WHEN** the first implementation-bearing target dual-link proof records
  gateway captures, ground events, or ground channels
- **THEN** those surfaces SHALL contribute only to the
  `operator-observability` verdict
- **AND** they SHALL NOT overturn passing target-side command truth by
  themselves

#### Scenario: Gateway boundary keeps its non-claims explicit
- **WHEN** the same proof is documented in current docs or evidence
- **THEN** it SHALL keep explicit non-claims for one-GDS heterogeneous
  upstream handling and one-gateway simultaneous S-band/UHF multiplexing
- **AND** it SHALL NOT restate the target-bearing proof as new gateway-owned
  policy or ownership

### Requirement: Manual Dual-GDS Operator Surface Lives In A Dedicated Subtree

The ground TT&C gateway capability SHALL provide its maintained manual
dual-GDS operator entrypoints from a dedicated `scripts/manual_ops/` subtree
rather than by scattering new operator wrappers across the root `scripts/`
surface.

#### Scenario: Manual operator family is easy to discover
- **WHEN** an operator needs the maintained manual dual-GDS surface
- **THEN** the repo SHALL provide hosted and target entrypoints under
  `scripts/manual_ops/`
- **AND** the root `scripts/README.md` SHALL route readers to that subtree
  instead of introducing new root-level `run_*` wrappers for this family.

### Requirement: Manual Dual-GDS Surface Keeps Stock GDS Plus Gateway Boundary

The maintained manual operator family SHALL keep stock `fprime-gds` plus
repo-owned `ground_ttc_gateway` as the current ground operator boundary.

#### Scenario: Manual operator UI does not replace stock GDS
- **WHEN** the manual dual-GDS surface is started in hosted or target mode
- **THEN** each band surface SHALL still use stock `fprime-gds` northbound of
  `ground_ttc_gateway`
- **AND** the operator truth SHALL remain two separate per-band GDS surfaces
  rather than one custom simultaneous multiplexer.

### Requirement: Manual Dual-GDS Surface Supports UI And Headless GDS Modes

The maintained manual dual-GDS operator family SHALL support `GDS_UI_MODE`
selection while preserving the current headless launcher defaults used by
existing proof and automation surfaces.

#### Scenario: Manual surface can run in human-facing UI mode
- **WHEN** an operator starts the maintained manual dual-GDS surface with
  `GDS_UI_MODE=ui`
- **THEN** the surface SHALL launch stock `fprime-gds` in UI-capable mode
- **AND** it SHALL still publish the same manifest/status contract as the
  corresponding headless run.

#### Scenario: Existing proof defaults remain headless
- **WHEN** existing maintained proof or probe wrappers use the shared GDS
  launchers without requesting UI mode
- **THEN** those wrappers SHALL keep the current headless behavior by default
- **AND** the manual operator addition SHALL NOT silently redefine them as GUI
  workflows.

