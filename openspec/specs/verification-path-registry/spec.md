# verification-path-registry Specification

## Purpose
Define the repository-owned verification-path registry that records each formally proven validation path, the transport or port layering it covers, the governing archived evidence, and the adjacent paths that remain out of scope or unproven.
## Requirements
### Requirement: Repository Verification Path Registry
The repository SHALL maintain a reviewable verification-path registry under the checked-in documentation tree that names each formally proven validation path, the transport or port layering it covers, the governing archived evidence, and the adjacent paths it does not prove.

#### Scenario: Reviewer checks whether a path is already proven
- **WHEN** a developer or reviewer needs to know whether a validation path is already established in this repository
- **THEN** they SHALL be able to consult one repo-owned registry document that states the path name, scope, governing evidence, and out-of-scope neighboring paths

### Requirement: Path Registry Distinguishes Ground-Link Layers
The verification-path registry SHALL explicitly distinguish at least the direct `OBC -> GDS` TCP adapter path, the `fprime-cli -> GDS` command/uplink path, and any transparent-UART or framed-transparent external-comm path so they cannot be treated as interchangeable.

#### Scenario: GDS connectivity does not imply command-path coverage
- **WHEN** the repository has evidence proving direct `OBC -> GDS` TCP connectivity
- **THEN** the registry SHALL still describe `fprime-cli -> GDS` command/uplink behavior as a distinct path unless separate evidence has proven it

### Requirement: Registry Entries Cite Archived Evidence
Each verification-path registry entry SHALL cite the specific archived change or evidence record that proves the path instead of relying on generic framework knowledge.

#### Scenario: Registry entry points to governing record
- **WHEN** a registry entry declares a path formally proven
- **THEN** it SHALL cite the archived change or evidence file that reviewers can inspect to confirm that claim

### Requirement: GPS Hosted Replay Validation Path Is Registered
The repository verification-path registry SHALL include a dedicated entry for the first hosted GPS fake/replay validation path and SHALL describe that path as distinct from any still-unimplemented Raspberry Pi GPIO UART wiring or live-fix hardware path.

#### Scenario: GPS hosted path can be reused without implying hardware bring-up
- **WHEN** a later change wants to reuse the first GPS validation baseline
- **THEN** reviewers SHALL be able to cite one registry entry that names the hosted GPS fake/replay path, its governing evidence, and the still-out-of-scope Raspberry Pi UART hardware path

### Requirement: Storage Health Hosted Validation Path Is Registered
The repository verification-path registry SHALL include a dedicated entry for the first hosted storage health validation path and SHALL describe that path as distinct from any still-unimplemented Raspberry Pi target disk-health or cleanup-policy path.

#### Scenario: Hosted storage path can be reused without implying cleanup or target coverage
- **WHEN** a later change wants to reuse the first storage health validation baseline
- **THEN** reviewers SHALL be able to cite one registry entry that names the hosted governed-root storage path, its governing evidence, and the still-out-of-scope target-disk or cleanup-policy work

### Requirement: Hosted EPS Internal libcsp Path Is Registered
The repository verification-path registry SHALL include a dedicated entry for the hosted EPS internal libcsp service path once EPS status, PDU, heater/config, and reset behavior have passed through EPS node `2` on the governed hosted CSP substrate.

#### Scenario: EPS CSP path can be reused without implying ADCS or ground coverage
- **WHEN** a later change wants to reuse hosted EPS business traffic over libcsp
- **THEN** reviewers SHALL be able to cite one registry entry that names the EPS internal libcsp path, its governing evidence, and the still-out-of-scope ADCS, ground, external comm, GPS, and hardware EPS paths

### Requirement: Hosted ADCS Internal libcsp Path Is Registered
The repository verification-path registry SHALL include a dedicated entry for the hosted ADCS internal libcsp service path once ADCS state, mode, target, and calibration behavior have passed through ADCS node `3` on the governed hosted CSP substrate.

#### Scenario: ADCS CSP path can be reused without implying adjacent paths
- **WHEN** a later change wants to reuse hosted ADCS business traffic over libcsp
- **THEN** reviewers SHALL be able to cite one registry entry that names the ADCS internal libcsp path, its governing evidence, and the still-out-of-scope ground, external comm, GPS, and hardware ADCS paths

### Requirement: Repository Evidence Governs Validation Path Reuse
The verification-path registry SHALL keep distinct entries for adjacent ADCS hosted protocol and recovery paths, and those entries SHALL describe the active ADCS CSP service set and bounded shared recovery behavior accurately enough for future reuse decisions.

#### Scenario: ADCS hosted internal CSP path includes the reset service
- **WHEN** reviewers inspect the active hosted ADCS internal libcsp path entry
- **THEN** the registry SHALL describe ADCS-owned application services on ports `20` through `24`
- **AND** it SHALL include the ADCS reset request/reply path in the proven scope only after repository evidence is refreshed for that service

#### Scenario: Shared recovery entry distinguishes ADCS R3 reset from old R2 restart
- **WHEN** reviewers inspect the active bounded shared recovery path for ADCS
- **THEN** the registry SHALL state that first-fault ADCS scheduled-poll recovery uses `R3_RESET_SUBSYSTEM_INTERFACE` and the ADCS CSP reset service rather than first-fault R2 process restart
- **AND** it SHALL keep higher-level relatch escalation and unrelated target service-management proofs as separate adjacent claims

### Requirement: Legacy Direct-ZMQ Retirement Is Registered
The repository verification-path registry SHALL include a dedicated cleanup/guardrail entry once the active EPS and ADCS direct ZMQ request/reply business paths have been removed and guarded by a repo-local regression checker.

#### Scenario: Future changes cannot silently restore retired direct-ZMQ paths
- **WHEN** a later change modifies EPS or ADCS hosted transport code
- **THEN** reviewers SHALL be able to cite the registry entry and checker evidence showing that direct ZMQ request/reply business traffic is retired while libcsp ZMQHUB remains the allowed hosted internal CSP substrate

### Requirement: Raspberry Pi CSP Plus Comm Baseline Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the Raspberry Pi CSP plus external comm baseline path once the target profile proves internal CSP reachability and `/dev/serial0` comm exchange in one governed run.

#### Scenario: Later target work can cite the combined baseline
- **WHEN** a later target-side change needs to rely on both Raspberry Pi CSP bring-up and comm UART availability
- **THEN** reviewers SHALL be able to cite one registry entry that names the combined target baseline and its still-out-of-scope neighboring paths

### Requirement: Live OBC GPS UART Path Is Registered Separately
The verification-path registry SHALL include a distinct entry for the governed `GY-GPS6MV2 -> obc.local:/dev/serial0 -> GpsBridge` live UART path.

#### Scenario: GPS live UART registry entry stays separate from direct GDS and historical comm paths
- **WHEN** a later change needs to cite target-side live GPS behavior
- **THEN** the registry SHALL direct it to the governed GPS live UART entry rather than to direct `GDS -> TCP -> OBC` records or the older OBC-side serial comm entries

### Requirement: Historical OBC-Side Comm UART Entries Remain Historical
After `obc.local:/dev/serial0` is reassigned to GPS, the verification-path registry SHALL preserve the old OBC-side serial comm entries as historical paths and SHALL describe active comm development as proceeding through other governed paths until a later hardware migration change re-establishes a new target baseline.

#### Scenario: Reviewers can distinguish current GPS ownership from historical comm ownership
- **WHEN** reviewers compare current GPS target validation against older OBC-side serial comm validation
- **THEN** the registry SHALL make clear which path is current and which is historical

### Requirement: Remote Pi-To-macOS Internal CSP Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the remote topology where the Raspberry Pi target OBC uses a macOS-hosted CSP hub plus remote EPS node `2` and ADCS node `3`.

#### Scenario: Later changes can cite the remote internal CSP topology
- **WHEN** a later change needs to reuse the governed `Pi OBC -> remote macOS simulators` baseline
- **THEN** reviewers SHALL be able to cite one registry entry that names the remote internal CSP path, its governing evidence, and the still-out-of-scope neighboring paths

### Requirement: Remote Target-Side GDS Subsystem Command Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the target-side `fprime-cli -> GDS -> Pi OBC -> remote EPS/ADCS simulator` subsystem command path once the governed probe proves one EPS and one ADCS command through that route.

#### Scenario: Ground-driven subsystem commands stay distinct from adjacent paths
- **WHEN** a reviewer checks whether the remote target-side subsystem command path is already proven
- **THEN** the registry SHALL show that this path covers only the bounded EPS and ADCS command flow through GDS and SHALL keep direct adapter connectivity, external comm, GPS, and future physical-bus behavior distinct

### Requirement: Three-Host Split-Host Internal CSP Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the three-host topology where `obc.local` reaches remote EPS node `2` and ADCS node `3` on `subsystem.local` through a macOS-hosted CSP hub.

#### Scenario: Reviewers can distinguish two-host and three-host remote CSP paths
- **WHEN** a later change needs to reuse the split-host subsystem baseline
- **THEN** reviewers SHALL be able to cite a registry entry that identifies `subsystem.local` as a separate simulator host instead of reusing the older two-host `Pi OBC -> remote macOS simulators` path

### Requirement: Three-Host Target-Side GDS Subsystem Command Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the bounded `macOS fprime-cli -> GDS -> obc.local -> subsystem.local` subsystem command path once the governed probe proves one EPS and one ADCS command through that route.

#### Scenario: Three-host ground-driven subsystem commands stay distinct
- **WHEN** a reviewer checks whether the split-host subsystem command path is already proven
- **THEN** the registry SHALL show that this path covers only the bounded EPS and ADCS command flow through GDS and the split-host CSP topology, while keeping direct adapter connectivity, external comm, GPS, and future physical-bus behavior distinct

### Requirement: Three-Host CSP Plus External Comm Coexistence Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the three-host validation path where `obc.local` runs `/dev/serial0` external comm while `subsystem.local` still provides remote EPS and ADCS simulator nodes through the macOS-hosted CSP hub.

#### Scenario: Coexistence path can be cited without over-claiming adjacent behavior
- **WHEN** a later target-side change needs to rely on split-host subsystem reachability plus baseline external comm coexistence
- **THEN** reviewers SHALL be able to cite one registry entry that names that coexistence path and its still-out-of-scope neighboring paths

### Requirement: Future Omitted-RF TT&C Path Is Registered Separately
When the repository proves a gateway-backed omitted-RF TT&C path, the verification-path registry SHALL record that path separately from both the direct `GDS -> TCP -> OBC` development path and the existing external comm mock or UART baselines.

#### Scenario: Registry keeps gateway-backed TT&C distinct from direct GDS
- **WHEN** the first gateway-backed omitted-RF TT&C path becomes formally proven
- **THEN** the registry SHALL identify it as a distinct path and SHALL keep direct GDS and current external-comm baseline entries separate

#### Scenario: Registry identifies reused baseline and new proof boundary together
- **WHEN** the first gateway-backed COMM TT&C path is registered
- **THEN** the registry SHALL name the gateway-backed `GDS -> gateway -> serial ingress -> COMM -> internal CSP -> OBC` path as the newly proven path
- **AND** it SHALL say separately whether direct `GDS -> TCP -> OBC` connectivity is only reused as a neighboring baseline or also revalidated in the same evidence

#### Scenario: Registry keeps historical or controller-oriented comm baselines separate
- **WHEN** the first gateway-backed COMM TT&C path is registered
- **THEN** the registry SHALL keep the controller-oriented mock, transparent, framed, and historical UART comm entries separate instead of treating them as interchangeable proof of the COMM CSP path

### Requirement: Future Shared CAN FD Internal CSP Path Is Registered Separately
When the repository proves a spacecraft-side shared `CAN FD` carrier for future CSP-facing subsystems, the verification-path registry SHALL record that path separately from hosted `ZMQHUB` CSP baselines and from omitted-RF ground ingress paths.

#### Scenario: Registry keeps shared CAN FD proof separate from hosted ZMQHUB proof
- **WHEN** the first shared `CAN FD` internal CSP path becomes formally proven
- **THEN** the registry SHALL identify that path as distinct from the hosted or split-host `ZMQHUB/TCP/IP` CSP baselines

#### Scenario: First physical internal CSP proof records logical-node limits explicitly
- **WHEN** the first governed CAN FD-capable SocketCAN path is registered
- **THEN** the registry SHALL identify `EPS` and `ADCS` as separate logical CSP nodes while also stating that the proof does not imply independent physical EPS and ADCS controllers

#### Scenario: First physical internal CSP proof records reserved-channel isolation separately
- **WHEN** the first governed CAN FD-capable SocketCAN path is registered
- **THEN** the registry SHALL record the active shared-bus path separately from the reserved subsystem CAN channel evidence and SHALL not treat the reserved channel as an active COMM path

### Requirement: Future Gateway And Shared-Bus Paths Stay Distinct From Existing Live GPS UART Proof
When the repository later proves gateway-backed TT&C or shared `CAN FD` CSP paths, the verification-path registry SHALL keep those entries distinct from the already-governed live `GPS -> OBC` UART path.

#### Scenario: Registry does not collapse GPS proof into later TT&C or CAN FD proof
- **WHEN** a reviewer checks future gateway-backed TT&C or shared `CAN FD` entries
- **THEN** the registry SHALL continue to identify the governed live GPS UART entry as a separate direct sensor path

### Requirement: Subsystem COMM UART Preflight Path Is Registered
The verification-path registry SHALL include a distinct entry for the macOS-initiated physical COMM UART preflight into `subsystem.local` once the governed probe proves bounded serial request/reply traffic over that wiring.

#### Scenario: Subsystem UART preflight is separate from historical OBC UART paths
- **WHEN** reviewers check whether the new subsystem-side COMM serial wiring has been proven
- **THEN** the registry SHALL point to the subsystem COMM UART preflight evidence
- **AND** it SHALL keep the older OBC-side `/dev/serial0` external comm evidence historical rather than treating it as proof of the new subsystem-side wiring

#### Scenario: Registry does not overclaim reverse-direction acquisition
- **WHEN** reviewers check whether `subsystem.local` can initiate clean cold-first downlink into a passive macOS receiver
- **THEN** the registry SHALL treat that behavior as unproven by the subsystem COMM UART preflight unless later evidence proves it separately

#### Scenario: Subsystem UART preflight is separate from gateway-backed TT&C
- **WHEN** a later change uses the physical serial link for gateway-backed TT&C ingress
- **THEN** the registry SHALL require that later change to prove the TT&C path separately instead of reusing the UART preflight as a complete ground-link proof

### Requirement: Lab Serial Acquisition Path Is Registered Separately
The verification-path registry SHALL register subsystem-origin lab serial acquisition separately from both the macOS-initiated subsystem UART preflight and any gateway-backed TT&C path.

#### Scenario: Registry separates acquisition from TT&C
- **WHEN** the lab serial acquisition probe passes
- **THEN** the registry SHALL identify the path as passive macOS acquisition of bounded frames transmitted by `subsystem.local`
- **AND** it SHALL state that the path does not prove full TT&C, stock F' event/telemetry downlink, RF, file/downlink, target OBC, or COMM shared CAN FD

#### Scenario: Registry remains unchanged if acquisition fails
- **WHEN** the lab serial acquisition probe fails
- **THEN** the registry SHALL NOT add the acquisition path as a proven validation path

### Requirement: Physical Lab Serial Ingress Path Is Registered Separately
The verification-path registry SHALL register the physical lab serial ingress result separately from hosted PTY gateway TT&C, subsystem UART preflight, and subsystem-origin acquisition paths.

#### Scenario: Registry names the proven physical ingress boundary
- **WHEN** the lab serial ingress probe passes Stage 1
- **THEN** the registry SHALL identify the newly proven path as `fprime-cli -> GDS -> ground_ttc_gateway -> physical serial -> subsystem.local comm_csp_node -> CSP -> hosted OBC`
- **AND** it SHALL state whether the registered verdict is uplink ingress only or full bounded TT&C

#### Scenario: Registry keeps adjacent physical serial paths distinct
- **WHEN** reviewers inspect the physical lab serial ingress entry
- **THEN** the registry SHALL keep macOS-initiated UART preflight and subsystem-origin acquisition as separate supporting paths rather than treating them as the same proof boundary

#### Scenario: Failed downlink does not become a registered full TT&C path
- **WHEN** Stage 2 does not prove events and telemetry
- **THEN** the registry SHALL NOT register full physical lab serial TT&C
- **AND** it MAY register only the Stage 1 uplink ingress path if Stage 1 passed

### Requirement: Physical Lab Serial Bounded TT&C Path Is Registered Separately
The verification-path registry SHALL register bounded physical lab serial TT&C only after evidence proves both bounded physical serial command ingress and bounded event/telemetry downlink over the COMM path.

#### Scenario: Registry names the bounded physical TT&C boundary
- **WHEN** the physical lab serial downlink probe passes
- **THEN** the registry SHALL identify the newly proven path as `fprime-cli -> GDS -> ground_ttc_gateway -> physical serial -> subsystem.local comm_csp_node -> CSP -> hosted OBC -> COMM downlink -> ground_ttc_gateway -> GDS -> fprime-cli`
- **AND** it SHALL state that the proven scope is bounded command readback plus command-event and telemetry visibility

#### Scenario: Registry keeps uplink ingress and full TT&C distinct
- **WHEN** reviewers inspect the physical lab serial TT&C registry entry
- **THEN** the registry SHALL keep the prior physical lab serial uplink ingress entry as a narrower adjacent path
- **AND** it SHALL not treat hosted PTY TT&C, UART preflight, or subsystem-origin acquisition as the same proof boundary

#### Scenario: Registry excludes future COMM work
- **WHEN** the bounded physical lab serial TT&C path is registered
- **THEN** the registry SHALL explicitly state that file/downlink, RF, target OBC migration, no-preamble first-byte-clean behavior, and COMM shared CAN FD participation remain unproven by that evidence

### Requirement: Historical Physical Lab Serial COMM HK File Downlink Path Is Registered Separately
The verification-path registry SHALL preserve physical lab serial COMM HK
file/downlink as a historical, retired-fallback path only after evidence proves
that stock F' file downlink traversed the gateway-backed physical COMM path and
produced byte-matching files in the ground storage directory.

#### Scenario: Registry names the file/downlink boundary
- **WHEN** the physical lab serial COMM file/downlink probe passes
- **THEN** the registry SHALL identify the historical path as `HK_DOWNLINK_* -> FileDownlink -> COMM downlink -> ground_ttc_gateway -> GDS file storage`
- **AND** it SHALL state that the proven scope was housekeeping archive index plus at least two slot files over the existing COMM TT&C path
- **AND** it SHALL state that this path is not the current official `.fdp` mission-history baseline

#### Scenario: Registry keeps file downlink distinct from command/event/channel TT&C
- **WHEN** reviewers inspect the physical lab serial COMM file/downlink registry entry
- **THEN** the registry SHALL keep the bounded command/event/channel TT&C entry as a prerequisite and adjacent path
- **AND** it SHALL NOT treat command/event/channel visibility alone as proof of file/downlink behavior

#### Scenario: Registry excludes future COMM work
- **WHEN** the COMM file/downlink path is registered
- **THEN** the registry SHALL explicitly state that arbitrary file downlink, RF, target OBC migration, no-preamble first-byte-clean behavior, archive wraparound, ScenarioBridge integration, and COMM shared CAN FD participation remain unproven by that evidence

### Requirement: Physical COMM SocketCAN TT&C Path Is Registered Separately
The verification-path registry SHALL register COMM SocketCAN command/event/channel TT&C as a distinct path only after evidence proves gateway-backed TT&C through COMM node `4` over `subsystem.local:can1` to target OBC on `obc.local:can0`.

#### Scenario: Registry names the COMM SocketCAN boundary
- **WHEN** the COMM SocketCAN TT&C probe passes
- **THEN** the registry SHALL identify the newly proven path as `GDS -> ground_ttc_gateway -> lab serial ingress -> COMM node 4 on subsystem.local:can1 -> shared SocketCAN bus -> obc.local:can0 -> OBC -> COMM downlink -> GDS`
- **AND** it SHALL state that EPS/ADCS remain on `subsystem.local:can0`

#### Scenario: Registry keeps prior paths distinct
- **WHEN** reviewers inspect the COMM SocketCAN TT&C entry
- **THEN** the registry SHALL keep EPS/ADCS SocketCAN foundation, physical lab serial COMM TT&C, and COMM file/downlink as separate adjacent paths
- **AND** it SHALL not treat any of those entries alone as proof of COMM SocketCAN TT&C

#### Scenario: Registry excludes future COMM work
- **WHEN** the COMM SocketCAN TT&C path is registered
- **THEN** the registry SHALL explicitly state that file/downlink, RF, no-preamble first-byte-clean behavior, dual-bus redundancy, and independent COMM hardware remain unproven by that evidence

### Requirement: Historical Physical COMM SocketCAN HK File Downlink Path Is Registered Separately
The verification-path registry SHALL preserve COMM SocketCAN HK file/downlink as
a historical, retired-fallback path only after evidence proves that stock F'
file downlink traversed COMM node `4` over `subsystem.local:can1` to target OBC
on `obc.local:can0` and produced byte-matching files in the GDS file-storage
directory.

#### Scenario: Registry names the COMM SocketCAN file boundary
- **WHEN** the COMM SocketCAN file/downlink probe passes
- **THEN** the registry SHALL identify the historical path as `HK_DOWNLINK_* -> FileDownlink -> COMM downlink over shared SocketCAN -> ground_ttc_gateway -> GDS file storage`
- **AND** it SHALL state that the proven scope was housekeeping archive index plus at least two slot files over the existing SocketCAN-backed COMM TT&C path
- **AND** it SHALL state that this path is not the current official `.fdp` mission-history baseline

#### Scenario: Registry keeps adjacent paths distinct
- **WHEN** reviewers inspect the COMM SocketCAN file/downlink entry
- **THEN** the registry SHALL keep physical lab serial COMM file/downlink and physical COMM SocketCAN command/event/channel TT&C as separate adjacent paths
- **AND** it SHALL NOT treat either adjacent path alone as proof of COMM SocketCAN file/downlink

#### Scenario: Registry excludes future COMM work
- **WHEN** the COMM SocketCAN file/downlink path is registered
- **THEN** the registry SHALL explicitly state that arbitrary onboard file path downlink, RF, no-preamble first-byte-clean behavior, archive wraparound, ScenarioBridge integration, dual-bus redundancy, independent COMM hardware, and end-to-end missing-packet retransmission remain unproven by that evidence

### Requirement: Lab Target COMM CSP Operational Path Is Registered Separately
The verification-path registry SHALL register the lab target COMM CSP operational path separately from the earlier probe-owned physical COMM SocketCAN command/event/channel and file/downlink paths.

#### Scenario: Registry names the service-managed lab path
- **WHEN** the lab target operational evidence passes
- **THEN** the registry SHALL identify the newly reusable path as service-managed `obc.local` installed OBC plus service-managed `subsystem.local` EPS/ADCS/COMM, macOS `fprime-gds` plus `ground_ttc_gateway`, physical lab serial ingress, and shared SocketCAN COMM path

#### Scenario: Registry distinguishes probe evidence from operational baseline
- **WHEN** reviewers inspect the registry entry
- **THEN** it SHALL cite the earlier SocketCAN TT&C and SocketCAN file/downlink entries as supporting adjacent evidence
- **AND** it SHALL state that the new entry proves the service-managed lab operational path rather than introducing a new COMM wire contract

#### Scenario: Registry excludes flight and future communications claims
- **WHEN** the lab target operational path is registered
- **THEN** it SHALL explicitly state that final flight deployment, RF behavior, no-preamble first-byte-clean behavior, reliable retransmission, arbitrary onboard file downlink, ScenarioBridge/pass automation, and final OS CAN provisioning remain unproven

### Requirement: Hosted Dual-Link COMM Simulator Identity Path Is Registered
The verification-path registry SHALL include a distinct entry for the hosted dual-link COMM simulator identity and coexistence path once generic COMM node `4`, S-band COMM node `5`, and UHF COMM node `6` have been proven as separate hosted simulator process identities.

#### Scenario: Registry identifies newly proven foundation boundary
- **WHEN** the hosted dual-link COMM simulator foundation path is registered
- **THEN** the registry SHALL identify the newly proven path as hosted OBC node `1` reaching generic `comm_csp_node` node `4`, `sband_comm_csp_node` node `5`, and `uhf_comm_csp_node` node `6` over the governed hosted internal CSP substrate
- **AND** it SHALL cite the dual-link foundation evidence record

#### Scenario: Registry keeps older node-4 evidence compatible
- **WHEN** reviewers inspect existing generic COMM node `4` evidence after the dual-link foundation change
- **THEN** the registry SHALL keep that earlier evidence scoped to generic compatibility COMM rather than reclassifying it as S-band or UHF evidence

#### Scenario: Registry excludes future full-link proofs
- **WHEN** reviewers inspect the dual-link foundation registry entry
- **THEN** the entry SHALL state that it does not prove complete S-band GDS path, UHF UART backup path, CCSDS behavior, RF behavior, reliable transfer, file/downlink behavior, target hardware behavior, or Pi hardware deployment

### Requirement: Hosted S-band TCP TT&C Path Is Registered
The verification-path registry SHALL include a distinct entry for the hosted S-band TCP gateway-backed COMM command/event/channel TT&C path once it is proven through `sband_comm_csp_node` node `5`.

#### Scenario: Registry identifies S-band TCP TT&C boundary
- **WHEN** the hosted S-band TCP TT&C path is registered
- **THEN** the registry SHALL identify the newly proven path as bounded command/event/channel traffic through `fprime-cli -> fprime-gds -> ground_ttc_gateway -> S-band TCP -> sband_comm_csp_node(node 5) -> internal CSP -> hosted OBC`
- **AND** it SHALL cite the S-band TCP ground-link evidence record

#### Scenario: Registry keeps adjacent paths distinct
- **WHEN** reviewers inspect the hosted S-band TCP TT&C registry entry
- **THEN** the entry SHALL state that it does not prove direct `GDS -> TCP -> OBC`, UHF UART backup, CCSDS behavior, RF behavior, reliable transfer, file/downlink behavior, target hardware, Raspberry Pi deployment, or arbitrary onboard file downlink

### Requirement: Hosted S-band TCP File Downlink Path Is Registered
The verification-path registry SHALL include a distinct entry for bounded official `.fdp` file/downlink over the hosted S-band TCP gateway-backed COMM path once received files byte-match OBC runtime source snapshots.

#### Scenario: Registry identifies S-band TCP file/downlink boundary
- **WHEN** the hosted S-band TCP file/downlink path is registered
- **THEN** the registry SHALL identify the newly proven path as `DpCatalog -> FileDownlink -> S-band TCP COMM downlink -> ground_ttc_gateway -> GDS file storage`
- **AND** it SHALL require byte-matched official `.fdp` files before the file/downlink entry is cited as evidence

#### Scenario: Registry keeps S-band file scope bounded
- **WHEN** reviewers inspect the hosted S-band TCP file/downlink registry entry
- **THEN** the entry SHALL cite the S-band TCP TT&C path as a prerequisite
- **AND** it SHALL state that the entry does not prove arbitrary onboard file path downlink, reliable transfer, packet-loss recovery, RF behavior, target hardware, Raspberry Pi deployment, CCSDS, or UHF backup behavior

### Requirement: Hosted UHF Serial Backup TT&C Path Is Registered
The verification-path registry SHALL include a distinct entry for the hosted UHF serial gateway-backed COMM command/event/channel ingress path once it is proven through `uhf_comm_csp_node` node `6`.

#### Scenario: Registry identifies UHF backup ingress boundary
- **WHEN** the hosted UHF serial backup TT&C path is registered
- **THEN** the registry SHALL identify the newly proven path as bounded command/event/channel traffic through `fprime-cli -> fprime-gds -> ground_ttc_gateway(link=uhf serial) -> hosted serial -> uhf_comm_csp_node(node 6) -> internal CSP -> hosted OBC`
- **AND** it SHALL cite the UHF UART backup evidence record

#### Scenario: Registry keeps adjacent paths distinct
- **WHEN** reviewers inspect the hosted UHF serial backup TT&C registry entry
- **THEN** the entry SHALL state that it does not prove S-band TCP, direct `GDS -> TCP -> OBC`, generic node `4` COMM, full UHF command authority, failover policy, file/downlink behavior, CCSDS behavior, RF behavior, reliable transfer, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation

### Requirement: Active UHF CCSDS Node-6 Path Is Registered
The verification-path registry SHALL include a distinct entry for the active hosted UHF CCSDS node `6` path once bounded command/event/telemetry/file behavior and decoded framing are proven on the default hosted `OBC`.

#### Scenario: Registry identifies active UHF CCSDS boundary
- **WHEN** the active hosted UHF CCSDS path is registered
- **THEN** the registry SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(link=uhf serial) -> hosted serial -> uhf_comm_csp_node(node 6) -> internal CSP -> hosted OBC`
- **AND** it SHALL record `SCID 0x44`, `VCID 2`, and TM frame size `1024`
- **AND** it SHALL cite the active UHF CCSDS adoption evidence record rather than the historical UHF UART backup record

#### Scenario: Registry keeps active and historical UHF paths distinct
- **WHEN** reviewers inspect the active hosted UHF CCSDS registry entry
- **THEN** the entry SHALL state that it does not prove RF behavior, reliable transfer, target hardware, Raspberry Pi deployment, broader authority expansion, or legacy retirement
- **AND** it SHALL remain distinct from the historical UHF `ComFprime` backup entry and the UHF BeaconV1 side-channel entry

### Requirement: Hosted UHF Node-6 Beacon Path Is Registered
The verification-path registry SHALL include a distinct entry for bounded BeaconV1 capture over the hosted UHF node-6 beacon side channel once the captured frame is decoded and recorded as formal evidence.

#### Scenario: Registry identifies UHF beacon boundary
- **WHEN** the hosted UHF beacon path is registered
- **THEN** the registry SHALL identify the newly proven path as `OBC BeaconPublisher -> UHF node-6 beacon side channel -> uhf_comm_csp_node(node 6) -> hosted beacon serial -> capture/decode`
- **AND** it SHALL require a captured BeaconV1 binary artifact and decoded JSON artifact before the entry is cited as evidence

#### Scenario: Registry keeps UHF beacon scope bounded
- **WHEN** reviewers inspect the hosted UHF beacon registry entry
- **THEN** the entry SHALL state that it does not prove command response behavior, file/downlink, full UHF command authority, failover policy, reliable transfer, CCSDS behavior, RF behavior, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation

### Requirement: Default CCSDS S-band Path Registration Requires Passing Hosted Adoption Proof
The verification path registry SHALL register a reusable default hosted CCSDS S-band ground-link path only after the hosted adoption proof passes.

#### Scenario: Passing adoption proof registers default CCSDS path
- **WHEN** the default CCSDS hosted S-band node `5` adoption proof passes command, event, telemetry, bounded file/downlink, gateway raw-byte compatibility, and decoded frame/APID observation checks
- **THEN** the registry SHALL add a default hosted CCSDS S-band ground-link path that references the `ccsds-sband-hosted-adoption-v1` evidence record
- **AND** the path SHALL name `space-packet-space-data-link`, SCID `0x44`, VCID `1`, TM frame size `1024`, S-band node `5`, and the default hosted `OBC` executable

#### Scenario: Failed adoption proof does not register reusable path
- **WHEN** any CCSDS adoption proof area fails or remains inconclusive
- **THEN** the registry SHALL NOT add a reusable default CCSDS hosted ground-link path
- **AND** the evidence SHALL record blockers instead of claiming adoption

#### Scenario: Existing paths remain distinct
- **WHEN** the registry is updated for this change
- **THEN** existing stock `ComFprime` S-band and UHF paths SHALL remain separately named
- **AND** historical `ccsds-ground-link-spike-v1` evidence SHALL remain distinct from default hosted adoption evidence
- **AND** the registry SHALL NOT merge direct TCP, S-band-through-COMM, UHF UART backup, spike CCSDS, and default CCSDS hosted paths into a single validation path

### Requirement: Hosted Legacy Command Ingress Authority Compatibility Path Is Registered

The verification-path registry SHALL keep hosted legacy command-ingress
authority families registered only as historical compatibility evidence and
SHALL NOT present them as current maintained closeout gates once secure-auth-
only retirement is complete.

#### Scenario: Registry identifies historical envelope sequence-enforcement boundary
- **WHEN** reviewers inspect hosted legacy command-ingress authority entries
- **THEN** the registry SHALL state that those entries prove historical legacy
  envelope metadata or sequence behavior only
- **AND** it SHALL state that those entries are not current maintained
  secure-baseline authority.

### Requirement: Hosted Legacy Command Ingress Authority Path Includes Session Lifecycle Boundary

The verification-path registry SHALL preserve hosted legacy session-lifecycle
records only as archived historical compatibility evidence after public
`SESSION_OPEN` retirement.

#### Scenario: Registry identifies historical lifecycle proof boundary
- **WHEN** reviewers inspect hosted legacy lifecycle records after this change
- **THEN** the registry SHALL state that the path proved explicit
  `SESSION_OPEN` behavior, source-epoch replacement, and reopened-session
  history on the old path
- **AND** it SHALL identify that boundary as archived historical compatibility
  evidence rather than current maintained authority.

#### Scenario: Registry keeps lifecycle path out of maintained gate inventory
- **WHEN** the current maintained secure baseline gate set is described
- **THEN** the registry SHALL exclude legacy hosted command-ingress lifecycle
  wrappers from that maintained closeout-gate set.

### Requirement: Hosted Command Ingress Authority Path Includes Authenticated Envelope Boundary

The verification-path registry SHALL preserve authenticated envelope v1 proof
families as historical compatibility citations only once the current runtime no
longer accepts legacy command-envelope v1 traffic.

#### Scenario: Registry identifies authenticated legacy scope as historical
- **WHEN** reviewers inspect the hosted authenticated legacy command-ingress
  family after retirement
- **THEN** the registry SHALL state that the path proved the old authenticated
  envelope ordering on the legacy path
- **AND** it SHALL NOT describe that path as a current maintained operator or
  closeout gate.

### Requirement: Boot Trust Chain Validation Path Is Registered

The verification-path registry SHALL include a dedicated entry for any boot trust-chain validation path proven by `boot-trust-chain-v1`, with hosted and Raspberry Pi proof boundaries kept distinct.

#### Scenario: Hosted boot trust path can be cited without target overclaiming
- **WHEN** the hosted boot trust-chain probe passes
- **THEN** the registry SHALL identify the path as hosted `BootManager` staged-image manifest verification and activation lifecycle evidence
- **AND** it SHALL state that the path does not prove Raspberry Pi target reboot, bootloader handoff, hardware secure boot, physical SD-card switching, or power-loss behavior.

#### Scenario: Raspberry Pi boot trust path is registered only when proven
- **WHEN** a Raspberry Pi boot trust-chain probe passes on target hardware
- **THEN** the registry SHALL identify the target path separately from hosted evidence
- **AND** it SHALL state exactly whether the probe covered target process restart, installed/autostart runtime roots, or only the interactive target boot-update path.

#### Scenario: Failed or unrun target probe is not registered as proven
- **WHEN** Raspberry Pi target validation is not executed or does not pass
- **THEN** the registry SHALL NOT register a target boot trust-chain path as formally proven
- **AND** the evidence SHALL keep the remaining target scope explicit as deferred or constrained.

### Requirement: Registry Tracks COMM Session-And-Downlink Policy Paths

The verification-path registry SHALL add a hosted COMM session-and-downlink QoS path that identifies the command-policy and shared downlink behavior proven on the active baseline without collapsing distinct S-band and UHF boundaries into a single generic path.

#### Scenario: Command-policy path is registered

- **WHEN** hosted evidence proves COMM-driven authenticated policy across S-band node `5` and UHF node `6`
- **THEN** the registry SHALL describe the bounded command-policy path, the affected roles, and the explicit scope limits that remain outside the proof

#### Scenario: UHF file/downlink path is registered separately

- **WHEN** hosted evidence proves bounded UHF node-`6` file/downlink after primary switch
- **THEN** the registry SHALL add a distinct UHF file/downlink entry rather than extending the existing S-band file/downlink entry by implication

### Requirement: Registry Tracks Hosted Watchdog-v1 Path

The verification-path registry SHALL add a distinct hosted watchdog-v1 path that identifies active-baseline software-watchdog supervision without collapsing it into EPS timeout FDIR or target watchdog reset proof.

#### Scenario: Hosted watchdog path is registered

- **WHEN** hosted evidence proves bounded watchdog supervision over the active `TopCcsds` baseline
- **THEN** the registry SHALL describe the supervised source set, heartbeat freshness boundary, escalation behavior, and explicit scope limits that remain outside the proof

#### Scenario: Feed-suppression proof stays separate from target reset proof

- **WHEN** hosted evidence proves supervisor-side watchdog feed suppression
- **THEN** the registry SHALL identify that proof as feed-eligibility or feed-suppression behavior only
- **AND** it SHALL keep Raspberry Pi hardware watchdog reset, boot recovery, and reset-cause persistence as separate unproven or future paths unless later evidence proves them

### Requirement: Historical Hosted Multi-Subsystem Shared Recovery Closure Remains Registered Separately
The verification-path registry SHALL preserve the historical hosted bounded `EPS + ADCS + COMM` shared recovery closure as a distinct archived proof entry separate from the earlier watchdog-only and EPS-only recovery evidence.

#### Scenario: Registry preserves the archived hosted shared recovery closure
- **WHEN** reviewers inspect the historical `multi-subsystem-fdir-v1` entry
- **THEN** the registry SHALL identify that archived proof as `detector-local EPS/ADCS/COMM fault injection -> TopCcsds shared RecoveryExecutor -> bounded recovery action -> hosted reboot-equivalent relaunch truth`
- **AND** it SHALL cite the governing `evidence/records/multi-subsystem-fdir-v1/README.md` evidence
- **AND** it SHALL not imply that the original `run_multi_subsystem_fdir_v1_probe.sh` wrapper remains a maintained rerunnable current proof surface on later baselines

#### Scenario: Registry keeps adjacent recovery paths distinct
- **WHEN** reviewers inspect the hosted multi-subsystem recovery entry
- **THEN** the registry SHALL keep earlier watchdog-v1, EPS-timeout-v1, and recovery-executors-v1 paths as separate adjacent proofs rather than treating any one of them as complete proof of the new three-subsystem closure

#### Scenario: Registry keeps broader FDIR claims out of scope
- **WHEN** the hosted multi-subsystem recovery closure is registered
- **THEN** the entry SHALL state that it does not prove GPS, payload, TTC, storage-health, RF, target-hardware reboot, or a generic all-subsystem FDIR platform

### Requirement: Registry Distinguishes Governed Sequence Upload And Execution Path

The verification path registry SHALL distinguish the new governed hosted sequence-upload and official-sequence-execution path from adjacent command, file-downlink, or TTC policy paths.

#### Scenario: New hosted path is registered with bounded claims

- **WHEN** `official-sequencing-system-resources-v1` evidence is recorded
- **THEN** the registry SHALL identify the path as the archived hosted CCSDS file-upload to sequence staging plus admitted official-sequence execution proof family
- **AND** it SHALL describe what it proves and what it does not prove
- **AND** it SHALL say whether the path remains a current maintained closeout gate or only supplemental historical evidence
- **AND** it SHALL keep this path distinct from command-auth/session paths, file-downlink paths, and TTC pass-window policy evidence

### Requirement: Hosted Persistent Fault Ring Relaunch Path Is Registered Separately
The verification-path registry SHALL register a distinct hosted persistent
fault ring relaunch path once repository-owned evidence proves same-runtime-root
relaunch history readback and newer-copy corruption fallback for the persistent
fault ring.

#### Scenario: Registry names the persistent ring relaunch boundary
- **WHEN** the persistent fault ring hosted probe passes
- **THEN** the registry SHALL identify the proven path as a hosted same-runtime-
  root relaunch boundary that records `RecoveryExecutor` lifecycle breadcrumbs,
  later `BootManager` boot breadcrumbs, and bounded history readback through
  `PersistentFaultManager`

#### Scenario: Registry keeps adjacent reboot and recovery paths distinct
- **WHEN** reviewers inspect the persistent fault ring relaunch entry
- **THEN** the registry SHALL keep boot metadata restart truth and the earlier
  shared recovery probe as adjacent prerequisite or neighboring paths rather
  than treating them as the same proof boundary

### Requirement: Hosted Command Ingress Authority Path Includes Persistent Freshness Boundary

The verification-path registry SHALL preserve hosted persistent-freshness
records only as historical compatibility evidence once current runtime restart
behavior is governed by fresh secure-auth re-bootstrap instead of persisted
legacy reopen-floor semantics.

#### Scenario: Registry identifies persistent freshness as historical
- **WHEN** reviewers inspect the hosted persistent-freshness entry after this
  change
- **THEN** the registry SHALL state that the path proved historical persisted
  legacy reopen-floor behavior only
- **AND** it SHALL NOT describe that entry as a current maintained gate.

### Requirement: Raspberry Pi Active OBC Freshness Persistence Path Is Registered Separately

The verification-path registry SHALL include a distinct Raspberry Pi active-path
entry for bounded command-freshness and boot-trust persistence proof on the
governed active `OBC` package path.

#### Scenario: Registry identifies active target persistence boundary
- **WHEN** a Raspberry Pi persistence probe passes for
  `persistent-command-freshness-v1`
- **THEN** the registry SHALL identify the path as active-package
  `TopCcsds` target restart or reboot persistence for boot-trust metadata plus
  command-ingress freshness state
- **AND** it SHALL state exactly whether the proof covered installed current
  release, autostart relaunch, or interactive target runtime only.

#### Scenario: Registry keeps broader target claims out of scope
- **WHEN** reviewers inspect the Raspberry Pi persistence entry after this
  change
- **THEN** the entry SHALL state that it does not prove bootloader handoff,
  hardware secure boot, power-loss robustness, full COMM lab-operational
  closure, RF behavior, or final flight deployment readiness.

### Requirement: R2 Process Restart Paths Are Registered
The repository verification-path registry SHALL include entries for the hosted
and Raspberry Pi service-managed R2 process restart paths once
`target-recovery-closure-v1` records passing evidence, and those entries SHALL
remain distinct from R6 reboot-equivalent, Linux reboot, hardware watchdog
reset, power-loss paths, and historical legacy Top evidence.

#### Scenario: Hosted R2 registry entry stays distinct from R6
- **WHEN** the hosted R2 process-restart path is registered
- **THEN** the registry SHALL identify the exact hosted runtime path, governing
  evidence, R2 exit-code behavior, and boot metadata readback
- **AND** it SHALL keep R6 reboot-equivalent closure as a neighboring but
  distinct path.

#### Scenario: Target R2 registry entry stays distinct from hardware reset
- **WHEN** the Raspberry Pi service-managed R2 process-restart path is
  registered
- **THEN** the registry SHALL identify the service-managed OBC target path, the
  exact R2 fault boundary exercised by the governing evidence, systemd restart observation, governing
  evidence, and boot metadata readback
- **AND** it SHALL state that hardware watchdog reset, Linux reboot, bootloader
  or partition handoff, power-loss recovery, and historical legacy Top behavior
  remain unproven by that entry.

### Requirement: Legacy Top Retirement Is Registered As A Cleanup Boundary
The verification-path registry SHALL identify the retired `OBC/Top` /
`OBC_ComFprimeLegacy` path as historical-only after
`legacy-top-retirement-docs-reorg-v1`, and SHALL keep active CCSDS hosted and
target paths distinct from historical ComFprime evidence.

#### Scenario: Registry distinguishes active and historical OBC ground paths
- **WHEN** reviewers inspect registry entries that mention stock `ComFprime`,
  old UHF backup, or old S-band gateway evidence
- **THEN** the registry SHALL state whether the entry is historical-only or an
  active reusable path
- **AND** the registry SHALL NOT treat historical `OBC_ComFprimeLegacy` evidence
  as proof of the maintained active `OBC` / `TopCcsds` path.

#### Scenario: Current helper paths cite active topology sources
- **WHEN** registry or verification inventory helpers describe current
  maintained source surfaces
- **THEN** they SHALL refer to active `OBC/TopCcsds` sources instead of
  `OBC/Top` sources.

### Requirement: Raspberry Pi Hardware Watchdog Reset Path Is Registered Separately

The verification-path registry SHALL add a distinct Raspberry Pi hardware
watchdog reset entry once
`target-hardware-watchdog-reset-proof-v1` records passing evidence, and that
entry SHALL remain separate from hosted watchdog supervision, target
service-managed `R2` restart, generic Linux reboot, and power-loss recovery.

#### Scenario: Hardware watchdog registry entry stays distinct from service restart
- **WHEN** the Raspberry Pi hardware watchdog reset path is registered
- **THEN** the registry SHALL identify the active `obc-comm-csp-stack.service`
  target path, governing evidence, watchdog-source trigger boundary, and reboot
  readback contract
- **AND** it SHALL keep target `R2` process restart as a neighboring but
  distinct verification path

#### Scenario: Hardware watchdog registry entry stays distinct from Linux reboot
- **WHEN** the Raspberry Pi hardware watchdog reset path is registered
- **THEN** the registry SHALL state that the path proves board reset caused by
  the hardware watchdog timeout under the active OBC baseline
- **AND** it SHALL NOT collapse that path into generic Linux reboot,
  bootloader/partition handoff, or power-loss recovery claims

### Requirement: Target Node-5 Default Path Is Registered Separately
The verification-path registry SHALL register the migrated target/lab default path as a node-`5` S-band COMM path distinct from the older target node-`4` compatibility path.

#### Scenario: Registry names the target node-5 default boundary
- **WHEN** target node-`5` default-path evidence passes
- **THEN** the registry SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> TCP -> subsystem.local sband_comm_csp_node(node 5) -> shared SocketCAN -> obc.local OBC`
- **AND** it SHALL describe the older target node-`4` path as historical or compatibility-only rather than as current target baseline

### Requirement: Target Node-6 Quiet UHF Paths Are Registered Separately
The verification-path registry SHALL register target node-`6`
`uhf-primary-after-failover` and `uhf-backup` command/readback proofs as
bounded quiet-mode paths distinct from both the default target node-`5` path
and hosted UHF evidence.

#### Scenario: Registry names the quiet node-6 boundary
- **WHEN** target node-`6` quiet-mode evidence passes
- **THEN** the registry SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> physical serial -> subsystem.local uhf_comm_csp_node(node 6) -> shared SocketCAN -> obc.local OBC`
- **AND** it SHALL record whether the bounded proof used
  `uhf-primary-after-failover` or `uhf-backup`
- **AND** it SHALL record that operator profile `uhf-primary` proves the
  `uhf-primary-after-failover` runtime role only after explicit switch from the
  default target node-`5` path

#### Scenario: Registry keeps node-6 proof scope bounded
- **WHEN** target node-`6` quiet-mode paths are registered
- **THEN** the entry SHALL state that the path does not prove general non-quiet serial/background-TM stability, simultaneous dual-link runtime, or beacon/handshake/retry policy

### Requirement: Target Reboot-Class Proofs Follow Node 5
The verification-path registry SHALL record existing target recovery-restart and hardware-watchdog proofs under the migrated default node-`5` target path rather than under a remaining node-`4` or node-`6` baseline.

#### Scenario: Registry binds recovery proofs to node 5
- **WHEN** target reboot-class evidence is updated after migration
- **THEN** the registry SHALL identify target `R2` recovery restart and target hardware-watchdog reset as proofs exercised over the default target node-`5` COMM path
- **AND** it SHALL keep quiet-mode node-`6` operational proof as a separate adjacent path

### Requirement: COMM Policy Clarifications Cite Governing Evidence Or Stay Non-Claims

The verification-path registry SHALL require current COMM policy wording to
cite governing secure-auth evidence for maintained command-session truth, while
keeping retained legacy `SESSION_OPEN(seq0)` citations historical-only and
keeping retired timing wrappers out of the maintained gate set.

#### Scenario: Current secure policy cites maintained secure-auth evidence
- **WHEN** the repository cites current command-session, uplink-authority, or
  observability-governance behavior
- **THEN** the wording SHALL point to the governing maintained secure-auth
  registry entries or archived evidence for those exact paths
- **AND** it SHALL NOT require retained legacy `SESSION_OPEN(seq0)` proof as a
  current maintained dependency.

#### Scenario: Retired timing wrappers stay out of maintained closeout authority
- **WHEN** reviewers inspect current maintained gate wording after this change
- **THEN** the registry SHALL classify
  `target-timing-empirical-ceiling-freeze-v1` and
  `target-timing-wcet-profile-proof-v1` as retired historical timing paths
- **AND** it SHALL NOT present them as current maintained closeout gates for
  unrelated product changes.

### Requirement: Hosted UHF Beacon Suppress Runtime Path Is Registered Separately

The verification-path registry SHALL register the hosted UHF beacon
suppress/runtime proof as a path distinct from the ordinary hosted UHF node-`6`
beacon side-channel capture path and from hosted UHF command-path proofs.

#### Scenario: Registry names the hosted suppress/runtime boundary
- **WHEN** hosted beacon suppress/runtime evidence passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay or serial southbound) -> uhf_comm_csp_node(node 6) -> CSP -> OBC CommController -> BeaconPublisher suppress gate -> uhf_comm_csp_node beacon side channel -> hosted beacon serial capture`
- **AND** it SHALL state that the path proves suppress start, same-session
  refresh, timeout resume, and at least one negative no-suppress case

### Requirement: Hosted UHF Primary Packet Quiet Path Is Registered Separately

The verification-path registry SHALL register the hosted UHF primary
packet-quiet proof as a path distinct from the hosted UHF beacon
suppress/runtime proof and from the broader hosted UHF command/file proofs.

#### Scenario: Registry names the hosted UHF primary packet-quiet boundary
- **WHEN** hosted UHF primary packet-quiet evidence passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay or serial southbound) -> sband_comm_csp_node(node 5) or uhf_comm_csp_node(node 6) -> CSP -> OBC CommController -> CommEgressMux packet-quiet gate -> hosted proof artifacts`
- **AND** it SHALL state that the path proves live packet suppression begins
  when UHF becomes the current primary band even before accepted qualifying UHF
  `SESSION_OPEN(seq0)` starts beacon suppress
- **AND** it SHALL state that official file/data-product downlink remains
  formal during UHF primary packet quiet

### Requirement: Target Quiet-UHF Beacon Suppress Runtime Path Is Registered Separately

The verification-path registry SHALL register the target/lab quiet node-`6`
beacon suppress/runtime proof as a path distinct from ordinary quiet-UHF
command/readback, file/downlink, and failover continuity proofs.

#### Scenario: Registry keeps quiet-UHF suppress/runtime scope bounded
- **WHEN** target quiet-UHF beacon suppress/runtime evidence passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> physical serial -> subsystem.local uhf_comm_csp_node(node 6) -> shared SocketCAN -> obc.local OBC CommController -> BeaconPublisher suppress gate -> subsystem.local beacon capture`
- **AND** it SHALL state that the proof remains quiet-path only
- **AND** it SHALL keep simultaneous dual-link runtime, UHF reliable transfer,
  RF closure, and broader handshake/runtime claims outside the registered path

### Requirement: Quiet And Non-Quiet Target Node-6 Boundaries Stay Distinct

The verification-path registry SHALL keep target/lab quiet node-`6` proof and
target/lab non-quiet node-`6` diagnosis as separate boundaries unless fresh
reviewed evidence proves a reusable non-quiet path.

#### Scenario: Registry does not widen quiet node-6 truth by adjacency
- **WHEN** reviewers inspect the existing target/lab quiet node-`6` entries
- **THEN** those entries SHALL continue to state that they do not prove general
  non-quiet serial stability under background telemetry

#### Scenario: Diagnosis evidence does not automatically become a reusable path
- **WHEN** `target-nonquiet-background-tm-stability-v1` records only an
  oracle-only, mixed, or other partial-closure result
- **THEN** the registry SHALL keep that result in the evidence tree without
  promoting it to a reusable non-quiet node-`6` validation path

#### Scenario: Registry promotion requires explicit non-quiet proof
- **WHEN** the change produces reviewed evidence that a target/lab non-quiet
  node-`6` boundary is reusable
- **THEN** the registry SHALL identify the exact proven path, oracle boundary,
  and residual exclusions explicitly
- **AND** it SHALL keep quiet-path and non-baseline side-channel evidence distinct

### Requirement: Future Target-Bearing Dual-Link Clarification Uses Citation Guidance Without Registering A New Path

The verification-path registry SHALL add citation guidance only, and SHALL NOT
register a new simultaneous target path, when the repository freezes the future
target-bearing simultaneous dual-link claim without adding fresh proof.

#### Scenario: Clarification-only boundary does not create a new registry entry
- **WHEN** the repository records a clarification-only future target-bearing
  dual-link boundary
- **THEN** the registry SHALL keep existing simultaneous target paths
  unregistered unless fresh reviewed evidence proves one
- **AND** it SHALL not present the clarification slice itself as a newly proven
  simultaneous validation path

#### Scenario: Citation guidance binds exact reused boundaries
- **WHEN** reviewers inspect the same clarification boundary
- **THEN** the registry SHALL direct them to cite:
  - entry `59` for default target node-`5` bootstrap and primary truth
  - entry `60` for quiet or explicitly switched node-`6` command/file baseline
  - entry `60A` for quiet node-`6` suppress/runtime only
  - entry `66` for target TCP comparator use only
  - entry `67` for direct target command-path adjacency only
- **AND** it SHALL state that `target-nonquiet-background-tm-stability-v1`
  remains oracle rationale only rather than a reusable simultaneous target path

#### Scenario: Hosted layers remain non-claim support only
- **WHEN** reviewers inspect adjacent hosted simultaneous entries
- **THEN** the registry SHALL keep hosted `43B` and `43C` citations limited to
  their hosted operator and orchestration boundaries
- **AND** it SHALL NOT let those hosted entries stand in for target-bearing
  command truth

### Requirement: Registry Adds A Distinct Physical Target-Bearing Dual-Link Entry Only After Fresh Proof

The verification-path registry SHALL add a distinct physical target-bearing
dual-link path only when fresh reviewed evidence closes the first
implementation-bearing proof on the target CAN + UHF UART topology.

#### Scenario: Registry entry names the exact physical proof path
- **WHEN** the official target-bearing dual-link proof is finalized
- **THEN** the registry SHALL identify the path as the physical target CAN +
  UHF UART family built from:
  - default target node-`5` primary truth
  - non-quiet node-`6` `uhf-backup` adjunct
  - explicit switched `uhf-primary-after-failover` non-quiet truth
- **AND** it SHALL keep target TCP comparator and direct-target command-path
  evidence as adjacent citations rather than part of the new path itself

#### Scenario: Registry entry is branch-scoped
- **WHEN** the official run proves only one successful outcome branch
- **THEN** the registry entry SHALL describe only that exact branch
- **AND** it SHALL name whether the governing evidence landed as
  `operator-observability=PASS` or `DEGRADED`
- **AND** it SHALL name quiet rescue only if that rescue was actually used in
  the official successful run

#### Scenario: Registry entry keeps residual non-claims explicit
- **WHEN** the new target-bearing entry is registered
- **THEN** it SHALL keep explicit non-claims for symmetric dual-authority
  commands, one-GDS heterogeneous upstream handling, one-gateway multiplexing,
  RF closure, and generic clean non-quiet operator observability beyond the
  exact branch proven

### Requirement: Hosted Maintained Per-Band Stock-Stack Operator Baseline Path Is Registered

The verification-path registry SHALL include a dedicated hosted maintained
operator-baseline entry once repository-owned evidence proves one shared hosted
runtime with distinct S-band and UHF stock `fprime-gds` plus
`ground_ttc_gateway` surfaces.

#### Scenario: Registry names the shared runtime and distinct stock surfaces
- **WHEN** the hosted maintained operator-baseline proof passes
- **THEN** the registry SHALL identify one shared hosted `TopCcsds` runtime
  together with a distinct S-band stock stack and a distinct UHF stock stack
- **AND** it SHALL state that the two surfaces remain separate operator paths
  rather than one stock GDS or one gateway multiplexer path

#### Scenario: Registry cites reused prerequisites without collapsing paths
- **WHEN** reviewers inspect the hosted maintained operator-baseline entry
- **THEN** the entry SHALL cite the governing hosted operator-baseline evidence
- **AND** it SHALL identify adjacent S-band and UHF transport or policy path
  entries as reused prerequisites rather than collapsing them into one generic
  simultaneous runtime claim

#### Scenario: Registry records bounded simulator non-interference separately from orchestration claims
- **WHEN** the same entry includes hosted cleanup-hardening follow-up evidence
- **THEN** it SHALL describe that addition as bounded launcher non-interference
  with unrelated active EPS/ADCS simulator runs on different CSP hub ports
- **AND** it SHALL NOT restate that cleanup boundary as orchestration,
  simultaneous runtime arbitration, or target-bearing dual-link proof

#### Scenario: Registry keeps orchestration and target claims deferred
- **WHEN** reviewers inspect the same entry
- **THEN** it SHALL keep explicit non-claims for one-GDS heterogeneous
  upstream handling, one-gateway multiplexer behavior, simultaneous dual-link
  runtime arbitration, target-bearing simultaneous closure, and RF behavior

### Requirement: Hosted Dual-Link Orchestration Owner Path Is Registered Separately

The verification-path registry SHALL include a dedicated hosted layer-2
orchestration-owner entry once repository-owned evidence proves a distinct
hosted lifecycle owner above the maintained per-band stock-stack baseline.

#### Scenario: Registry keeps the orchestration owner separate from `43B`
- **WHEN** the hosted orchestration-owner proof passes
- **THEN** the registry SHALL identify the new path as a hosted orchestration
  lifecycle/failure/cleanup surface above the maintained per-band stock-stack
  baseline
- **AND** it SHALL keep `43B` as the separate layer-1 path for the maintained
  shared-runtime plus distinct stock-surface baseline

#### Scenario: Registry cites orchestration-owned oracle boundaries
- **WHEN** reviewers inspect the hosted orchestration-owner registry entry
- **THEN** the entry SHALL cite the governing orchestration evidence record
- **AND** it SHALL state that the proof oracle comes from orchestration-owned
  manifest, status, failure, and cleanup surfaces plus process/listener
  inspection
- **AND** it SHALL NOT restate older COMM semantic proof surfaces as the new
  orchestration oracle

#### Scenario: Registry keeps adjacent runtime and target claims deferred
- **WHEN** reviewers inspect the same entry
- **THEN** it SHALL keep explicit non-claims for command authority ownership,
  gateway multiplexing, one-GDS heterogeneous upstream behavior, COMM runtime
  ownership, target-bearing simultaneous closure, and RF behavior

### Requirement: Reliable-transfer Node-6 Boundaries Are Registered Adjacent To Existing Node-6 Paths

The verification-path registry SHALL register bounded UHF reliable-transfer
proofs as distinct reliable-transfer boundaries adjacent to existing hosted and
target/lab node-`6` paths instead of widening those earlier paths by
implication.

#### Scenario: Hosted switched-UHF reliable transfer is registered separately
- **WHEN** hosted switched-UHF reliable-transfer evidence passes
- **THEN** the registry SHALL identify the path as one shared hosted OBC
  runtime with distinct stock S-band and UHF ground stacks, explicit switch to
  `uhf-primary-after-failover`, and node-`6` reliable-transfer receiver output
- **AND** it SHALL keep existing hosted node-`6` file/downlink and packet-quiet
  entries as separate adjacent boundaries

#### Scenario: Target quiet switched node-6 reliable transfer is registered separately
- **WHEN** target/lab quiet switched node-`6` reliable-transfer evidence passes
- **THEN** the registry SHALL identify the path as the explicit-switched quiet
  node-`6` reliable-transfer boundary
- **AND** it SHALL state that quiet mode remained probe-owned and was removed
  after the proof
- **AND** it SHALL keep target quiet command/file baseline, suppress/runtime,
  and non-quiet diagnosis entries separate from this reliable-transfer entry

#### Scenario: Registry keeps the new node-6 reliable slice bounded
- **WHEN** reviewers inspect either new node-`6` reliable-transfer entry
- **THEN** the entry SHALL keep `uhf-backup` reliable transfer, RF closure,
  restart-persistent resume, broad CFDP, one-GDS aggregation, one-gateway
  multiplexing, and generic simultaneous closure outside the registered path

### Requirement: Registry Adds Target Secure Auth Proof Path Only After Fresh Target Evidence

The verification-path registry SHALL add a target secure-auth proof entry only
after fresh target evidence exercises the installed S-band and bounded UHF
paths.

#### Scenario: Registry names target S-band and UHF ancestry separately
- **WHEN** the target secure-auth path is registered
- **THEN** the entry SHALL cite hosted entries `43E/43F` as ancestry only
- **AND** it SHALL cite target entry `59` for S-band path reuse
- **AND** it SHALL cite target entry `69` for bounded UHF physical node-`6`
  path reuse.

#### Scenario: Registry records exact target secure-auth scope
- **WHEN** the target secure-auth path is registered
- **THEN** the entry SHALL state the S-band proof cases, the bounded UHF proof
  cases, the installed-release keystore provenance, and the retained gateway
  capture plus target-journal evidence surfaces.

#### Scenario: Registry keeps adjacent non-claims explicit
- **WHEN** reviewers inspect the target secure-auth entry
- **THEN** the entry SHALL keep encryption, RF, boot-trust expansion,
  hardware-backed key storage, generic file authority, UHF primary staged-file
  success, one-GDS aggregation, one-gateway multiplexing, and legacy v1
  retirement out of scope.

### Requirement: Hosted Payload FDP Path Is Registered Separately
The verification-path registry SHALL include a distinct hosted payload `.fdp` path once the governed hosted node-`5` proof demonstrates canonical payload publication, downlink byte-match, decode, and JPEG extraction parity.

#### Scenario: Hosted payload artifact proof stays distinct from HK FDP and local payload evidence
- **WHEN** reviewers inspect whether payload end-to-end closure is formally proven on the hosted path
- **THEN** the registry SHALL identify a dedicated hosted node-`5` payload `.fdp` entry
- **AND** it SHALL keep that entry distinct from existing hosted HK `.fdp` proof paths, hosted payload local-capture evidence, and direct adapter target payload evidence

### Requirement: Target Payload FDP Path Is Registered Separately
The verification-path registry SHALL include a distinct governed target payload `.fdp` path once the governed target node-`5` proof demonstrates canonical publication, downlink byte-match, decode, and JPEG extraction parity on the target path.

#### Scenario: Target payload official closure stays distinct from direct adapter capture proof
- **WHEN** reviewers inspect whether target payload delivery closure is proven
- **THEN** the registry SHALL identify a dedicated target node-`5`
  COMM-backed payload `.fdp` entry
- **AND** it SHALL keep target official payload claims distinct from the older
  Pi-local direct `OBC -> GDS` payload capture evidence
- **AND** it SHALL state explicitly that raw-register closure, physical
  switched camera rail closure, and UHF nonquiet runtime stability remain
  outside the current target payload `.fdp` entry

### Requirement: Payload registry entries classify capture policy separately from prepared state

The verification-path registry SHALL describe payload `AUTO` and
`DETERMINISTIC` as the maintained normal capture-policy families instead of
parallel prepared-state routes.

#### Scenario: Payload registry documents single-ready truth

- **GIVEN** the maintained payload target and hosted registry entries
- **WHEN** they describe the current payload still-capture path
- **THEN** they SHALL describe a single persistent normal camera-ready state
  for still capture
- **AND** they SHALL reserve special prepared-session wording only for
  `RAW_SENSOR`
- **AND** they SHALL NOT describe `PAYLOAD_CAPTURE_STILL` as a current
  maintained capture family

### Requirement: Payload registry records persistent-session target truth distinctly from earlier warm-up closure

The verification-path registry SHALL distinguish the current persistent-session
payload target path from the earlier target black-image warm-up closure that
still used per-capture stream start/stop semantics.

#### Scenario: Registry identifies the newer persistent-session authority

- **WHEN** reviewers inspect the maintained target real-camera payload entry
- **THEN** the registry SHALL identify the current persistent-session proof as
  the maintained authority for target non-RAW session lifecycle
- **AND** it SHALL classify the older per-capture warm-up evidence as
  historical previous closure rather than the current runtime truth

### Requirement: Target Real-Camera Payload Sanity Path Is Registered Separately From Route 1 Official Delivery

The verification-path registry SHALL keep target real-camera source-image
sanity distinct from the governed target node-`5` official payload `.fdp`
delivery path.

#### Scenario: Registry distinguishes target source-image sanity from official payload delivery

- **WHEN** reviewers inspect whether target payload source-image validity is
  formally proven
- **THEN** the registry SHALL identify a distinct target real-camera capture
  sanity path for onboard source artifacts
- **AND** it SHALL keep that entry separate from the Route 1 official
  payload `.fdp` downlink closure path
- **AND** it SHALL state explicitly that the source-artifact sanity proof makes
  no ground or downlink claim

### Requirement: Hosted Node-5 Tier-Selection Proof Registers Curated Summary And Explicit Detail Readback

The verification-path registry SHALL register the hosted node-`5`
observability proof as a curated-summary path, not merely an auth-gated
access-gating path.

#### Scenario: Registry states the hosted curated-summary boundary
- **WHEN** the hosted node-`5` tier-selection proof passes
- **THEN** the registry SHALL state that the path proves post-auth curated live
  summary on node `5`
- **AND** it SHALL state that representative detailed family telemetry remains
  absent from ambient live visibility until explicit `GET_*` readback opens it
  in bounded form.

### Requirement: Target Node-5 Tier-Selection Proof Registers Curated Summary And Explicit Detail Readback

The verification-path registry SHALL register the target node-`5`
observability proof with the same curated-summary versus bounded-detail
distinction.

#### Scenario: Registry states the target curated-summary boundary
- **WHEN** the target node-`5` tier-selection proof passes
- **THEN** the registry SHALL state that the path proves post-auth curated live
  summary on the maintained target node-`5` path
- **AND** it SHALL state that representative detailed family telemetry remains
  absent from ambient live visibility until explicit `GET_*` readback opens it
  in bounded form.

#### Scenario: Registry keeps residual live chatter out of current baseline claims
- **WHEN** reviewers inspect the updated hosted or target node-`5` entry
- **THEN** the entry SHALL keep any remaining transport, queue, driver, or
  other residual runtime chatter outside the current node-`5` operator
  baseline by explicitly classifying formal reviewable proof surfaces versus
  diagnostics-only residuals
- **AND** it SHALL NOT widen the proof into generic telemetry filtering, UHF
  command-paced observability, or a new command plane claim.

### Requirement: Registry Records Residual-Observability Buckets On The Node-5 Proof Paths

The verification-path registry SHALL describe the hosted and target node-`5`
observability paths using explicit resource-truth, reviewable proof, and
diagnostics-only residual boundaries.

#### Scenario: Hosted and target node-5 registry entries keep resource truth explicit
- **WHEN** reviewers inspect the current hosted or target node-`5`
  observability-governance entries
- **THEN** the registry SHALL identify `SYS_*` as the formal node-`5`
  resource keep-live truth
- **AND** it SHALL identify `SystemResources.*` as supplemental
  diagnostics-only live telemetry.

#### Scenario: Hosted and target node-5 registry entries keep reviewable proof surfaces explicit
- **WHEN** the same entries describe the supported reviewable path
- **THEN** they SHALL keep `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`,
  transport-error growth, `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
  `CSP_OWNER_TIMEOUT`, `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band
  `CommEgressMux` counters in the formal reviewable scope
- **AND** they SHALL keep diagnostics-only residual chatter outside the formal
  operator baseline without claiming it has been fully removed from runtime
  output.

#### Scenario: Registry keeps the same node-5 proof path identity during detailed GET requalification
- **WHEN** the residual-governance cleanup is still repairing representative
  detailed `GET_*` proof drift on the hosted or target node-`5` path
- **THEN** the registry SHALL keep the same hosted and target path identities
  rather than inventing a parallel observability path family
- **AND** it SHALL allow the active same-change work record to cite the last
  fully requalified ancestry evidence until fresh reruns close again.

### Requirement: Hosted Node-5 Observability-Governance Proof Is Registered As A Distinct S-band Path

The verification-path registry SHALL register the hosted node-`5` S-band
observability-governance proof as a path distinct from UHF quiet, UHF beacon
suppress, and unrelated telemetry-heavy adjunct records.

#### Scenario: Registry identifies hosted node-5 auth-gated observability boundary
- **WHEN** the hosted node-`5` observability-governance proof passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> internal CSP -> hosted OBC CommController -> CommEgressMux S-band live-packet gate`
- **AND** it SHALL state that the proof covers pre-auth packet quiet,
  post-auth live packet visibility, bounded authenticated detailed `GET_*`
  readback requalification, and
  session-close suppression on the maintained hosted node-`5` path.

### Requirement: Target Node-5 Observability-Governance Proof Is Registered As A Distinct S-band Path

The verification-path registry SHALL register the service-managed target
node-`5` S-band observability-governance proof as a path distinct from quiet
node-`6`, target beacon suppress, and target non-quiet UHF diagnosis records.

#### Scenario: Registry identifies target node-5 auth-gated observability boundary
- **WHEN** the target node-`5` observability-governance proof passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> subsystem.local sband_comm_csp_node(node 5) -> shared SocketCAN -> obc.local OBC CommController -> CommEgressMux S-band live-packet gate`
- **AND** it SHALL state that the proof covers pre-auth packet quiet,
  post-auth live packet visibility, bounded authenticated detailed `GET_*`
  readback requalification, and
  session-close suppression on the maintained target node-`5` path.

#### Scenario: Registry keeps node-5 observability-governance scope bounded
- **WHEN** reviewers inspect either new node-`5` observability-governance entry
- **THEN** the entry SHALL keep its scope bounded to observability governance on
  the maintained S-band path
- **AND** it SHALL NOT widen that result into UHF command-paced summary
  closure, beacon suppress proof, generic telemetry-schema redesign, or
  one-gateway simultaneous aggregation.

### Requirement: Registry Keeps Node-5 Observability Proof Packet-Path Oracles Explicit
The verification-path registry SHALL describe the maintained hosted and target
node-`5` observability-governance entries as packet-path proofs whose bounded
readback and switch-close checkpoints stay packet-path grounded, not just
passive observer silence.

#### Scenario: Hosted node-5 observability entry states bounded readback and close semantics
- **WHEN** reviewers inspect the maintained hosted node-`5`
  observability-governance entry after the oracle-hardening follow-up
- **THEN** the entry SHALL state that bounded detailed `GET_*` requalification
  stays tied to a bounded command-specific packet-path artifact
- **AND** session-close suppression SHALL still keep gateway/downlink capture
  quiet reviewable alongside any passive observer surfaces.

#### Scenario: Target node-5 observability entry states bounded readback and close semantics
- **WHEN** reviewers inspect the maintained target node-`5`
  observability-governance entry after the oracle-hardening follow-up
- **THEN** the entry SHALL state that bounded detailed `GET_*` requalification
  stays tied to a bounded command-specific packet-path artifact
- **AND** session-close suppression SHALL still keep gateway/downlink capture
  quiet reviewable alongside any passive observer surfaces.

### Requirement: Registry Keeps Target Secure-Auth Path Identity While Allowing Source-Aware Handshake Observation
The verification-path registry SHALL keep the maintained target secure-auth path
identity unchanged when the proof oracle must recover from handshake-source
drift between wire capture and native packet logs.

#### Scenario: Target secure-auth entry keeps source-aware oracle bounded
- **WHEN** reviewers inspect the maintained target secure-auth entry after the
  oracle-hardening follow-up
- **THEN** the entry SHALL allow source-aware handshake confirmation across the
  existing wire-capture and native packet-log surfaces
- **AND** it SHALL keep that result bounded to the existing target secure-auth
  path instead of describing it as a new target path or new command plane.

### Requirement: Chapter 5 Integrated Routes Register Staged Closure Boundaries

The verification-path registry SHALL treat Chapter 5 Route 1, Route 2, and
Route 3 as distinct governed validation-path families, and each family MAY cite
multiple staged scripts or a repo-owned playbook as its governing closure
surface when the evidence record states that split explicitly.

#### Scenario: Registry distinguishes Route 1 sequence verification from adjacent paths
- **WHEN** the repository registers current Route 1 sequence verification
- **THEN** the registry SHALL identify the official sequence compile,
  governed staging upload, admission validation, execution, SoC/mode
  admission, and fresh payload-completion boundary
- **AND** it SHALL name hosted and governed target variants separately
- **AND** it SHALL keep payload-capture-only, SoC-policy-only, file/downlink-
  only, and historical integrated-route evidence distinct
- **AND** it SHALL identify A -> B -> C ownership for the target variant.

#### Scenario: Registry distinguishes Route 2 from COMM semantic overclaim
- **WHEN** the repository later registers Chapter 5 Route 2 closure
- **THEN** the registry SHALL identify the exact TTC entry, ADCS mode readback,
  and bounded link-continuity path that was proven
- **AND** it SHALL state whether the verdict used nonquiet primary UHF or quiet
  fallback
- **AND** it SHALL keep automatic S-band-to-UHF failover as a separate
  unproven or future path unless later evidence proves it explicitly

#### Scenario: Registry distinguishes Route 3 from EPS soft reboot and generic reboot claims
- **WHEN** the repository later registers Chapter 5 Route 3 closure
- **THEN** the registry SHALL identify the exact recovery chain and watchdog
  reboot boundary proven on the active runtime
- **AND** it SHALL keep EPS timeout `R3` plus `SAFE` fallback distinct from any
  nonexistent EPS software-reboot semantic
- **AND** it SHALL keep Linux reboot, hardware power-loss, and other broader
  reset claims separate unless later evidence proves them

### Requirement: Current Registry Distinguishes Historical Explicit-Switch UHF Paths From Maintained Autonomous Failover Paths

The verification-path registry SHALL keep older quiet or explicit-switch UHF
primary records reviewable as historical evidence while registering maintained
current route closure only on the non-quiet autonomous-failover path.

#### Scenario: Historical explicit-switch records remain visible but not current
- **WHEN** the registry cites older target UHF primary or dual-link records
- **THEN** it SHALL mark those records as historical or superseded if their
  acceptance boundary depended on explicit `COMM_SET_ACTIVE(UHF)` or on UHF
  primary packet quiet
- **AND** it SHALL NOT cite them as the maintained Chapter 5 current closure
  path.

#### Scenario: Current autonomous failover path is registered distinctly
- **WHEN** the new autonomous failover proof passes
- **THEN** the registry SHALL name the maintained target path as
  detector-triggered `COMM_PRIMARY_UNAVAILABLE` plus executor-owned promotion
  to non-quiet `uhf-primary-after-failover`
- **AND** it SHALL identify the governing evidence for post-failover UHF
  re-auth and bounded readback separately from historical explicit-switch
  proofs.

### Requirement: Historical Or Retired Wrapper Entrypoints Self-Identify

The verification-path registry SHALL require any script entrypoint that is
currently classified as supplemental historical or retired historical rather
than a maintained closeout gate to self-identify that status at execution time.

#### Scenario: Supplemental historical wrapper requires explicit opt-in
- **WHEN** a repository-owned wrapper is preserved only as supplemental
  historical reference and a developer invokes it without explicit override
- **THEN** the wrapper SHALL refuse to proceed
- **AND** it SHALL explain that the wrapper is not part of the current
  maintained gate set
- **AND** it SHALL state the explicit override required to run it intentionally.

#### Scenario: Retired historical wrapper fails closed
- **WHEN** a repository-owned wrapper is classified as retired historical and a
  developer invokes it
- **THEN** the wrapper SHALL exit before attempting runtime setup or proof work
- **AND** it SHALL direct the developer to the current registry or runbook
  authority instead of behaving like a maintained gate.

### Requirement: Historical Wrapper Messaging Matches Governing Docs

The verification-path registry, runbooks, and wrapper entrypoints SHALL use
consistent current-baseline wording for historical or retired probe families so
developers do not see conflicting authority signals.

#### Scenario: Wrapper messaging stays aligned with registry wording
- **WHEN** reviewers inspect a hardened historical or retired wrapper together
  with the governing docs
- **THEN** the wrapper message SHALL match the same classification vocabulary
  used by the current registry/runbook layer
- **AND** it SHALL not imply that the wrapper is a current maintained closeout
  dependency.

### Requirement: Node-5 Transport Uplift Keeps The Same Registry Path Identity

The verification-path registry SHALL retain node-`5` downlink `v2` as
transport-uplift ancestry below stock `FileDownlink`, not as a second official
path or current runtime gate.

#### Scenario: V2 registry wording preserves ownership and non-claims
- **WHEN** reviewers inspect hosted or target node-`5` V2 evidence
- **THEN** the registry SHALL keep the existing official file/downlink path
  identity and stock owner chain
- **AND** it SHALL keep node-`6` migration, reliable-transfer widening,
  payload-route replacement, and end-to-end reliable delivery as non-claims

### Requirement: Node-5 Downlink V3 Records Hosted And Target Official Path Timing Separately

The verification-path registry SHALL treat node-`5` downlink `v3` as a
transport uplift below the existing official `.fdp` path and SHALL record
transport-local and end-to-end timing separately.

#### Scenario: Hosted V3 evidence records acceptance and official timing
- **WHEN** hosted V3 evidence is recorded
- **THEN** focused transport evidence SHALL include byte-match,
  accepted-before-flushed separation, and bounded resend/no-progress behavior
- **AND** official `.fdp` evidence SHALL separate catalog completion from final
  GDS arrival and record telemetry-queue residuals

#### Scenario: Governed target V3 evidence records A-owned carrier conditions
- **WHEN** governed target V3 evidence is recorded
- **THEN** it SHALL compare current timing with the V2 branch-local reference
- **AND** it SHALL record restart, disconnect, or backpressure observations
- **AND** it SHALL record that A established scoped COMM CAN FD while C did not
  restart or remove shared target services
- **AND** it SHALL distinguish CAN FD frame format from unproven BRS,
  data-phase-rate, RF, and OTA claims

### Requirement: Verification Path Registry Records Mission Console Beacon Viewer As A Separate Operator Path

The verification path registry SHALL record Mission Console beacon viewer proof
as a dedicated operator-facing surface distinct from lower-level UHF beacon
runtime, beacon suppress semantics, stock GDS observability, and explicit
readback families.

#### Scenario: Hosted Mission Console beacon viewer path is registered separately
- **WHEN** hosted Mission Console beacon-viewer evidence is recorded
- **THEN** the registry entry SHALL identify the path as hosted manual-surface
  beacon capability plus Mission Console dashboard/`/beacon` viewing
- **AND** it SHALL NOT over-claim stock GDS beacon visibility, RF/OTA receipt,
  or readback semantics.

#### Scenario: Target Mission Console beacon viewer path records A-owned mirrored sidecar provenance
- **WHEN** target Mission Console beacon-viewer evidence is recorded
- **THEN** the registry entry SHALL identify A-owned remote Beacon sidecar
  readiness, target manual ground capture mirroring, and Mission Console viewer
- **AND** it SHALL state that C did not restart, override, or stop shared
  target services
- **AND** it SHALL state that the Mission Console proof consumes ground-visible
  mirrored artifacts, not a direct RF/OTA beacon receipt path.

### Requirement: Published Verification Entrypoints Are Manifest Governed
Every source executable SHALL have a publication status, and every shipped
verification entrypoint SHALL cite a current registry path or operator surface.

#### Scenario: Script inventory is checked
- **WHEN** the source executable inventory and public tree are compared
- **THEN** maintained and support/internal scripts SHALL be the only shipped
  executables
- **AND** historical, retired, deprecated, and alias-only scripts SHALL be
  absent

### Requirement: Registry Exposes Stable Current And Historical Status
The repository SHALL present current verification coverage in
`docs/verification.md` and SHALL retain the detailed path ledger under
`evidence/verification-path-registry.md` for engineering traceability.

#### Scenario: Reviewer selects a validation path
- **WHEN** a reviewer starts from the verification overview
- **THEN** maintained build, test, hosted, and target/lab evidence SHALL be
  discoverable by capability
- **AND** readers needing exact path identities SHALL be routed to the detailed
  evidence ledger
