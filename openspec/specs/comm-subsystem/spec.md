# comm-subsystem Specification

## Purpose
Define the first-version external communications architecture, generic comms component families, transport strategy, and comms-owned public contracts.
## Requirements
### Requirement: External Link Transport Strategy
The comm subsystem SHALL keep the external comms link separate from the internal CSP network, SHALL use TCP mock as the first-version development transport, and SHALL allow target integration to switch to UART through profile and instance selection.

#### Scenario: External link transport changes by profile
- **WHEN** the project moves from software-only development to Raspberry Pi integration
- **THEN** the external link transport MAY change from TCP mock to UART without redefining the internal CSP network baseline

### Requirement: Generic Comms Component Families

The subsystem SHALL implement `CommController`, `UartDriver`, and `RadioController` as reusable component families or single controller roles, SHALL use instance names rather than renamed public symbols to distinguish S-band and UHF behavior, and SHALL keep the transport choice below the controller layer.

#### Scenario: First implementation keeps transport-specific details below the controller layer
- **WHEN** `RadioController` is validated through TCP mock and PTY-backed byte-stream paths
- **THEN** the controller logic SHALL operate through a shared transport abstraction instead of branching on the concrete transport type

#### Scenario: Runtime beacon writes are drained by UART runtime path
- **WHEN** a live beacon frame is handed to `UartDriver` while its transport is connected and its runtime queue is empty
- **THEN** the driver SHALL accept it through a bounded runtime queue
- **AND** it SHALL drain that queue from `UartDriver.schedIn` instead of requiring the producer caller to perform byte-stream writes inline
- **AND** it SHALL reject the handoff when no connected transport path is available

#### Scenario: Byte-stream writes use timeout-bounded polling
- **WHEN** TCP or serial byte-stream transport sends data
- **THEN** the transport SHALL poll for write readiness using its configured timeout
- **AND** it SHALL return a timeout or I/O error instead of spinning indefinitely in an unbounded write loop

### Requirement: Comms Public Contract

The subsystem SHALL own the `COMM_*`, `RADIO_*`, and `UART_*` command, telemetry, and event families required for band selection, pass control, UART health, radio status, and error reporting.

#### Scenario: Pass window state is emitted by `CommController`
- **WHEN** a pass window starts, ticks down, or stops
- **THEN** `CommController` SHALL publish active-band, pass-active, remaining-seconds, and total-pass state through its owned telemetry and events

#### Scenario: Radio status is available without hardware-specific framing
- **WHEN** `RADIO_GET_STATUS` runs against the first hosted mock-radio backend
- **THEN** the subsystem SHALL return enabled, power, frequency, temperature, and RSSI status without requiring KISS or a vendor-specific protocol

### Requirement: Comms Verification Modes
The subsystem SHALL support validation through TCP mock and PTY-backed virtual UART paths, SHALL classify true hardware UART or radio validation as `Blocked-HW` when the necessary hardware path is still unavailable, SHALL provide a hosted comm mock executable that the integrated software-only OBC runtime can use without test-only harness code, SHALL expose a hosted ground path that uses `Drv::TcpClient` to connect the OBC deployment to `fprime-gds`, and SHALL provide a repo-local Raspberry Pi target integration path where the same controller logic runs against either TCP mock or an explicit UART device path selected by profile configuration.

#### Scenario: Hosted ground path uses the documented TCP client link
- **WHEN** the hosted OBC deployment runs against `fprime-gds`
- **THEN** the deployment SHALL bring up the F' command/event/tlm stack and connect to the documented IP adapter port through `Drv::TcpClient`

#### Scenario: Raspberry Pi target stack reuses shared comm logic
- **WHEN** the project launches the `integ-rpi` profile on a Raspberry Pi target
- **THEN** the external link SHALL be selectable through runtime transport settings such as TCP mock or UART device path while `CommController`, `RadioController`, and `UartDriver` keep the same controller-layer behavior as the hosted profile

### Requirement: Raspberry Pi Host UART Hardware Validation Path
The comm subsystem SHALL provide a governed validation path where the Raspberry Pi target runs the existing `CommController`, `RadioController`, and `UartDriver` stack against a development-host mock-radio peer over an explicit hardware serial device path, and this path SHALL preserve the same controller-layer behavior already validated through TCP mock and PTY-backed serial modes.

#### Scenario: Host mock-radio backend attaches to a real serial device
- **WHEN** an operator launches the hosted mock-radio backend for hardware-UART validation
- **THEN** the backend SHALL be able to open an explicit macOS serial device path and serve the existing mock-radio protocol over that hardware link instead of TCP or PTY

#### Scenario: Raspberry Pi target uses an explicit UART device path
- **WHEN** the Raspberry Pi target stack is launched for hardware-UART validation
- **THEN** the runtime SHALL accept an explicit target serial device path and run the external comm link through that device while keeping the controller-layer command and telemetry behavior unchanged

#### Scenario: Real serial hardware mode does not redefine the transport contract
- **WHEN** the project switches from PTY-backed validation to the Raspberry Pi to host hardware-UART path
- **THEN** the shared byte-stream transport contract SHALL remain the controller-facing interface and SHALL NOT require a protocol redesign

### Requirement: Radio Protocol Adapter Strategy
The comm subsystem SHALL route radio command/status framing through a protocol adapter layer that sits below `RadioController` and above the shared byte-stream transport, so future framing changes do not require controller or transport redesign.

#### Scenario: Controller logic stays protocol-agnostic
- **WHEN** `RadioController` requests status, enable, power, or frequency operations
- **THEN** it SHALL continue to use the same controller-visible command, telemetry, and event behavior regardless of which supported radio protocol adapter is selected underneath

#### Scenario: Default adapter preserves hosted mock behavior
- **WHEN** the runtime uses the default first-version radio protocol adapter
- **THEN** the subsystem SHALL preserve the existing hosted text mock-radio request/response behavior already validated through TCP, PTY, and Raspberry Pi UART paths

#### Scenario: Future protocol selection is configuration-driven
- **WHEN** the project later adds KISS or a vendor-specific adapter
- **THEN** the selected adapter SHALL be chosen through runtime or profile configuration rather than by rewriting `RadioController` or the byte-stream transport implementation

### Requirement: Legacy EnduroSat Transparent UART Validation Path
The comm subsystem SHALL provide a governed validation path for legacy EnduroSat-style UART transparent mode that uses the existing serial byte-stream transport and the OBC-side raw UART exchange path to validate payload delivery without requiring the current `mock-text` request/response grammar.

#### Scenario: Legacy transparent peer is selected for hardware-adjacent validation
- **WHEN** the project launches the legacy EnduroSat transparent-UART validation flow
- **THEN** the host-side peer SHALL behave as a transparent serial endpoint for payload exchange rather than as the current command/status mock-radio peer

#### Scenario: Transparent validation reuses the existing byte-stream transport
- **WHEN** the Raspberry Pi target validates the legacy EnduroSat transparent-UART path
- **THEN** the flow SHALL reuse the existing serial byte-stream transport contract and SHALL NOT require a controller-layer redesign

### Requirement: Transparent UART Path Stays Distinct From Controller-Oriented Mock Radio
The comm subsystem SHALL keep the controller-oriented `mock-text` radio baseline and the legacy EnduroSat transparent-UART validation path as separate governed modes so that controller regression and transparent data-path validation remain distinguishable.

#### Scenario: Default comm regression still uses the existing mock radio
- **WHEN** the standard hosted or Raspberry Pi comm regression suite runs without selecting the legacy transparent path
- **THEN** the subsystem SHALL continue to use the current `mock-text`-based radio validation behavior

#### Scenario: Transparent mode does not over-claim unsupported control semantics
- **WHEN** the legacy transparent-UART path is exercised
- **THEN** the subsystem SHALL treat that validation as a data-plane-first path and SHALL NOT claim that legacy EnduroSat control/configuration behavior is fully implemented unless a later change adds that control plane explicitly

### Requirement: Transparent Link Framing Above UART Transport
The comm subsystem SHALL provide a repository-owned transparent link framing mode above the existing serial byte-stream transport so that the legacy transparent-UART path can exchange binary-safe framed payloads without replacing the current TCP/GDS baseline or `mock-text` radio baseline.

#### Scenario: Framed transparent mode reuses the current serial link
- **WHEN** the project selects the framed transparent-UART validation path
- **THEN** the OBC SHALL reuse the existing serial transport and SHALL add framing above that transport rather than redefining the hardware link itself

#### Scenario: Direct TCP ground baseline remains unchanged
- **WHEN** the project runs the direct `fprime-gds` integration path
- **THEN** that path SHALL continue to use the existing documented TCP connection and SHALL NOT depend on the transparent framing mode

### Requirement: Framed Transparent Path Is Binary-Safe
The transparent link framing mode SHALL support payload bytes that include delimiter and escape values, SHALL define explicit frame boundaries, and SHALL detect payload corruption using CRC-32.

#### Scenario: Payload includes reserved bytes
- **WHEN** the framed transparent path carries payload bytes that include the framing delimiter, escape byte, or `0x00`
- **THEN** the link layer SHALL escape and recover those bytes without truncation or ambiguity

#### Scenario: Corrupted frame is rejected
- **WHEN** the host or target receives a framed payload whose CRC-32 does not match
- **THEN** the frame SHALL be rejected and the exchange SHALL report a framing or transport error instead of silently accepting corrupted data

### Requirement: Host Transparent Peer Can Deframe Governed Frames
The host-side transparent peer SHALL support a governed mode that deframes the repository-owned transparent link format, validates the frame, and returns a framed response over the same serial link.

#### Scenario: Framed transparent peer echoes decoded payload
- **WHEN** the host-side peer receives a valid framed payload
- **THEN** it SHALL decode the payload, preserve the recovered bytes, and return a framed response using the same link format

### Requirement: Framed Transparent Path Supports Sustained Binary-Safe Exchange
The comm subsystem SHALL provide a governed validation path where the repository-owned transparent frame v1 carries repeated binary-safe payloads over the existing Raspberry Pi to host UART/RS485 path without replacing the current direct TCP/GDS or `mock-text` baselines.

#### Scenario: Repeated framed payloads remain reviewable
- **WHEN** the project runs the governed framed transparent robustness probe
- **THEN** the host and target SHALL be able to exchange multiple framed payloads, including representative binary-safe payloads, over the same serial session and expose a reviewable success summary

### Requirement: Framed Transparent Path Recovers After Host-Peer Restart
The comm subsystem SHALL provide a governed validation path where the framed transparent UART flow detects a host-side peer interruption, allows the peer to be restarted, and supports a subsequent successful framed exchange without redefining the frame format or replacing the existing serial transport contract.

#### Scenario: Framed exchange succeeds after peer restart
- **WHEN** the host-side framed transparent peer is interrupted and then restarted during governed validation
- **THEN** a later framed exchange over the same governed hardware path SHALL succeed and SHALL preserve the same frame v1 encode/decode behavior

### Requirement: Historical Raspberry Pi Comm UART Baseline Evidence Remains Reviewable
The comm subsystem SHALL preserve the older Raspberry Pi `/dev/serial0` external comm evidence as historical evidence after later changes reassign that UART to another active hardware path.

#### Scenario: Historical target comm validation remains auditable
- **WHEN** reviewers inspect the older Raspberry Pi CSP + comm baseline probe or OBC-side UART probe
- **THEN** they SHALL still be able to see that `CommController`, `RadioController`, and `UartDriver` exchanged data over the former target UART device against a host-side mock peer
- **AND** the evidence SHALL NOT be restated as the current active target baseline once serial ownership changes

### Requirement: OBC-Side Serial Comm Evidence Is Historical After GPS Reallocation
Once the governed target GPS live UART path reallocates `obc.local:/dev/serial0` to GPS, the repository SHALL preserve the prior OBC-side serial comm evidence as historical evidence and SHALL NOT continue to describe it as the current active target baseline.

#### Scenario: Historical comm evidence remains reviewable without overstating current ownership
- **WHEN** reviewers inspect older OBC-side serial comm records that used `/dev/serial0`
- **THEN** those records SHALL remain available as historical proof of prior behavior, but current baseline wording SHALL state that active comm development has moved back to TCP/dev paths until later comm migration work lands

### Requirement: Historical Dual-Pi Comm Coexistence Evidence Remains Reviewable
The comm subsystem SHALL preserve the earlier dual-Pi split-host comm coexistence result as historical evidence after `obc.local:/dev/serial0` is reassigned away from the active comm path.

#### Scenario: Older coexistence result is not mistaken for the current target baseline
- **WHEN** reviewers inspect the dual-Pi split-host coexistence record
- **THEN** they SHALL be able to see that external comm previously coexisted with the split-host subsystem path on `obc.local:/dev/serial0`
- **AND** the repository SHALL describe that result as historical rather than as the current active target comm hardware path

### Requirement: Comm Is The Future Ground-Facing Spacecraft Subsystem
The comm subsystem SHALL be treated as the long-term spacecraft-side subsystem responsible for handling omitted-RF ground TT&C traffic before that traffic reaches OBC-owned business logic, while preserving the current controller-oriented mock and UART paths as bounded development baselines.

#### Scenario: Current mock-radio paths stay bounded rather than becoming the end-state narrative
- **WHEN** the repository evolves the comm architecture beyond the first mock-radio slices
- **THEN** it SHALL keep the existing `mock-text`, transparent, and framed paths as governed development evidence without treating them alone as the final TT&C architecture

### Requirement: Comm Participates In The Future CSP Subsystem Topology
The future comm architecture SHALL allow `COMM` to participate as a CSP-facing subsystem alongside `EPS` and `ADCS`, and the first governed implementation SHALL assign `COMM` to node `4` with reserved COMM-owned application service ports `30` through `39`.

#### Scenario: Future COMM integration remains distinct from direct GDS and GPS
- **WHEN** the repository later adds the governed COMM CSP-facing path
- **THEN** that path SHALL remain distinct from the direct `GDS -> TCP -> OBC` development path and from the direct GPS sensor path

#### Scenario: First COMM CSP slice reserves stable identity and service ownership
- **WHEN** the first governed COMM CSP path is implemented
- **THEN** the repository SHALL keep node `4` and application service ports `30` through `39` reserved for COMM-owned traffic instead of reusing EPS or ADCS port ranges

#### Scenario: Deployed COMM clients use owner-managed runtime access
- **WHEN** `CommController`, `GroundLinkDriver`, or `CspBridge` uses the CSP client path in the deployed topology
- **THEN** those COMM-facing runtime calls SHALL execute through the topology-owned runtime owner boundary
- **AND** the existing COMM public commands, events, telemetry, and service-port contracts SHALL remain unchanged

### Requirement: Omitted-RF TT&C Ingress Remains Distinct From The Spacecraft Internal Bus
The comm subsystem SHALL allow a first omitted-RF lab ingress path that uses a governed serial ingress into the subsystem-side comm path, and that ingress SHALL be treated as the RF-omitted boundary rather than as the spacecraft internal subsystem bus.

#### Scenario: Lab-side UART ingress does not redefine the spacecraft-side carrier
- **WHEN** a future omitted-RF TT&C change uses subsystem-side UART ingress
- **THEN** the repository SHALL still treat spacecraft-side `CAN FD` as the planned internal carrier direction for `EPS`, `ADCS`, and `COMM`

#### Scenario: First COMM CSP slice keeps legacy external comm baseline separate
- **WHEN** the first gateway-backed COMM path is validated through subsystem-side serial ingress
- **THEN** the repository SHALL continue to describe the older mock, transparent, and framed external comm paths as separate governed development baselines instead of folding them into the COMM CSP proof

### Requirement: First COMM CSP Path Bridges Bounded Ground-Link Chunks
The comm subsystem SHALL provide a first governed CSP-facing `COMM` node that bridges bounded omitted-RF ground-link byte chunks between the lab-side serial ingress and the spacecraft-side internal CSP bus while preserving the existing controller-oriented mock and UART comm path as a separate baseline.

#### Scenario: COMM node bridges serial ingress and internal CSP
- **WHEN** the repository runs the first gateway-backed omitted-RF TT&C path
- **THEN** the subsystem-side `COMM` process SHALL accept bounded stock F' framed byte chunks from the governed lab-side serial ingress
- **AND** it SHALL exchange those chunks with `OBC` over COMM-owned CSP services without redefining the older controller-oriented external comm path as the same architecture domain

### Requirement: Hosted COMM Simulator Has A Model Layer
The comm subsystem SHALL implement the hosted `COMM` CSP stand-in as a simulator model plus server shell, where the model owns business state and behavior while the server shell owns libcsp and serial I/O integration.

#### Scenario: COMM model preserves existing CSP service contract
- **WHEN** the hosted `COMM` node handles `UPLINK_POLL`, `DOWNLINK_WRITE`, or `LINK_STATUS`
- **THEN** the node SHALL preserve the existing service ports `30`, `31`, and `32` and their request/reply wire layouts
- **AND** the simulator model SHALL provide the service semantics without adding a new CSP debug or status service in this change

#### Scenario: COMM model tracks bounded uplink state
- **WHEN** serial ingress provides uplink bytes to the hosted `COMM` simulator
- **THEN** the simulator model SHALL store those bytes in a bounded queue with deterministic drop-new overflow behavior
- **AND** overflow SHALL preserve already queued bytes while incrementing reviewable model status counters

#### Scenario: COMM model tracks downlink acceptance state
- **WHEN** OBC writes a bounded downlink chunk through the hosted `COMM` CSP service
- **THEN** the simulator model SHALL accept the chunk only when the link is available and the model is not applying downlink backpressure or injected error state
- **AND** rejected downlink writes SHALL map to the existing CSP result vocabulary rather than changing the wire contract

#### Scenario: Model status remains internal for the first foundation slice
- **WHEN** tests or future hosted simulator code inspect `COMM` simulator state
- **THEN** queue depth, overflow, backpressure, disconnect, and error-injection state SHALL be available through C++ model-facing status APIs
- **AND** the existing `LINK_STATUS` wire reply SHALL remain compatible with the previously validated gateway-backed path

### Requirement: Subsystem COMM UART Link Preflight
The comm subsystem SHALL provide a governed preflight validation path for the macOS-initiated physical UART request/reply link between the macOS development host and `subsystem.local`, and that path SHALL use explicit serial device paths on both hosts.

#### Scenario: Subsystem serial link exchanges bounded mock-text traffic
- **WHEN** `subsystem.local` runs the existing mock-text serial peer on an explicit subsystem serial device
- **AND** the macOS host runs the serial probe on an explicit host serial device
- **THEN** the macOS-initiated probe SHALL exchange bounded `STATUS`, `ENABLE 1`, and final `STATUS` requests over the physical serial link
- **AND** the final observed status SHALL show the link carried the enabled state update

#### Scenario: Subsystem cold-first traffic is not implied
- **WHEN** the subsystem UART preflight evidence is reused by later COMM work
- **THEN** the repository SHALL treat clean `subsystem.local` cold-first traffic into a passive macOS receiver as unproven unless later evidence proves that acquisition behavior separately

#### Scenario: Subsystem UART preflight stays distinct from TT&C
- **WHEN** the subsystem UART preflight evidence is recorded
- **THEN** the repository SHALL describe it as physical serial byte-exchange evidence only
- **AND** it SHALL NOT describe that result as clean subsystem-origin cold-first downlink, gateway-backed TT&C, RF, real radio, file/downlink, target OBC, or COMM shared CAN FD validation

### Requirement: Lab Serial Acquisition For Subsystem-Origin Bytes
The comm subsystem SHALL provide a governed focused probe that validates bounded subsystem-origin serial acquisition from `subsystem.local` to a passive macOS receiver over the physical lab serial wiring.

#### Scenario: Passive macOS receiver acquires subsystem-origin frames
- **WHEN** macOS opens the explicit host serial endpoint before `subsystem.local` transmits
- **AND** `subsystem.local` sends the governed preamble and bounded acquisition frames over `/dev/serial0`
- **THEN** the macOS receiver SHALL extract the exact framed payload sequence from the serial stream

#### Scenario: Acquisition proof stays below TT&C
- **WHEN** the lab serial acquisition probe passes
- **THEN** the repository SHALL describe the result as subsystem-origin serial acquisition only
- **AND** it SHALL NOT describe that result as gateway-backed TT&C, RF, file/downlink, target OBC, or COMM shared CAN FD validation

### Requirement: Physical Lab Serial Uplink Ingress
The comm subsystem SHALL provide a governed staged probe that can validate bounded macOS-origin TT&C uplink ingress through the physical lab serial link into `subsystem.local` COMM node `4` and onward to hosted OBC over the existing COMM CSP service contract.

#### Scenario: Physical serial ingress reaches OBC command readback
- **WHEN** macOS runs the ground TT&C gateway against the explicit host serial endpoint
- **AND** `subsystem.local` runs native-built `comm_csp_node` on `/dev/serial0` as node `4`
- **AND** hosted OBC runs with `GROUND_LINK_MODE=comm-csp`
- **THEN** bounded GDS commands SHALL be able to change OBC EPS and ADCS readback before the repository claims physical lab serial uplink ingress

#### Scenario: COMM contract remains unchanged
- **WHEN** the lab serial ingress probe runs
- **THEN** the COMM node SHALL retain node `4`
- **AND** the path SHALL keep using services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT introduce new COMM service ports or wire layouts

#### Scenario: Downlink does not gate uplink ingress verdict
- **WHEN** bounded uplink commands reach OBC readback but events or telemetry are not visible through `fprime-cli`
- **THEN** the repository MAY record physical lab serial uplink ingress as the formal verdict
- **AND** it SHALL record downlink or full TT&C as diagnostic or not proven

### Requirement: Physical Lab Serial Downlink Proof
The comm subsystem SHALL provide a governed focused probe that validates bounded ground-visible event and telemetry downlink over the physical lab serial COMM path after bounded physical uplink ingress to hosted OBC has already succeeded in the same run.

#### Scenario: Physical serial downlink requires command readback first
- **WHEN** macOS runs the ground TT&C gateway against the explicit host serial endpoint
- **AND** `subsystem.local` runs native-built `comm_csp_node` on `/dev/serial0` as node `4`
- **AND** hosted OBC runs with `GROUND_LINK_MODE=comm-csp`
- **THEN** the probe SHALL first verify bounded EPS and ADCS command readback through the physical serial COMM path before claiming downlink success

#### Scenario: Events and telemetry are visible through the physical COMM path
- **WHEN** the focused physical lab serial downlink probe reaches its downlink verdict
- **THEN** `fprime-cli events` SHALL observe bounded command event output for the commands sent through the path
- **AND** `fprime-cli channels` SHALL observe `GROUND_LINK_TX_BYTES`
- **AND** the resulting evidence MAY describe the path as bounded physical lab serial TT&C

#### Scenario: COMM service contract remains unchanged
- **WHEN** the physical lab serial downlink probe runs
- **THEN** COMM SHALL retain node `4`
- **AND** the path SHALL keep using services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT introduce new COMM service ports or wire layouts

#### Scenario: Adjacent COMM futures stay out of the verdict
- **WHEN** the physical lab serial downlink evidence is recorded
- **THEN** the verdict SHALL NOT claim file/downlink, RF behavior, target OBC migration, no-preamble first-byte-clean behavior, ScenarioBridge pass state, link availability policy, or COMM shared CAN FD participation

### Requirement: COMM TT&C Official Data-Product File Downlink Proof
The comm subsystem SHALL provide a governed validation path that proves official `.fdp` file/downlink behavior over the COMM TT&C path without adding new COMM service ports, wire layouts, or generic arbitrary-file commands.

#### Scenario: File downlink uses official catalog commands
- **WHEN** the COMM file/downlink probe runs
- **THEN** it SHALL use `DpCatalog.BUILD_CATALOG` and `DpCatalog.START_XMIT_CATALOG`
- **AND** it SHALL NOT add a new operator command for arbitrary onboard file paths

#### Scenario: Official `.fdp` files are downlinked
- **WHEN** the COMM file/downlink proof reaches its file verdict
- **THEN** the probe SHALL have produced one or more official `.fdp` files under the governed runtime `data-products` directory
- **AND** it SHALL downlink one or more catalog-selected `.fdp` files over the same GDS/COMM path

#### Scenario: Received files match OBC runtime sources
- **WHEN** official `.fdp` files are received by the ground-side file-storage directory
- **THEN** the probe SHALL compare the received `.fdp` files against the OBC runtime source files
- **AND** the formal file/downlink verdict SHALL require byte-for-byte matches for the selected files

#### Scenario: COMM service contract remains unchanged
- **WHEN** the COMM file/downlink probe runs
- **THEN** COMM SHALL retain node `4`
- **AND** the path SHALL keep using services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT introduce new COMM service ports or wire layouts

#### Scenario: Adjacent COMM futures stay out of the verdict
- **WHEN** COMM file/downlink evidence is recorded
- **THEN** the verdict SHALL NOT claim RF behavior, target OBC migration, no-preamble first-byte-clean behavior, ScenarioBridge pass state, link availability policy, arbitrary file downlink, or COMM shared CAN FD participation

### Requirement: COMM SocketCAN TT&C Participation
The comm subsystem SHALL provide a governed validation path where COMM node `4` participates on the spacecraft-side SocketCAN carrier through `subsystem.local:can1` while preserving the existing gateway-backed TT&C service contract.

#### Scenario: COMM node joins the shared SocketCAN carrier
- **WHEN** the COMM SocketCAN TT&C probe runs
- **THEN** `comm_csp_node` SHALL run as COMM node `4` through `CSP_TRANSPORT=socketcan` on `subsystem.local:can1`
- **AND** EPS and ADCS SHALL remain reachable through `subsystem.local:can0`

#### Scenario: COMM service contract remains unchanged
- **WHEN** COMM participates over SocketCAN
- **THEN** COMM SHALL keep using services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add COMM service ports or wire layouts

#### Scenario: Gateway-backed TT&C reaches OBC over SocketCAN
- **WHEN** macOS sends bounded GDS commands through `ground_ttc_gateway` and the lab serial ingress
- **THEN** the commands SHALL traverse COMM node `4` over the shared SocketCAN carrier and produce bounded OBC readback, command events, and `GROUND_LINK_TX_BYTES`

#### Scenario: Adjacent COMM futures stay out of the verdict
- **WHEN** COMM SocketCAN TT&C evidence is recorded
- **THEN** the verdict SHALL NOT claim file/downlink behavior, RF behavior, no-preamble first-byte-clean behavior, target OBC migration beyond this target OBC run, dual-bus redundancy, or independent COMM hardware beyond the `subsystem.local:can1` controller

### Requirement: COMM SocketCAN Official Data-Product File Downlink Validation
The comm subsystem SHALL provide a governed validation path where official `DpCatalog` file/downlink commands traverse COMM node `4` over the shared SocketCAN carrier to target OBC and return received files to GDS file storage.

#### Scenario: File downlink uses existing COMM SocketCAN path
- **WHEN** the COMM SocketCAN file/downlink probe runs
- **THEN** `comm_csp_node` SHALL run as COMM node `4` through `CSP_TRANSPORT=socketcan` on `subsystem.local:can1`
- **AND** target OBC SHALL run on `obc.local:can0` with `GROUND_LINK_MODE=comm-csp`
- **AND** EPS and ADCS SHALL remain reachable through `subsystem.local:can0`

#### Scenario: Existing file command surface is reused
- **WHEN** official `.fdp` file/downlink is validated over COMM SocketCAN
- **THEN** the path SHALL use `DpCatalog.BUILD_CATALOG` and `DpCatalog.START_XMIT_CATALOG`
- **AND** the change SHALL NOT add a generic arbitrary-file command, COMM service port, or wire layout

#### Scenario: File packets stay bounded for the physical COMM carrier
- **WHEN** official `.fdp` file/downlink is validated over COMM SocketCAN
- **THEN** the project SHALL keep stock F' `FileDownlink` while bounding `FW_FILE_BUFFER_MAX_SIZE`
- **AND** the active payload dual-artifact slice MAY raise that global bound when the stock CCSDS/file path is revalidated with the larger packet size
- **AND** the verdict SHALL still require byte-matched files instead of treating bounded command success as sufficient proof

#### Scenario: File verdict requires byte matches
- **WHEN** `DpCatalog.START_XMIT_CATALOG` completes over the SocketCAN-backed COMM path
- **THEN** GDS file storage SHALL contain one or more selected `.fdp` files
- **AND** each received file SHALL match its target OBC runtime source snapshot byte-for-byte

#### Scenario: Adjacent futures stay out of the verdict
- **WHEN** COMM SocketCAN file/downlink evidence is recorded
- **THEN** the verdict SHALL NOT claim arbitrary onboard file path downlink, RF behavior, no-preamble first-byte-clean behavior, archive wraparound, ScenarioBridge/pass automation, dual-bus redundancy, independent COMM hardware beyond the `subsystem.local:can1` controller, or end-to-end missing-packet retransmission under packet loss

### Requirement: Service-Managed Subsystem COMM CSP Lab Runtime
The comm subsystem SHALL support a workspace-based lab runtime on `subsystem.local` where EPS, ADCS, and COMM node processes are managed by systemd with separate service failure boundaries.

#### Scenario: Subsystem aggregate target starts separate node services
- **WHEN** the subsystem lab runtime is installed and enabled
- **THEN** `subsystem-sband-csp-stack.target` and `subsystem-uhf-csp-stack.target` SHALL each aggregate separate EPS, ADCS, and COMM services instead of running all three long-lived processes inside one stack service
- **AND** each service SHALL have distinct systemd status and journal surfaces

#### Scenario: EPS and ADCS stay on the EPS/ADCS CAN channel
- **WHEN** the subsystem aggregate target starts EPS and ADCS services
- **THEN** EPS node `2` and ADCS node `3` SHALL run over `subsystem.local:can0` with the configured SocketCAN transport settings

#### Scenario: S-band and UHF COMM stay split by carrier boundary
- **WHEN** the subsystem aggregate targets start the COMM services
- **THEN** S-band COMM node `5` SHALL run over `subsystem.local:can1` with its configured TCP listener
- **AND** UHF COMM node `6` SHALL run over `subsystem.local:can1` plus the configured lab serial ingress device and baudrate for the RF-omitted ground path

#### Scenario: Subsystem runtime remains non-root
- **WHEN** the subsystem EPS, ADCS, and COMM services start their long-running runtime processes
- **THEN** those runtime processes SHALL run as the configured non-root workspace user for this lab baseline
- **AND** failures caused by missing workspace build outputs SHALL be reported as setup failures rather than silently falling back to root execution

### Requirement: Lab Operational COMM SocketCAN Path Reuses Existing Contract
The lab operational COMM SocketCAN baseline SHALL preserve the existing COMM
node identity, service ports, stock F' framing, and bounded proof scope while
moving runtime management from probes into services. Current file/downlink
validation SHALL be described through official data-product file/downlink
surfaces rather than retired HK fallback files.

#### Scenario: COMM contract remains unchanged under services
- **WHEN** command/event/channel or official data-product file/downlink validation runs over the service-managed lab path
- **THEN** COMM SHALL retain node `4`
- **AND** it SHALL keep services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add COMM service ports, wire layouts, custom GDS plugins, RF behavior, reliable retransmission, or arbitrary onboard file downlink

### Requirement: COMM-Facing Beacon Sink
The comm subsystem SHALL provide or accept a spacecraft-side sink for live beacon frames that sends bounded payloads through the hosted COMM-facing path without requiring ground acknowledgement, command response, or RF validation.

#### Scenario: Beacon broadcast does not alter COMM service contract
- **WHEN** this change adds live beacon broadcast behavior
- **THEN** existing COMM node identity and TT&C service ports SHALL remain unchanged
- **AND** the change SHALL NOT require replacing `ComFprime` or adding CCSDS framing

#### Scenario: Beacon path claim remains bounded
- **WHEN** the hosted beacon probe captures a frame
- **THEN** the evidence SHALL identify it as hosted/COMM-facing behavior
- **AND** it SHALL NOT claim physical RF, real-radio, CFDP, ARQ, or pass-window behavior

### Requirement: Dual-Link COMM Simulator Foundation
The comm subsystem SHALL provide explicit hosted simulator process identities for generic compatibility COMM, S-band COMM, and UHF COMM while preserving the existing COMM CSP service contract.

#### Scenario: Executables define distinct link identities
- **WHEN** the hosted COMM simulator executables are built
- **THEN** `comm_csp_node` SHALL remain the generic compatibility COMM executable with default node `4`
- **AND** `sband_comm_csp_node` SHALL provide the S-band COMM executable identity with default node `5`
- **AND** `uhf_comm_csp_node` SHALL provide the UHF COMM executable identity with default node `6`

#### Scenario: Link identities reuse the existing COMM services
- **WHEN** generic, S-band, or UHF COMM simulator identities handle CSP ground-link requests
- **THEN** each identity SHALL use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add COMM service ports or alter the existing request/reply wire layouts

#### Scenario: Shared implementation keeps executable boundaries visible
- **WHEN** the COMM simulator identities share `CommNodeServer` or `CommSimModel` implementation
- **THEN** each executable's startup logs, launcher/probe logs, and evidence SHALL identify the executable name, link identity, node id, serial endpoint, baudrate, and CSP interface name

#### Scenario: Foundation scope excludes complete link paths
- **WHEN** the dual-link simulator foundation evidence is recorded
- **THEN** the evidence SHALL NOT claim complete S-band GDS path, UHF UART backup path, CCSDS behavior, RF behavior, reliable transfer, file/downlink behavior, or Pi hardware validation

### Requirement: S-band TCP Ground Link Uses COMM Node 5
The comm subsystem SHALL support a hosted S-band simulated TCP ground-link path through `sband_comm_csp_node` as CSP node `5` while preserving generic COMM node `4` compatibility, UHF node `6` foundation behavior, and the existing COMM CSP service contract.

#### Scenario: S-band executable owns the TCP endpoint
- **WHEN** the hosted S-band TCP ground-link probe runs
- **THEN** `sband_comm_csp_node` SHALL run as link identity `sband` with CSP node `5`
- **AND** it SHALL listen on the configured S-band simulated TCP endpoint
- **AND** the probe SHALL NOT use `comm_csp_node` node `4` as S-band evidence

#### Scenario: OBC reaches GDS through COMM rather than direct TCP
- **WHEN** the hosted S-band TCP ground-link probe validates command, event, telemetry, or file/downlink behavior
- **THEN** hosted OBC SHALL run with `GROUND_LINK_MODE=comm-csp` and `COMM_CSP_NODE=5`
- **AND** direct `GDS -> TCP -> OBC` SHALL remain disabled or outside the verdict boundary

#### Scenario: COMM service contract remains unchanged
- **WHEN** S-band TCP traffic traverses COMM
- **THEN** S-band SHALL use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add COMM service ports or alter the existing request/reply wire layouts

#### Scenario: Adjacent COMM futures stay out of the verdict
- **WHEN** S-band TCP ground-link evidence is recorded
- **THEN** the verdict SHALL NOT claim UHF UART backup, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, pass scheduling, or arbitrary onboard file downlink

### Requirement: UHF Hosted UART Backup Uses COMM Node 6
The comm subsystem SHALL support a hosted UHF UART/RS485/USB/macOS stand-in backup command ingress path through `uhf_comm_csp_node` as CSP node `6` while preserving generic COMM node `4` compatibility, S-band node `5` behavior, and the existing COMM CSP command/downlink service contract.

#### Scenario: UHF executable owns the hosted serial endpoint
- **WHEN** the hosted UHF UART backup probe runs
- **THEN** `uhf_comm_csp_node` SHALL run as link identity `uhf` with CSP node `6`
- **AND** it SHALL use the configured hosted serial endpoint and baudrate for backup ingress
- **AND** the probe SHALL NOT use `comm_csp_node` node `4`, `sband_comm_csp_node` node `5`, S-band TCP, or direct `GDS -> TCP -> OBC` as UHF evidence

#### Scenario: OBC reaches GDS through UHF COMM rather than direct TCP
- **WHEN** the hosted UHF UART backup probe validates command, event, or telemetry behavior
- **THEN** hosted OBC SHALL run with `GROUND_LINK_MODE=comm-csp` and `COMM_CSP_NODE=6`
- **AND** direct `GDS -> TCP -> OBC` SHALL remain disabled or outside the verdict boundary

#### Scenario: UHF backup ingress keeps COMM services unchanged
- **WHEN** UHF backup command ingress traverses COMM
- **THEN** UHF SHALL use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add command/downlink service ports or alter the existing request/reply wire layouts

### Requirement: UHF Node-6 Beacon Side Channel
The comm subsystem SHALL support bounded BeaconV1 emission through `uhf_comm_csp_node` node `6` as a UHF link-local beacon side channel that remains distinct from command/downlink chunk services.

#### Scenario: Beacon path traverses UHF node 6
- **WHEN** the hosted UHF beacon probe runs
- **THEN** BeaconV1 frames SHALL be sent from hosted OBC to `uhf_comm_csp_node` node `6`
- **AND** `uhf_comm_csp_node` SHALL emit the beacon frame on the configured hosted beacon serial endpoint for capture

#### Scenario: Beacon side channel does not alter command services
- **WHEN** UHF BeaconV1 frames are emitted
- **THEN** services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS` SHALL remain unchanged
- **AND** beacon emission SHALL NOT require ground acknowledgement, command response semantics, file/downlink behavior, reliable transfer, CCSDS framing, or replacement of `ComFprime`

#### Scenario: UHF v1 claims stay bounded
- **WHEN** UHF UART backup evidence is recorded
- **THEN** the verdict SHALL NOT claim full UHF command authority, failover policy, arbitrary file downlink, S-band TCP behavior, direct GDS TCP behavior, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation

### Requirement: Hosted S-band CCSDS Is The Default OBC Ground Path
The comm subsystem SHALL treat the default hosted `OBC` deployment as the
governed S-band CCSDS ground path. Old `ComFprime` behavior SHALL remain
reviewable only through archived historical evidence, not through a maintained
legacy OBC deployment or regression script surface.

#### Scenario: Default hosted OBC uses CCSDS S-band
- **WHEN** the default hosted `OBC` deployment is built or run
- **THEN** it SHALL use `ComCcsds` for the ground communication topology
- **AND** it SHALL expose the default command, event, telemetry, and file
  namespace as `OBCApp`
- **AND** it SHALL use S-band COMM node `5` when exercising the hosted
  gateway-backed path.

#### Scenario: Legacy ComFprime path is historical only
- **WHEN** old hosted `ComFprime` behavior is cited
- **THEN** it SHALL be described as archived historical evidence rather than as
  a maintained regression deployment
- **AND** no current script, operator guide, or verification inventory entry
  SHALL require `OBC_ComFprimeLegacy`.

#### Scenario: Existing baselines stay scoped
- **WHEN** S-band node `5` or UHF node `6` `ComFprime` baseline records are
  cited
- **THEN** they SHALL be described as stock `ComFprime` gateway baselines or
  historical records as appropriate
- **AND** they SHALL NOT be described as active CCSDS adoption evidence or as
  maintained legacy Top runtime coverage.

### Requirement: CCSDS Hosted Adoption Uses S-band Node 5 Plus Active UHF Node 6
The comm subsystem SHALL treat the default hosted CCSDS baseline as S-band node `5` plus a separate active UHF node `6` CCSDS path, with distinct bounded evidence and unchanged shared COMM service semantics.

#### Scenario: S-band adoption uses hosted COMM CSP
- **WHEN** the CCSDS hosted adoption proof runs
- **THEN** hosted OBC SHALL run with `GROUND_LINK_MODE=comm-csp`
- **AND** it SHALL use `COMM_CSP_NODE=5`
- **AND** direct `GDS -> TCP -> OBC` SHALL remain disabled or outside the verdict boundary

#### Scenario: COMM service contract remains unchanged
- **WHEN** CCSDS traffic traverses the COMM path
- **THEN** S-band SHALL continue to use services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT alter existing COMM service ports, node IDs, or request/reply wire layouts

#### Scenario: Active UHF CCSDS stays distinct from S-band adoption
- **WHEN** CCSDS hosted adoption results are recorded
- **THEN** S-band node `5` and active UHF node `6` SHALL be recorded as separate bounded evidence surfaces
- **AND** historical UHF `ComFprime` records SHALL remain distinct from the active UHF CCSDS adoption evidence

### Requirement: CCSDS Adoption Covers Bounded TT&C, File Downlink, And Decoded Framing
The comm subsystem SHALL prove default hosted S-band CCSDS adoption with bounded command, event, telemetry, bounded file/downlink, gateway raw-byte compatibility, and decoded CCSDS frame/APID observations.

#### Scenario: Bounded CCSDS adoption proof covers required behavior
- **WHEN** the CCSDS hosted adoption proof passes
- **THEN** it SHALL include command uplink, event downlink, telemetry downlink, `DpCatalog`-driven `.fdp` file downlink, gateway raw-byte compatibility, and decoded frame/APID observations
- **AND** it SHALL identify the default hosted `OBC` deployment rather than a spike-only executable

#### Scenario: Active UHF CCSDS adoption proof stays bounded
- **WHEN** active hosted UHF CCSDS adoption evidence is recorded
- **THEN** it SHALL include bounded command uplink, event downlink, telemetry downlink, bounded file/downlink, and decoded frame/APID observations on node `6`
- **AND** it SHALL record `SCID 0x44`, `VCID 2`, and TM frame size `1024`

#### Scenario: Adjacent futures stay out of the verdict
- **WHEN** CCSDS adoption evidence is recorded
- **THEN** the verdict SHALL NOT claim RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, command authority expansion beyond the existing COMM/runtime policies, pass scheduling, or arbitrary onboard file downlink

### Requirement: COMM-Owned Operational Link Roles

The comm subsystem SHALL own explicit runtime roles for primary command, primary telemetry, and primary file-transfer links, together with per-link availability and reviewable transition state for the active baseline.

#### Scenario: Default startup roles are explicit

- **WHEN** the active hosted `TopCcsds` baseline starts
- **THEN** COMM SHALL report `S-band` as the primary command, telemetry, and file-transfer link
- **AND** it SHALL report UHF as the bounded backup command path when UHF ingress is configured and available

#### Scenario: Operator switches the primary link

- **WHEN** the operator executes the governed COMM primary-link switch command
- **THEN** COMM SHALL update primary command, telemetry, and file-transfer roles together
- **AND** it SHALL emit reviewable events or telemetry reflecting the new primary-link state

### Requirement: COMM Drives Authenticated Ingress Policy By Link Role

The comm subsystem SHALL drive runtime authenticated command policy for each active ingress path according to the current COMM link role while leaving authenticated envelope verification, session-open lifecycle, and strict-monotonic sequence ownership inside `CommandIngressAuthority`.

#### Scenario: S-band primary allows full authenticated command set

- **WHEN** an authenticated command arrives through the S-band ingress while COMM marks S-band as primary
- **THEN** the command SHALL be evaluated against the full catalog policy rather than the restricted UHF-backup policy

#### Scenario: UHF backup stays low-risk

- **WHEN** an authenticated command arrives through the UHF ingress while COMM marks UHF as backup
- **THEN** COMM-driven policy SHALL allow only bounded read/status or explicitly allowlisted low-risk commands
- **AND** it SHALL reject higher-risk mode, config, reset, power, ADCS, COMM-control, file-transfer, and data-product control commands

#### Scenario: UHF primary gains full authenticated authority

- **WHEN** COMM marks UHF as primary
- **THEN** authenticated commands arriving through the UHF ingress SHALL be evaluated against the full catalog policy, including governed data-product and file-transfer commands

### Requirement: Observe-Only Pass State

The comm subsystem SHALL continue to publish pass-active and pass-end state for operator visibility, but pass state SHALL NOT by itself admit or deny authenticated commands or new file-transfer ownership on the active baseline.

#### Scenario: Pass transition is observable only

- **WHEN** a governed COMM pass starts or stops
- **THEN** COMM SHALL update reviewable pass events, telemetry, or counters
- **AND** the authenticated command and downlink admission policy SHALL continue to be driven by link availability and primary-link role rather than pass state alone

### Requirement: Shared File-Downlink Ownership Is COMM-Owned

The comm subsystem SHALL own the shared file/downlink policy surface used by official data products on the active baseline.

#### Scenario: DP catalog is the active owner

- **WHEN** a `DpCatalog` file/downlink request is accepted
- **THEN** COMM SHALL record `DpCatalog` as the active shared downlink owner until completion, cancellation, or owner drop

#### Scenario: Additional DP requests are busy-rejected during an active transfer

- **WHEN** a second `DpCatalog` downlink request arrives while COMM already owns an active shared downlink surface for `DpCatalog`
- **THEN** COMM SHALL reject the new request as busy instead of bypassing, preempting, or queueing an extra file-transfer owner

### Requirement: COMM Converges State On Link Loss Or Primary Switch
The comm subsystem SHALL converge authenticated-session policy and shared downlink ownership state when the current primary link becomes unavailable or the primary-link role changes, but fault-driven convergence SHALL now be performed through the shared recovery owner.

#### Scenario: Fault-driven link loss uses the shared recovery owner
- **WHEN** the current primary COMM link becomes unavailable because of a shared-recovery COMM fault
- **THEN** the active runtime SHALL converge session and shared downlink-owner state through executor-owned COMM recovery actuation
- **AND** `CommController` SHALL leave direct detector-side failover and direct detector-side session revoke out of scope for that fault transition

#### Scenario: Fault-driven failover does not auto-restore nominal S-band
- **WHEN** executor-owned COMM recovery fails over away from the nominal primary link because of a fault
- **THEN** the active runtime SHALL leave later nominal-link restoration to a separate governed operator or future recovery action
- **AND** it SHALL NOT auto-restore S-band merely because S-band later becomes healthy again

### Requirement: COMM Fault Detection Is Reviewable And Bounded
The comm subsystem SHALL detect bounded current-primary link failure for the shared recovery path while keeping operator role policy separate from fault policy.

#### Scenario: COMM detector distinguishes policy events from fault events
- **WHEN** an operator switches the primary link or pass state starts or stops
- **THEN** COMM SHALL continue to treat those transitions as policy or observability events
- **AND** it SHALL NOT treat them by themselves as shared-recovery COMM fault incidents

#### Scenario: COMM fault thresholds are configuration-driven
- **WHEN** the active baseline evaluates the current primary COMM link during scheduled ticks
- **THEN** it SHALL latch `COMM_PRIMARY_UNAVAILABLE` after the configured `unavailableFailureThreshold` consecutive scheduled detector cycles where that primary link is unavailable
- **AND** it SHALL latch `COMM_PRIMARY_TRANSPORT` after the configured `unavailableFailureThreshold` consecutive scheduled detector cycles where the primary link remains available but its cumulative `tx/rx` error total grows
- **AND** it SHALL clear the active COMM detector fault on the first scheduled cycle where the primary link is available and has no new `tx/rx` error growth

### Requirement: Fault-Driven COMM Actuation Is Executor-Owned
The comm subsystem SHALL route fault-driven primary-link switching, session revoke, and shared downlink-owner clear through the shared recovery owner instead of performing them in the detector path.

#### Scenario: Detector-side link loss does not switch primary roles directly
- **WHEN** COMM detects a current-primary fault in this change
- **THEN** `CommController` SHALL emit reviewable fault or clear truth
- **AND** it SHALL NOT directly switch the primary link set, revoke the current primary command session, or clear downlink owners from the detector-side fault transition alone

#### Scenario: Executor-owned failover returns structured truth
- **WHEN** the shared recovery owner requests bounded COMM recovery actuation
- **THEN** `CommController` SHALL return structured failover truth including whether a switch occurred, whether the runtime was already on a healthy primary, whether no healthy backup existed, how many sessions were revoked, how many owners were cleared, and the final primary command, telemetry, and file links

### Requirement: Active CCSDS File Ingress Has Repo-Owned Sequence-Staging Governance

The active CCSDS file-uplink path SHALL be governed by a repo-owned owner before
packets reach `Svc::FileUplink`, and staged upload admission SHALL now depend
on both secure auth state and current runtime file-role policy.

#### Scenario: START packet requires valid path, active auth, and allowed role
- **WHEN** the active `ComCcsds` or `OBCComCcsds` file ingress receives a
  `Fw::FilePacket` `START`
- **THEN** the repo-owned file-ingress owner SHALL still reject absolute paths,
  `..`, non-allowlisted logical destinations, and physical symlink escape
- **AND** it SHALL also require active secure auth on that ingress/service
- **AND** it SHALL also require current runtime file-role policy to allow file
  upload on that ingress
- **AND** only then SHALL it rewrite the accepted logical
  `.sequence-staging/<leaf>` destination to the configured physical staging
  root before forwarding to `FileUplink`.

#### Scenario: UHF backup remains deny-only for staged upload
- **WHEN** UHF secure auth is active while runtime role remains `BACKUP`
- **THEN** staged file upload SHALL still be denied on that ingress
- **AND** accepted secure read/status continuity SHALL NOT by itself reopen
  staged upload authority.

#### Scenario: Explicit-switched UHF primary may upload after re-auth
- **WHEN** runtime role switches to `uhf-primary-after-failover`
- **AND** the prior UHF auth state has been revoked and a new valid UHF secure
  auth cycle completes
- **THEN** the active UHF CCSDS file path MAY admit `.sequence-staging/<leaf>`
  upload under the same bounded sequence-staging rules as S-band.

#### Scenario: Mid-transfer invalidation fails closed
- **WHEN** a staged transfer `START` was previously accepted
- **AND** secure auth times out, is revoked, or current runtime file-role
  policy later becomes deny
- **THEN** subsequent `DATA`, `END`, and `CANCEL` packets for that transfer
  SHALL be dropped until a new valid `START` is accepted after re-auth.

### Requirement: Comm-Managed Unknown Uplink Is Handshake-Only And Fail-Closed

The active comm-managed unknown uplink surface SHALL admit only handshake APID
`0x00FE` traffic owned by `SecureLinkAuthorizer` and SHALL reject any other
unknown uplink packet without mutating auth/session state.

#### Scenario: Malformed or unsupported handshake uplink is rejected without state mutation
- **WHEN** `SecureLinkAuthorizer` receives malformed handshake bytes, an
  unsupported `serviceId`, or an unexpected handshake message type on uplink
- **THEN** it SHALL reject and return the packet
- **AND** it SHALL leave pending challenge, active auth, and timeout state
  unchanged.

#### Scenario: Other unknown uplink families are not admitted
- **WHEN** any non-handshake unknown packet reaches the current comm-managed
  unknown uplink route
- **THEN** the packet SHALL be rejected and returned by the handshake owner
- **AND** the repository SHALL treat APID `0x00FE` handshake traffic as the
  only current admitted unknown-uplink family on that route.

### Requirement: File-Ingress Governance Claim Stays Bounded To Active Comm-Managed Paths

This change SHALL keep file-ingress governance claims bounded to the current
comm-managed S-band path and explicit-switched UHF failover-primary path.

#### Scenario: Docs do not over-claim generic file-uplink closure
- **WHEN** this change is documented or evidenced
- **THEN** it SHALL be valid to claim staged file-uplink closure for active
  S-band and re-authenticated `uhf-primary-after-failover`
- **AND** it SHALL still NOT claim generic arbitrary-file authority, backup UHF
  file admission, CFDP redesign, reliable-transfer redesign, or target proof.

### Requirement: Ground Link Driver Stays A Byte/Backend Owner

The comm subsystem SHALL keep `GroundLinkDriver` focused on backend configuration, runtime lifecycle, byte send/receive, buffer ownership, and low-level counters/events/telemetry rather than treating it as the public mission link-state authority.

#### Scenario: Driver counters remain reviewable without owning mission policy

- **WHEN** `GroundLinkDriver` publishes TX/RX byte, chunk, or error telemetry
- **THEN** that telemetry SHALL remain the low-level driver review surface
- **AND** primary-link availability and stale-activity policy SHALL be derived elsewhere

### Requirement: Active COMM Policy Uses A Provider-Owned Link-Health Surface

The comm subsystem SHALL compute S-band and UHF ground-link health for active COMM runtime policy through a dedicated provider surface rather than by having `CommController` read raw driver stats directly.

#### Scenario: Provider runs before COMM policy

- **WHEN** the active hosted `TopCcsds` fast rate group runs
- **THEN** the ground-link health provider SHALL update per-band health views before `CommController` evaluates availability and transport-fault state

#### Scenario: COMM reads provider health views

- **WHEN** `CommController` evaluates primary-link availability or transport-fault growth
- **THEN** it SHALL consume the provider-owned per-band health view
- **AND** it SHALL NOT treat direct `GroundLinkDriver` stat polling as the authoritative mission link-state input

### Requirement: `COMM_CSP` Activity Freshness Is Reviewable

The comm subsystem SHALL define `COMM_CSP` link activity freshness for the active S-band node `5` and UHF node `6` policy paths from successful RX, successful TX, or successful status observation.

#### Scenario: Successful status observation keeps an idle COMM path healthy

- **WHEN** the active `COMM_CSP` link has no new TX or RX payload for one fast-group cycle
- **AND** the provider observes a successful link-status result during that cycle
- **THEN** the link activity age SHALL be refreshed
- **AND** the path SHALL remain available if the link is still connected

#### Scenario: Connected but stale COMM path becomes unavailable

- **WHEN** an active `COMM_CSP` path remains connected
- **BUT** it has activity age greater than one fast-group tick because RX, TX, and status observation all failed to refresh activity
- **THEN** the provider SHALL mark that path unavailable
- **AND** the reviewable reason SHALL distinguish stale activity from explicit disconnection

### Requirement: Direct TCP Remains A Connected-Only Fallback

The comm subsystem SHALL keep direct `OBC -> GDS` TCP as a connected-only development fallback instead of introducing a new keepalive protocol in this change.

#### Scenario: Idle direct TCP does not become stale only due to silence

- **WHEN** the direct TCP path remains socket-connected with no recent TX or RX payloads
- **THEN** the provider SHALL continue to treat the path as available
- **AND** it SHALL report the connected-only fallback reason rather than a stale-activity verdict

### Requirement: COMM Runtime State Exposes Per-Band Activity Age And Reasons
The comm subsystem SHALL expose per-band ground-link activity age and
availability reason through reviewable COMM runtime state on the active
baseline, and it SHALL keep reviewable raw ground-link observation and cached
radio observation available through separate runtime contracts. Radio
signal-quality metrics such as RSSI are intentionally limited in the current
baseline to raw `RadioController` observation with explicit freshness and
unavailable-value semantics, while SNR and any signal-quality contribution to
link-health policy remain deferred.

#### Scenario: Hosted status shows new health fields
- **WHEN** operators inspect hosted COMM runtime status
- **THEN** they SHALL be able to see S-band and UHF activity-age values
- **AND** they SHALL be able to distinguish healthy activity, connected-only fallback, explicit disconnect, and stale-activity reasons

#### Scenario: Radio signal metrics remain deferred
- **WHEN** reviewers inspect the v1 `GroundLinkHealthProvider` surface
- **THEN** RSSI and SNR SHALL NOT be required fields in the current `CommLinkHealthView`
- **AND** any future SNR or signal-quality integration into provider or COMM policy SHALL be specified by a later governed change that defines source ownership, units, freshness, and unavailable-value semantics

### Requirement: Raw Ground-Link Observation Fields Are Reviewable

The comm subsystem SHALL expose a complete raw ground-link observation contract
from `GroundLinkDriver` for each active band on the current baseline.

#### Scenario: Raw ground-link observation includes byte counters
- **WHEN** code or reviewable runtime status reads a band-specific ground-link
  observation
- **THEN** it SHALL include `mode`, `healthSemantics`, `connected`,
  `txChunks`, `rxChunks`, `txBytes`, `rxBytes`, `txErrors`, `rxErrors`, and
  `successfulStatusObservations`

### Requirement: Radio Observation Freshness Is Reviewable

The comm subsystem SHALL expose cached radio observation freshness and
unavailable-result semantics separately from derived link health.

#### Scenario: Cached radio observation reports age and last result
- **WHEN** operators inspect the current radio observability surface
- **THEN** they SHALL be able to see whether a cached radio sample exists
- **AND** they SHALL be able to see the cached-sample age in fast-group ticks
- **AND** they SHALL be able to distinguish successful observation from timeout,
  transport error, invalid response, no-sample, or unsupported-observation
  result

### Requirement: Hosted Runtime Status Separates Raw Observation From Policy

The comm subsystem SHALL keep hosted runtime status readback explicit about raw
ground-link observation, derived health, policy-facing COMM runtime state, and
cached radio observation.

#### Scenario: Hosted runtime status does not collapse link layers
- **WHEN** hosted runtime status is printed for the active baseline
- **THEN** it SHALL show per-band raw ground-link observation and cached radio
  observation separately from `CommRuntimeState`
- **AND** it SHALL NOT present raw RSSI or raw transport counters as if they
  were provider-owned availability verdicts

### Requirement: Ground Link Observation Contract Is OBC-Owned

The comm subsystem SHALL keep provider-facing ground-link observation types in the OBC-owned runtime surface rather than in the simulator/backend-owned implementation header, and that OBC-owned contract SHALL be the complete raw ground-link observability source for current runtime readback.

#### Scenario: Provider consumes OBC-owned observation types

- **WHEN** `GroundLinkHealthProvider` reads a ground-link observation
- **THEN** the observation type SHALL come from the OBC-owned ground-link runtime contract
- **AND** simulator/backend code SHALL only implement and populate that contract
- **AND** the provider-facing observation contract SHALL NOT expose raw CSP target-node identity

### Requirement: Link Health Semantics Are Explicit

The comm subsystem SHALL publish explicit ground-link health semantics with each observation so the provider does not infer active mission policy from raw backend node identifiers.

#### Scenario: Backend publishes configured semantics

- **WHEN** a COMM CSP backend publishes a ground-link observation
- **THEN** the observation SHALL use the health semantics supplied by runtime/topology configuration
- **AND** the backend SHALL NOT re-infer provider policy from raw node ID during observation publication

#### Scenario: Active COMM CSP nodes use active semantics

- **WHEN** a COMM CSP backend represents S-band node `5` or UHF node `6`
- **THEN** its observation SHALL use active COMM CSP health semantics
- **AND** the provider SHALL apply active link activity freshness and transport-growth policy

#### Scenario: Generic node 4 uses connected-only compatibility semantics

- **WHEN** a COMM CSP backend represents generic compatibility node `4`
- **THEN** its observation SHALL use connected-only fallback health semantics
- **AND** the provider SHALL NOT mark idle silence as stale activity
- **AND** the provider SHALL NOT report transport-growth faults from connected-only fallback observations

#### Scenario: Connected-only fallback remains available by connection state

- **WHEN** a connected direct-TCP or node `4` compatibility observation is evaluated
- **THEN** the provider SHALL report the link available from connection state
- **AND** the availability reason SHALL identify connected-only fallback

#### Scenario: Disabled observations are unavailable

- **WHEN** a disabled/no-backend observation is evaluated
- **THEN** the provider SHALL report the link unavailable

#### Scenario: Unsupported COMM CSP nodes do not become fallback links

- **WHEN** a COMM CSP backend represents a node other than generic node `4`, S-band node `5`, or UHF node `6`
- **THEN** its observation SHALL use disabled health semantics
- **AND** the provider SHALL NOT report connected-only fallback availability

### Requirement: CommController Has No Driver Type Coupling

The comm subsystem SHALL keep `CommController` policy evaluation dependent on the ground-link health provider rather than on direct `GroundLinkDriver` pointers, and the COMM-owned overflow-adjacent observability surface SHALL stay limited to owner, pending-owner, reject-count, and existing FDIR/runtime state rather than copying `ComCcsds` queue internals into `CommController`.

#### Scenario: COMM policy evaluates provider state only

- **WHEN** `CommController` is configured for runtime operation
- **THEN** its ground-link policy dependency SHALL be the health provider surface
- **AND** it SHALL NOT store direct S-band or UHF `GroundLinkDriver` pointers
- **AND** queue depth, queue overflow, and other `ComCcsds` queue-owned fields
  SHALL remain outside the `CommController` runtime contract

### Requirement: Node-5 COMM CSP Downlink V2 Stages Before Drain
The comm subsystem SHALL retain the node-`5`-only downlink transport `v2`
below stock `FileDownlink` as a bounded staging/commit/drain implementation
and regression reference, even though the current official runtime selection
uses `v3 -> v1` rather than `v3 -> v2 -> v1`.

#### Scenario: Node-5 v2 stages and commits one stream atomically
- **WHEN** target OBC sends one file/downlink buffer through the `v2` path
- **THEN** non-final chunks SHALL enter a bounded private staging stream
- **AND** the final chunk SHALL commit the full stream atomically into the
  shared drain queue before reporting success upstream

#### Scenario: Node-5 v2 retry, rejection, abort, and failure stay bounded
- **WHEN** final commit lacks credit, malformed traffic arrives, a stream is
  aborted/times out, or the external link fails
- **THEN** retry SHALL be limited to the final uncommitted chunk when possible
- **AND** invalid traffic SHALL NOT mutate staging or queue accounting
- **AND** abort/timeout SHALL clear only uncommitted staging
- **AND** external-link failure SHALL purge committed-undrained bytes and
  account them as dropped

### Requirement: Node-5 COMM CSP Downlink V2 Preserves Stock Upper-Layer Ownership
The comm subsystem SHALL keep `DpCatalog -> CommController -> FileDownlink`
as the official selecting, scheduling, and file-packet owner chain while the
retained `v2` transport operates below that boundary.

#### Scenario: Retained v2 falls back without widening scope
- **WHEN** a node-`5` `v2` status probe fails or returns an invalid reply
- **THEN** the backend SHALL emit a bounded fallback diagnostic and use the
  existing `v1` `DOWNLINK_WRITE` transport
- **AND** it SHALL NOT widen the behavior to other COMM nodes

### Requirement: Node-5 COMM CSP Downlink V3 Uses Bulk Data Frames Below GroundLinkDriver
The comm subsystem SHALL provide a node-`5`-only `v3` transport below
`GroundLinkDriver` using one-way bulk data frames, bounded progress polling,
resend-from-contiguous-progress, and atomic commit-to-drain acceptance.

#### Scenario: V3 acceptance is queue-backed rather than flush-backed
- **WHEN** the OBC sends one serialized upper-layer buffer through node `5`
- **THEN** node `5` SHALL report success only after the full buffer is
  committed into its bounded drain queue
- **AND** it SHALL NOT wait for external-link flush completion

#### Scenario: V3 retries remain idempotent and bounded
- **WHEN** ACK progress stalls or `BEGIN`, data, or `COMMIT` is retried
- **THEN** the sender SHALL resume from the reported contiguous boundary
- **AND** duplicate begin/data/commit handling SHALL NOT append duplicate bytes
- **AND** disconnect or drain failure SHALL purge staged/committed queue state
  and account committed-undrained bytes as dropped

### Requirement: Node-5 COMM CSP Downlink V3 Preserves Stock Official File Ownership
The comm subsystem SHALL keep `DpCatalog -> CommController -> FileDownlink ->
CommEgressMux -> GroundLinkDriver` as the official owner chain and SHALL use
`v3` strictly below `GroundLinkDriver.send()`.

#### Scenario: Node-5 backend falls back directly from v3 to v1
- **WHEN** the node-`5` V3 status probe fails or returns an invalid reply
- **THEN** the backend SHALL use the existing `v1` `DOWNLINK_WRITE` path
- **AND** it SHALL NOT require `v2` as an intermediate runtime fallback

### Requirement: Service-Managed Target S-band Uses Node 5
The comm subsystem SHALL support a service-managed target/lab S-band path where
`subsystem.local` hosts `sband_comm_csp_node` as COMM node `5` and `obc.local`
uses that path as the default active COMM profile.

#### Scenario: Target S-band service exposes node-5 v3 downlink services
- **WHEN** the target/lab S-band COMM profile is installed and enabled
- **THEN** `subsystem.local` SHALL run `sband_comm_csp_node` as COMM node `5`
- **AND** it SHALL expose services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`,
  `32 LINK_STATUS`, `34 DOWNLINK_STAGE_V2`, `35 DOWNLINK_STATUS_V2`,
  `36 DOWNLINK_ABORT_V2`, `37 RELIABLE_TRANSFER_DATA`,
  `38 RELIABLE_TRANSFER_CONTROL`, `39 DOWNLINK_CONTROL_V3`, and
  `40 DOWNLINK_DATA_V3`

#### Scenario: Governed target baseline scopes CAN FD to COMM V3 traffic
- **WHEN** the maintained target/lab COMM baseline is prepared
- **THEN** A SHALL enable SocketCAN CAN FD on OBC, S-band node `5`, and UHF
  node `6` only for destinations `5,6` and destination port `40`
- **AND** C SHALL only verify and consume that profile
- **AND** EPS/ADCS SHALL NOT receive the COMM CAN FD override
- **AND** this SHALL NOT imply BRS or measured data-phase-rate closure

#### Scenario: Target OBC default path uses node 5
- **WHEN** the target/lab default COMM profile runs
- **THEN** target OBC SHALL run with `GROUND_LINK_MODE=comm-csp`
- **AND** it SHALL use `COMM_CSP_NODE=5`

### Requirement: Service-Managed Target UHF Uses Node 6
The comm subsystem SHALL support a service-managed target/lab UHF path where `subsystem.local` hosts `uhf_comm_csp_node` as COMM node `6` on the physical serial ingress while preserving the existing COMM service contract.

#### Scenario: Target UHF service owns node 6
- **WHEN** the target/lab UHF COMM profile is installed and enabled
- **THEN** `subsystem.local` SHALL run `uhf_comm_csp_node` as COMM node `6`
- **AND** it SHALL use the configured lab serial ingress device and baudrate plus the existing COMM services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`

#### Scenario: Target UHF profiles select authority role without changing node identity
- **WHEN** target/lab UHF runs as operator profile `uhf-primary` or
  `uhf-backup`
- **THEN** both profiles SHALL keep `COMM_CSP_NODE=6`
- **AND** installed target OBC bootstrap semantics SHALL keep `COMMAND_AUTHORITY_PROFILE=sband-primary`
- **AND** operator profile `uhf-primary` SHALL exercise node-`6` ingress only
  after explicit switch from the default node-`5` path
- **AND** that switched role SHALL be the current
  `uhf-primary-after-failover` runtime policy role
- **AND** `uhf-backup` SHALL exercise node-`6` ingress as bounded backup authority on ingress `1`

### Requirement: Quiet-Mode Node 6 Proof Stays Bounded
The comm subsystem SHALL keep target node-`6` proof bounded to quiet-mode operational validation until general non-quiet serial stability is separately proven.

#### Scenario: Node 6 proof uses probe-owned quiet mode
- **WHEN** target node-`6` command/readback validation is recorded
- **THEN** the proof SHALL be allowed to use the existing probe-owned quiet-egress override
- **AND** the verdict SHALL NOT claim general non-quiet serial/background-TM closure

### Requirement: Target/Lab COMM Primary Unavailable Uses Subsystem Responsiveness
The comm subsystem SHALL treat target/lab `COMM_PRIMARY_UNAVAILABLE` as repeated no-response from the current primary COMM subsystem stand-in, not as absence of an attached ground gateway.

#### Scenario: Node 5 S-band default path ignores detached ground tooling
- **WHEN** target/lab runs with node `5` as the default active COMM path
- **AND** the local probe-owned `ground_ttc_gateway` is stopped or absent
- **AND** OBC can still successfully reach node `5` through bounded internal CSP ping
- **THEN** the active runtime SHALL keep the primary COMM path available
- **AND** it SHALL NOT latch `COMM_PRIMARY_UNAVAILABLE` from gateway detachment alone

#### Scenario: Repeated node 5 or node 6 CSP ping failure latches unavailable
- **WHEN** the current primary target/lab COMM profile selects node `5` or node `6`
- **AND** OBC observes the configured `unavailableFailureThreshold` consecutive scheduled internal CSP ping failures to that selected node
- **THEN** it SHALL latch `COMM_PRIMARY_UNAVAILABLE`
- **AND** the first scheduled successful responsiveness check SHALL clear that fault

#### Scenario: Ground-link transport observability stays separate
- **WHEN** target/lab evaluates COMM detector truth for node `5` or node `6`
- **THEN** ground-facing `GROUND_LINK_UP/DOWN` and transport-error growth SHALL remain reviewable observability
- **AND** those signals SHALL NOT redefine `COMM_PRIMARY_UNAVAILABLE` back into a ground-station-attachment fault
- **AND** those signals SHALL NOT by themselves submit reboot-class target/lab COMM FDIR while the primary subsystem responsiveness check remains healthy

### Requirement: Node-5 Residual Observability Buckets Stay Explicit

The comm subsystem SHALL distinguish node-`5` post-auth pass-time keep-live
truth, reviewable transport/policy proof observability, bounded fresh
readback, and diagnostics-only residual live chatter instead of treating the
entire residual surface as one undifferentiated `non-baseline live` bucket.

#### Scenario: Resource truth does not fall back to SystemResources
- **WHEN** current docs or runbooks describe node-`5` post-auth resource truth
- **THEN** they SHALL use `WatchdogSupervisor` `SYS_*` surfaces as the formal
  resource keep-live truth
- **AND** they SHALL treat `SystemResources.*` as supplemental diagnostics-only
  live telemetry rather than pass-time operator truth.

#### Scenario: Transport reviewable surfaces remain formal without becoming keep-live summary
- **WHEN** current docs, runbooks, or verification artifacts describe node-`5`
  transport facts
- **THEN** `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`, transport-error
  growth, and `GROUND_LINK_HEALTH_S_BAND_*` SHALL remain formal reviewable
  observability
- **AND** those surfaces SHALL NOT be reclassified as the pass-time keep-live
  summary.

#### Scenario: Queue and owner residuals are not flattened into one label
- **WHEN** current docs or evidence describe node-`5` queue and owner
  residuals
- **THEN** `QueueOverflow`, `CSP_OWNER_TIMEOUT`, and
  `CSP_OWNER_TOTAL_TIMEOUTS` SHALL remain formal reviewable proof surfaces
- **AND** queue-depth, owner-success, UART, and similar low-level counters
  SHALL remain diagnostics-only unless a later governed change promotes them

#### Scenario: Detailed GET readback drift is resolved inside the same governed node-5 cleanup
- **WHEN** the representative authenticated detailed `GET_*` readback proof
  drifts while the residual-governance cleanup is still active
- **THEN** the same governed change SHALL diagnose whether the drift is in the
  proof oracle, packetized delivery path, or component-owned summary/detail
  split
- **AND** it SHALL repair that drift without widening the contract into a
  second command plane or a generic telemetry-schema rewrite.

### Requirement: Node-5 COMM Policy Truth Stays Separate From Raw Transport And COMM Internals

The comm subsystem SHALL keep current node-`5` operator-facing policy truth,
reviewable policy observability, and diagnostics-only COMM internals as
separate layers.

#### Scenario: Policy-facing state remains formal pass-time truth
- **WHEN** current docs or proofs describe node-`5` live operator truth
- **THEN** `COMM_BAND_SWITCH`, `COMM_PRIMARY_LINK_CHANGED`,
  `COMM_LINK_AVAILABILITY_CHANGED`, `COMM_DOWNLINK_STATE_CHANGED`,
  `COMM_RECOVERY_FAILOVER_RESULT`, and
  `COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED` SHALL remain formal pass-time
  truth
- **AND** current primary-band state surfaces SHALL remain aligned with that
  operator-facing truth.

#### Scenario: Supporting counters stay reviewable without promoting reliable-transfer internals
- **WHEN** current docs or proofs describe node-`5` supporting COMM counters
- **THEN** availability, activity-age, downlink-owner, and failover counters
  SHALL remain reviewable policy observability
- **AND** reliable-transfer internals, pass counters, and unrelated UHF
  suppress internals SHALL remain diagnostics-only for this node-`5` cleanup.

### Requirement: Current COMM Policy Keeps Transport Facts, Runtime Policy, And Future Reliable Transfer Separate

The comm subsystem SHALL describe the current baseline using three explicit
layers: raw observation or transport fact, session or link-role runtime policy,
and future reliable-transfer behavior.

#### Scenario: Current COMM wording does not collapse adjacent layers
- **WHEN** reviewers inspect current COMM architecture, runbooks, or interface
  wording
- **THEN** raw `GroundLinkDriver`, `UartDriver`, and `RadioController`
  observations SHALL remain transport or observation facts
- **AND** `CommandIngressAuthority` and `CommController` decisions SHALL remain
  runtime policy rather than raw transport truth
- **AND** reliable transfer, ARQ, NACK, packet retry, or CFDP behavior SHALL
  remain future scope unless separate governed evidence proves it

### Requirement: Secure Auth Completion Is The Preferred UHF Operational Session Boundary

The comm subsystem SHALL make accepted UHF authorization completion for the
UHF secure service the only current maintained UHF operational
command-session boundary, and any retained legacy `SESSION_OPEN(seq0)` wording
SHALL be historical compatibility ancestry rather than an accepted current
runtime boundary.

#### Scenario: Link acquisition alone is not a secure command-session boundary
- **WHEN** the UHF link is electrically present, bytes are exchanged, or a
  first non-lifecycle command succeeds on an adjacent path
- **THEN** the repository SHALL NOT describe that fact alone as the current
  UHF operational secure command-session boundary.

#### Scenario: UHF auth success is the maintained secure boundary
- **WHEN** `SecureLinkAuthorizer` accepts a valid `RESPONSE` for
  `ServiceID = 2` on the relevant comm-managed ingress
- **AND** `CommandIngressAuthority` synthesizes the runtime opened-session
  state for that auth grant
- **THEN** the repository SHALL treat that auth completion as the current UHF
  operational secure command-session boundary for the maintained path.

#### Scenario: Legacy SESSION_OPEN is not current UHF boundary truth
- **WHEN** current COMM docs, probes, or operator wording mention
  `SESSION_OPEN(seq0)`
- **THEN** they SHALL describe it only as historical compatibility ancestry
- **AND** they SHALL NOT require it as the current operational UHF
  command-session boundary.

### Requirement: Beacon Suppress And Resume Policy Belongs To CommController

The current COMM runtime SHALL assign UHF beacon suppress and resume ownership
to `CommController`, SHALL start suppress only after accepted UHF secure
authorization completion synthesizes the opened-session state for the owning
session, SHALL refresh the suppress window only on later accepted
authenticated UHF secure command activity for that same active session, and
SHALL resume beaconing only after immediate session invalidation or a bounded
inactivity timeout.

#### Scenario: Suppress start requires accepted UHF auth completion
- **WHEN** raw UHF link acquisition, unrelated path traffic, invalid traffic,
  rejected traffic, or any non-accepted handshake exchange occurs
- **THEN** the runtime SHALL NOT enter suppress because of that traffic alone
- **AND** the suppress-start boundary SHALL remain accepted UHF auth
  completion for the qualifying active secure session.

#### Scenario: Accepted UHF secure command activity refreshes the active window
- **WHEN** the owning UHF secure session is already suppressing beacon
  emission
- **AND** a later accepted authenticated UHF secure command in that same
  active session succeeds, including read or status traffic
- **THEN** `CommController` SHALL refresh the bounded inactivity window for
  the owning session.

#### Scenario: Role invalidation clears suppress immediately
- **WHEN** the owning UHF secure session is invalidated by role switch or COMM
  failover handling
- **THEN** `CommController` SHALL clear suppress immediately
- **AND** the runtime SHALL require a new successful UHF authorization cycle
  before suppress may start again.

### Requirement: Current COMM Relay Policy Is Single-Path With Explicit Switch

The current comm runtime policy SHALL treat S-band node `5` as the default
active path, SHALL treat UHF node `6` as either bounded `uhf-backup` ingress or
explicit-switch `uhf-primary-after-failover`, and SHALL NOT claim simultaneous
routed S-band and UHF relay ownership on one active baseline process.

#### Scenario: Dual-link behavior stays outside current runtime claim
- **WHEN** the repository describes current hosted or target/lab COMM runtime
- **THEN** it SHALL describe the active policy as one active relay path plus
  explicit switch or failover boundaries
- **AND** it SHALL keep simultaneous dual-link arbitration, concurrent
  multi-owner relay, and always-on split-link runtime guarantees as future work

### Requirement: Current Retry Responsibility Stays Ground-Side And Whole-Command Only

The current baseline SHALL keep retry responsibility limited to bounded
ground-side or probe-side whole-command retries and SHALL NOT describe current
COMM runtime policy as packet retry, file-packet retry, or reliable-transfer
ownership.

#### Scenario: Current retry wording avoids transfer-scope inflation
- **WHEN** a current COMM path or operator flow retries a command or file
  command
- **THEN** that retry SHALL be described as a whole-command resend helper on
  the ground or probe side
- **AND** the wording SHALL NOT imply current packet retransmission,
  link-runtime ARQ, or reliable-transfer closure

### Requirement: Future Target-Bearing Node-6 Adjunct Uses Role-Valid Commands And Bounded Quiet Rescue

The future target-bearing dual-link proof SHALL use role-valid low-authority
commands for any concurrent non-quiet node-`6` `uhf-backup` adjunct, and if the
non-quiet node-`6` command attempt fails it MAY use quiet node-`6` only as
bounded adjunct rescue.

#### Scenario: Concurrent `uhf-backup` adjunct keeps authority policy intact
- **WHEN** the future proof exercises a concurrent non-quiet node-`6`
  `uhf-backup` adjunct
- **THEN** it SHALL use only commands that the current `uhf-backup` role
  allowlists for bounded read/status continuity
- **AND** it SHALL NOT select a denied high-authority opcode and then describe
  that denial as evidence about the future simultaneous claim boundary

#### Scenario: Quiet rescue does not become non-quiet closure
- **WHEN** quiet node-`6` is used after a failed non-quiet node-`6` command
  attempt
- **THEN** the repository MAY use the quiet path to rescue the bounded UHF
  adjunct needed for the overall target claim
- **AND** it SHALL keep that rescue distinct from any non-quiet
  operator-observability verdict
- **AND** it SHALL NOT restate quiet rescue as proof that non-quiet node-`6`
  observability was clean

### Requirement: First Target-Bearing Dual-Link Proof Is Branch-Scoped And Switch-Closed

The comm subsystem SHALL implement the first repository-owned target-bearing
dual-link proof on the physical target CAN + UHF UART path as a bounded,
primary-led, switch-closed family.

#### Scenario: Default node-5 truth remains the governing primary surface
- **WHEN** the first implementation-bearing target dual-link proof runs
- **THEN** it SHALL close phase A on the default target node-`5` S-band
  command truth
- **AND** it SHALL NOT treat node-`6` activity as replacing that governing
  primary surface

#### Scenario: Phase B adjunct stays minimal and role-valid
- **WHEN** the same proof exercises concurrent non-quiet node-`6`
  `uhf-backup`
- **THEN** its mandatory adjunct gate SHALL be one allowlisted low-authority
  read/status command
- **AND** it SHALL NOT require a richer same-path `EPS_GET_STATUS` plus
  `ADCS_GET_ATTITUDE` sequence as the mandatory phase-B PASS gate

#### Scenario: Phase C remains independently non-quiet
- **WHEN** the proof claims formal UHF command truth
- **THEN** it SHALL close that truth only after explicit switch to
  `uhf-primary-after-failover`
- **AND** that switched phase SHALL pass on the non-quiet physical node-`6`
  path even if quiet rescue was needed earlier for phase B

#### Scenario: Quiet rescue stays phase-B-only
- **WHEN** a failed non-quiet `uhf-backup` adjunct attempt is retried on quiet
  node-`6`
- **THEN** the repository MAY use that retry only to rescue the phase-B
  adjunct
- **AND** it SHALL NOT describe that rescue as replacing the mandatory phase-C
  switched non-quiet truth

### Requirement: Bounded reliable transfer on the current exact official `.fdp` paths

The comm subsystem SHALL provide one bounded reliable-transfer contract for
current official `.fdp` whole-file requests on exactly two current formal
paths:

- default S-band node-`5`
- explicit-switched `uhf-primary-after-failover` node-`6`

#### Scenario: Selected official `.fdp` requests use the reliable sidecar on the exact allowed path

- **WHEN** `CommController` admits a whole-file `.fdp` request while the
  current primary file link is one current exact allowed reliable-transfer path
- **THEN** it SHALL route that request through the bounded reliable-transfer
  sidecar instead of delegating the active sender to stock `FileDownlink`
- **AND** it SHALL keep `DpCatalog` as the upstream request/completion owner
- **AND** it SHALL preserve the current non-selected baseline path for other
  requests

### Requirement: Reliable transfer uses bounded segment-window semantics

The comm subsystem SHALL define first-version reliable-transfer behavior in
terms of fixed-size file segments and cumulative ACK progress.

#### Scenario: Reliable transfer progresses within one transfer context

- **WHEN** a v1 reliable transfer is active
- **THEN** the sender SHALL emit `Fw::FilePacket`-aligned `START`, `DATA`,
  `END`, and `CANCEL` vocabulary over the bounded node-local sidecar services
  on the admitted exact path
- **AND** the data-segment payload ceiling SHALL be `160` bytes on that
  bounded reliable-transfer sidecar path only
- **AND** the sender SHALL bound the cumulative ACK resend window to `2`
  segments
- **AND** the sender SHALL treat lack of forward ACK progress for one timeout
  interval as a resend trigger
- **AND** the sender SHALL stop claiming success unless final receiver
  verification passes
- **AND** the wording SHALL NOT treat the `160`-byte segment size as a generic
  repo-wide transport MTU

### Requirement: Reliable transfer completion is distinct from whole-command retry

The comm subsystem SHALL keep ground whole-command retry separate from v1
reliable-transfer success semantics.

#### Scenario: Transfer success is established inside one admitted attempt

- **WHEN** a reliable-transfer attempt is admitted
- **THEN** success SHALL require `BEGIN` acceptance, cumulative ACK completion,
  and receiver final verification for file size, checksum, and SHA-256
- **AND** an operator reissuing the original command SHALL be treated as a new
  transfer attempt rather than a hidden part of the same success claim

### Requirement: Reliable transfer exposes bounded failure truth

The comm subsystem SHALL expose reviewable failure truth for the v1 path.

#### Scenario: Reliable transfer stops after bounded no-progress retries

- **WHEN** cumulative ACK progress does not advance and the resend budget is
  exhausted
- **THEN** the sender SHALL mark the transfer as failed
- **AND** it SHALL report partial progress in terms of highest contiguous
  segment and acknowledged bytes
- **AND** it SHALL abort the receiver-side temp transfer context instead of
  promoting a final artifact

#### Scenario: Duplicate segments arrive within the active transfer context

- **WHEN** the receiver observes a duplicate segment for the active transfer id
- **THEN** it SHALL ignore duplicate payload data
- **AND** it SHALL preserve the current contiguous ACK boundary
- **AND** duplicate observation by itself SHALL NOT convert the transfer into
  failure

### Requirement: Reliable transfer remains bounded to current baseline

The comm subsystem SHALL keep reliable transfer bounded to the current official
HK `.fdp` whole-file family and to exactly two current formal paths:

- default S-band node-`5`
- explicit-switched `uhf-primary-after-failover` node-`6`

#### Scenario: UHF reliable transfer stays explicit-switch-only

- **WHEN** `uhf-reliable-transfer-v1` is cited
- **THEN** it SHALL mean reliable transfer on the current official HK `.fdp`
  family only after an explicit switch onto `uhf-primary-after-failover`
- **AND** it SHALL NOT be interpreted as `uhf-backup` reliable transfer or as
  automatic failover-to-UHF reliable transfer

#### Scenario: Active transfer contexts do not migrate across path changes

- **WHEN** a reliable transfer is already active and the primary file path
  changes
- **THEN** the in-flight transfer SHALL stay bound to its start-time path and
  target node
- **AND** the role change SHALL abort or fail that transfer boundedly instead
- **AND** it SHALL NOT migrate the active transfer across paths

#### Scenario: Non-claims remain explicit

- **WHEN** `uhf-reliable-transfer-v1` is cited
- **THEN** it SHALL NOT be interpreted as proof of RF closure,
  restart-persistent resume, broad CFDP adoption, one-GDS aggregation,
  one-gateway multiplexing, generic arbitrary-file authority redesign, or
  broader simultaneous dual-link runtime arbitration

### Requirement: Target Node-6 Non-Quiet Diagnosis Stays Separate From Quiet Proof

The comm subsystem SHALL keep target/lab quiet node-`6` proof distinct from any
later governed non-quiet target node-`6` diagnosis until the repository has
fresh evidence for the non-quiet boundary.

#### Scenario: Quiet proof keeps current meaning
- **WHEN** reviewers inspect the existing target/lab quiet node-`6` proof
- **THEN** that proof SHALL remain bounded to the current quiet-path command,
  file, sequence, failover, and beacon-suppress records
- **AND** it SHALL NOT be reinterpreted as proof of general non-quiet
  background-telemetry stability

#### Scenario: Non-quiet diagnosis keeps owner boundary explicit
- **WHEN** the repository adds a target node-`6` non-quiet diagnosis wrapper
- **THEN** the diagnosis SHALL treat `ground_ttc_gateway` as a raw relay only
- **AND** it SHALL keep runtime or egress ownership on the OBC COMM boundary
  unless the evidence proves a narrower owner

### Requirement: Oracle-Only Node-6 Findings Do Not Force Runtime Change

The comm subsystem SHALL allow target/lab node-`6` residual issues to close at
the proof/oracle boundary when fresh evidence shows correct target-side command
truth with polluted ground observability only.

#### Scenario: Oracle contamination stays a tooling boundary
- **WHEN** quiet control passes
- **AND** non-quiet target node-`6` command truth remains correct in target
  journal or equivalent target-local readback
- **AND** ground event/channel acceptance is the only degraded surface
- **THEN** the repository SHALL treat the issue as oracle-only
- **AND** it SHALL NOT claim that nominal runtime policy or relay ownership
  needed to change just to make the oracle convenient

### Requirement: Mixed Node-6 Findings Freeze The Runtime Residual Narrowly

The comm subsystem SHALL keep mixed target/lab node-`6` findings bounded to the
proven physical coexistence surface until a later change narrows the minimal
runtime owner further.

#### Scenario: Mixed node-6 diagnosis does not blame GDS or gateway
- **WHEN** quiet control passes or partially passes
- **AND** fresh target CAN node-`6` non-quiet evidence shows command ingress
  degrading before target acceptance while byte captures confirm the command
  left ground
- **AND** a supporting comparator indicates the higher-level target command
  policy is still healthy on an adjacent non-CAN southbound
- **THEN** the repository SHALL classify the residual as mixed
- **AND** it SHALL keep `ground_ttc_gateway` as a raw relay rather than the
  chosen runtime owner
- **AND** it SHALL NOT claim a second GDS is the required fix

### Requirement: Current Baseline Transport Ceilings Are Path-Specific And Source-Derived

The comm subsystem SHALL freeze current transport ceilings as path-specific
repo-local contracts derived from checked-in constants and serializer formulas,
not by reinterpreting hosted framing facts as universal MTU truth.

#### Scenario: Current command ceilings are derived from the command-envelope budget

- **WHEN** the current `sband-primary` or `uhf-backup` command-ingress ceiling
  is documented
- **THEN** it SHALL be derived from
  `FW_CMD_ARG_BUFFER_MAX_SIZE - command-envelope-fixed-overhead`
- **AND** the current admitted serialized inner `Fw::CmdPacket` ceiling SHALL
  be `446` bytes
- **AND** the current admitted inner command-argument ceiling SHALL be `440`
  bytes

#### Scenario: Current file/downlink ceiling is derived from stock FileDownlink sizing

- **WHEN** the current `uhf-primary-after-failover` official file/downlink
  ceiling is documented
- **THEN** it SHALL be derived from `FW_FILE_BUFFER_MAX_SIZE`,
  `Fw::FilePacket::DataPacket::HEADERSIZE`, and
  `sizeof(FwPacketDescriptorType)`
- **AND** the documented file-data ceiling SHALL match the active configured
  `FW_FILE_BUFFER_MAX_SIZE` rather than an older fixed `256`-byte baseline

#### Scenario: Hosted CCSDS frame size stays a framing fact

- **WHEN** the current hosted CCSDS frame size is described
- **THEN** it SHALL remain a hosted/configured framing fact
- **AND** the wording SHALL NOT treat that frame size as the admitted command
  or file inner-payload ceiling for the active baseline

### Requirement: Current APID Governance Freezes Reservation Truth With Proof Split

The comm subsystem SHALL freeze `ComCfg.Apid` as the current baseline APID
reservation source while distinguishing active path-proven flows from reserved
current-code values and future allocation work.

#### Scenario: Current active flows stay explicit

- **WHEN** the current active CCSDS APID flows are described
- **THEN** command `0`, telemetry `1`, log/event `2`, and file `3` SHALL be
  identified as active path-proven operational flows

#### Scenario: Reserved values stay distinct from active proof

- **WHEN** the current APID reservation map is documented
- **THEN** packetized telemetry `4`, data product `5`, idle `6`, handshake
  `0x00FE`, unknown `0x00FF`, idle packet `0x07FF`, and invalid values
  `>= 0x0800` SHALL be identified as governed reserved or invalid classes
- **AND** the wording SHALL NOT imply that those values are already active
  operational flows on a proven path

#### Scenario: Future APID claims remain governed

- **WHEN** later work introduces a new active APID claim or reassigns an
  existing reserved value
- **THEN** that work SHALL update code, current docs, formal specs, and fresh
  evidence through a governed change instead of treating the checked-in enum as
  self-approving runtime truth

### Requirement: Target Secure Auth Proof Reuses Existing S-Band And UHF Paths

The target secure-auth proof SHALL reuse the repository-owned target S-band
node-`5` path and physical node-`6` UHF path instead of defining a new target
transport or launch workflow.

#### Scenario: S-band target proof stays on registry entry 59
- **WHEN** `target-secure-auth-proof-v1` proves S-band secure auth behavior
- **THEN** it SHALL run through the existing macOS `fprime-gds` plus
  `ground_ttc_gateway` to `subsystem.local` node `5`, SocketCAN, and
  `obc.local` path
- **AND** it SHALL record that hosted secure-auth evidence is ancestry only,
  not target proof.

#### Scenario: UHF target proof stays bounded on registry entry 69
- **WHEN** `target-secure-auth-proof-v1` proves UHF secure auth behavior
- **THEN** it SHALL use the existing physical node-`6` `uhf-backup` adjunct
  plus explicit switch to `uhf-primary-after-failover`
- **AND** it SHALL keep `uhf-backup` and `uhf-primary-after-failover`
  authority claims separate.

### Requirement: Target Secure Auth Proof Keeps Runtime Scope Bounded

The target secure-auth proof SHALL validate target deployment behavior for the
already-designed secure-auth and bounded uplink-authority flows without adding
new feature scope unless the target proof exposes a product defect.

#### Scenario: S-band target proof covers secure command and staged upload
- **WHEN** the target S-band proof completes
- **THEN** it SHALL show APID `0x00FE` challenge auth, secure command v2 on
  the command APID, non-`1` first accepted secure-command sequence, strict
  next-sequence enforcement, malformed handshake fail-closed behavior, and
  `.sequence-staging/<leaf>` upload admission after secure auth.

#### Scenario: UHF target proof covers backup denial and re-auth
- **WHEN** the target UHF proof completes
- **THEN** it SHALL show `ServiceID = 2` auth on physical node `6`,
  `uhf-backup` read/status secure-command acceptance, `uhf-backup`
  high-authority denial, `uhf-backup` staged-upload denial, old auth/session
  invalidation after switch, and re-auth before
  `uhf-primary-after-failover` secure-command acceptance
- **AND** it SHALL NOT claim UHF primary staged-upload success in this change.

#### Scenario: Non-goals remain explicit
- **WHEN** the target secure-auth proof is documented
- **THEN** it SHALL NOT claim encryption, RF closure, boot-trust expansion,
  hardware-backed key storage, generic file authority, one-GDS aggregation,
  one-gateway multiplexing, or legacy v1 retirement.

### Requirement: Payload FDP Reuses Current Official Catalog Ownership
The comm subsystem SHALL treat canonical payload `.fdp` artifacts as part of the existing official `DpCatalog`-owned data-product delivery path.

#### Scenario: Payload data products do not create a second downlink owner
- **WHEN** `DpCatalog` selects a canonical payload `.fdp` file for downlink
- **THEN** `CommController` SHALL handle that request through the same current official `DpCatalog -> FileDownlink` ownership boundary used for other official data products
- **AND** the payload slice SHALL NOT introduce a payload-specific direct file-send command or a second payload file owner

#### Scenario: Governed target node-5 payload proof reuses the stock official path
- **WHEN** the repository proves canonical payload `.fdp` delivery on the
  governed target node-`5` S-band path
- **THEN** the proof SHALL use the stock `BUILD_CATALOG` plus
  `START_XMIT_CATALOG` delivery chain on that path
- **AND** it SHALL keep payload official closure distinct from Pi-local direct
  payload capture evidence

### Requirement: Payload FDP Does Not Broaden Reliable Transfer Scope
The first payload end-to-end closure slice SHALL keep the current reliable-transfer family bounded to its existing official HK `.fdp` scope.

#### Scenario: Payload official downlink stays on the stock path
- **WHEN** a canonical payload `.fdp` is downlinked on the current official path
- **THEN** the repository MAY prove byte-match and decode on the stock `DpCatalog` and `FileDownlink` path
- **AND** it SHALL NOT describe that proof as widening the current selected reliable-transfer family from official HK `.fdp` to payload `.fdp`

### Requirement: Current Node-5 S-band Live Content Uses Curated Summary Instead Of Broad Family Chatter

The current node-`5` auth-gated S-band live stream SHALL treat component-owned
scheduled summary as the baseline live content for the first selected family
set rather than forwarding every scheduled family detail as current operator
truth.

#### Scenario: Post-auth live visibility stays available but thinner
- **WHEN** node-`5` S-band live observability is open after accepted auth
- **THEN** the current baseline SHALL still provide live operator visibility
- **AND** it SHALL prefer curated scheduled summary and critical transition or
  fault events over broad scheduled family detail from `EPS`, `GPS`, `ADCS`,
  `RADIO`, and `STORAGE`.

### Requirement: Raw Radio Observation Remains Controller-Owned But Fresh Readback Is Explicit

The current comm baseline SHALL keep raw radio observation inside
`RadioController`, SHALL NOT promote raw radio detail into COMM policy truth,
and SHALL make `RADIO_GET_STATUS` an explicit fresh bounded readback command.

#### Scenario: Scheduled radio visibility is summary-only
- **WHEN** `RadioController` refreshes radio status on its scheduled path
- **THEN** it SHALL publish only summary observation state needed for current
  live review
- **AND** it SHALL keep raw radio detail out of the scheduled baseline live
  stream.

#### Scenario: Explicit radio status readback is fresh and bounded
- **WHEN** `RADIO_GET_STATUS` or a radio control command requests or returns a
  radio sample
- **THEN** `RadioController` SHALL interpret that sample through the same
  owner-controlled apply path used for cached observation state
- **AND** it SHALL make the detailed radio status reviewable as bounded
  readback without re-expanding broad scheduled live chatter.

### Requirement: S-band Live Packet Observability Is Auth-Gated And Quiet By Default

The current COMM runtime SHALL keep packetized S-band live `event/tlm`
observability quiet until an accepted authenticated S-band secure session is
active for the current comm-managed node-`5` path.

#### Scenario: Pre-auth S-band does not emit wholesale live packet chatter
- **WHEN** S-band is the current primary telemetry link
- **AND** no accepted authenticated S-band secure session is active
- **THEN** the runtime SHALL suppress packetized live `event/tlm` egress on the
  S-band path
- **AND** it SHALL keep command/auth closure plumbing available for the same
  path
- **AND** it SHALL NOT describe quiet startup as loss of command readiness,
  beacon duty, or file-role policy.

#### Scenario: Accepted S-band auth opens live packet observability
- **WHEN** `CommandIngressAuthority` synthesizes an accepted authenticated
  S-band session for the current comm-managed node-`5` path
- **THEN** `CommController` SHALL enable S-band live packet observability for
  that active session
- **AND** packetized live `event/tlm` SHALL remain enabled only while that
  authenticated session remains active.

#### Scenario: Session invalidation closes live packet observability
- **WHEN** the owning S-band authenticated session is revoked, role-invalidated,
  reconfigured by failover policy, or lost on restart
- **THEN** `CommController` SHALL close S-band live packet observability
  immediately
- **AND** the runtime SHALL require a new accepted S-band authentication cycle
  before live packet visibility opens again.

### Requirement: Current Observability Tiers Stay Distinct

The current baseline SHALL keep always-on critical surfaces, auth-gated live
packet observability, and bounded `GET_*` summary readback as distinct
observability tiers rather than restating all operator visibility as one
undifferentiated live telemetry surface.

#### Scenario: Always-on critical surfaces stay narrow
- **WHEN** the current baseline describes always-on observability
- **THEN** it SHALL keep that tier limited to UHF beacon plus command/auth
  closure plumbing and other already-governed minimum runtime surfaces
- **AND** it SHALL NOT restate packetized S-band `event/tlm` chatter as an
  always-on requirement by default.

#### Scenario: Bounded summary readback stays component-owned
- **WHEN** operators need status outside the current live packet tier
- **THEN** the current baseline SHALL reuse bounded component-owned `GET_*` or
  read/status command surfaces and their summary events/telemetry
- **AND** it SHALL NOT describe those bounded readbacks as a second hidden
  continuous command or telemetry plane.

### Requirement: Current Maintained Node-5 Proofs Depend On Post-Auth Visibility, Not Pre-Auth Chatter

The current maintained node-`5` S-band proof family SHALL treat live
observability dependencies as post-auth session behavior rather than as a
pre-auth always-chattering startup requirement.

#### Scenario: Maintained node-5 proofs remain compatible with auth-gated live visibility
- **WHEN** a maintained node-`5` hosted or target proof depends on S-band live
  `event/tlm` visibility
- **THEN** that proof SHALL open or reuse an accepted authenticated S-band
  session before claiming live observability
- **AND** it SHALL NOT require pre-auth broad S-band chatter as the governing
  baseline truth.

### Requirement: Manual Secure Ops Helper Stays Separate From GDS UI

The current COMM baseline SHALL expose manual secure auth, secure-v2 command,
and governed sequence/file operator actions through a repo-owned helper surface
that remains distinct from the stock GDS UI.

#### Scenario: GDS UI is not treated as the secure command plane
- **WHEN** operators use the maintained manual dual-GDS surface
- **THEN** stock `fprime-gds` SHALL remain the reviewable TT&C observation
  surface
- **AND** secure auth, secure-v2 command send, governed staged upload, and
  governed `SEQ_*` actions SHALL run through the repo-owned helper CLI instead
  of by intercepting stock GDS UI commands.

### Requirement: Manual Secure Ops Helper Uses The Current Secure Baseline

The maintained manual secure-ops helper SHALL use the current secure-auth plus
secure-v2 command path and SHALL keep legacy `SESSION_OPEN` out of the default
operator flow.

#### Scenario: Helper derives secure session from the tracked keystore
- **WHEN** the manual helper establishes auth on the maintained hosted or
  target COMM path
- **THEN** it SHALL use the tracked command-auth keystore material for the
  selected secure service
- **AND** it SHALL derive and persist the session using the current secure
  handshake and secure-v2 packet format.

#### Scenario: Helper scope stays governed
- **WHEN** an operator uses the maintained helper for file or sequence actions
- **THEN** it SHALL permit only governed `.sequence-staging/<leaf>` upload and
  `SequenceAdmissionController` wrapper commands
- **AND** it SHALL NOT present arbitrary file-uplink destinations, raw
  `SeqDispatcher.RUN`, or raw `CmdSequencer` controls as the maintained default
  operator surface.

### Requirement: Manual Secure Session State Has Explicit Invalidation

The maintained manual helper SHALL persist its auth/session state under the
owned surface root and SHALL invalidate that state on explicit band changes,
timeout, manual clear, or surface restart.

#### Scenario: Session invalidation is reviewable
- **WHEN** an operator inspects the maintained manual helper status
- **THEN** the helper SHALL report whether an active secure session exists,
  which band/service owns it, the next secure sequence, and whether the stored
  state has been invalidated by timeout or band transition.

### Requirement: Target Manual Auth Uses A Provenance Gate

The maintained target manual helper SHALL verify the installed release keystore
and active service root before completing manual auth establishment.

#### Scenario: Target helper rejects stale installed auth material
- **WHEN** target manual auth is requested
- **THEN** the helper SHALL compare the repo-tracked keystore SHA with the
  installed release keystore SHA
- **AND** it SHALL verify that the active OBC service still points at the
  governed current release root
- **AND** it SHALL refuse to claim successful auth establishment when that
  provenance gate fails.

### Requirement: Routine Healthy CSP Ping Success Is Not Part Of The Current Ground Event Surface

The current comm baseline SHALL keep periodic CSP ping success out of the
default packetized operator event stream while preserving failure and link
transition visibility.

#### Scenario: Healthy ping success updates counters without a success event
- **WHEN** `CspBridge` completes a ping command or runtime health probe and
  the runtime call succeeds with `success = true`
- **THEN** it SHALL continue updating the existing CSP counters and command
  response semantics
- **AND** it SHALL NOT emit `CSP_PING_RESULT` for that healthy success.

#### Scenario: Ping failure remains reviewable
- **WHEN** `CspBridge` completes a ping runtime call successfully but the ping
  result reports `success = false`
- **THEN** it SHALL still emit `CSP_PING_RESULT(targetNode, false, timeoutMs)`
- **AND** it SHALL preserve the existing failure/error signaling behavior.

### Requirement: Mission Console Consumes The Maintained Manual COMM Operator Surface

The COMM subsystem capability SHALL allow a repo-owned Mission Console to
consume the maintained manual secure auth, secure-v2 command, governed staged
upload, and governed `SEQ_*` operator surface without redefining stock GDS as
the secure command plane.

#### Scenario: Mission Console does not redefine GDS UI authority
- **WHEN** the Mission Console is used on the maintained hosted or target manual
  dual-GDS surface
- **THEN** stock `fprime-gds` SHALL remain a reviewable observation surface
- **AND** Mission Console operator actions SHALL still run through the
  repo-owned helper authority rather than through stock GDS UI interception.

### Requirement: Mission Console Ensure-Auth Respects Existing Session Invalidation

The maintained COMM operator path SHALL allow Mission Console `ensure-auth`
behavior that re-establishes auth only when no valid session exists and that
still treats band switch, timeout, manual clear, or surface restart as session
invalidation boundaries.

#### Scenario: Mission Console re-auths after invalidation but not before
- **WHEN** a Mission Console action requests `ensure-auth`
- **THEN** the Gateway SHALL establish auth only if the stored session is
  missing, invalidated, expired, or tied to an old surface identity
- **AND** an explicit `COMM_SET_ACTIVE` transition SHALL still force the old
  band session to become invalid and require re-auth on the new band.

### Requirement: Mission Console Negative Packet Demo Stays Diagnostic-Only

The maintained COMM operator path SHALL permit a bounded Mission Console
negative packet demo surface that sends selected malformed or replayed packets
through the existing TTS path for lab diagnosis without broadening the formal
operator authority.

#### Scenario: Negative packet demo does not become a second command plane
- **WHEN** Mission Console injects replay, stale-session, duplicate-sequence,
  tampered-sequence, or tampered-MAC packets
- **THEN** it SHALL send them only through the existing `gdsTtsPort` path using
  the current secure packet format or captured raw bytes
- **AND** it SHALL remain explicitly diagnostic-only rather than a general
  operator command surface or broad fuzzing framework.

### Requirement: UHF Primary Live Packet Egress Is Non-Quiet In The Current Baseline

The current COMM runtime SHALL keep live `event/tlm` packet egress enabled when
UHF is the current primary band, unless a separate diagnostic quiet override is
explicitly enabled.

#### Scenario: UHF primary no longer suppresses live packets by default
- **WHEN** the runtime promotes UHF to the current primary band through
  explicit operator policy or executor-owned fault recovery
- **THEN** it SHALL continue to route live packet egress on the current UHF
  primary path
- **AND** it SHALL NOT require a packet-quiet disable override to make bounded
  live readback observable.

#### Scenario: Diagnostic quiet remains separate from UHF primary semantics
- **WHEN** a bounded proof or diagnosis enables the dedicated diagnostic quiet
  overlay
- **THEN** the runtime MAY still suppress packet egress on both bands for that
  probe-owned diagnostic purpose
- **AND** that overlay SHALL NOT redefine the maintained non-quiet UHF primary
  product truth.

### Requirement: Current Autonomous Failover Truth Uses Detector Plus Executor

The current maintained `S-band -> UHF` failover truth SHALL be
`COMM_PRIMARY_UNAVAILABLE` detection plus executor-owned COMM failover rather
than a current proof family centered on manual `COMM_SET_ACTIVE(UHF)`.

#### Scenario: Autonomous failover promotes UHF after current-primary loss
- **WHEN** the current primary S-band COMM stand-in becomes unavailable for the
  configured detector threshold
- **THEN** `CommController` SHALL latch `COMM_PRIMARY_UNAVAILABLE`
- **AND** `RecoveryExecutor` SHALL own the bounded failover actuation that
  promotes UHF to the current primary link set.

#### Scenario: Post-failover UHF command truth requires fresh auth
- **WHEN** executor-owned COMM recovery promotes UHF to primary
- **THEN** the runtime SHALL invalidate stale UHF backup auth/session state
- **AND** it SHALL require a fresh accepted UHF secure-auth cycle before
  bounded UHF primary secure-command readback succeeds again.
