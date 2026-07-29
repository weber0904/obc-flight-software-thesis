# Verification Path Registry

Status: current registered validation-path index.
Last reconciled against in-branch `target-direct-control-matrix-cases-v1`
evidence, `target-timing-wcet-profile-proof-v1`,
`comm-operational-policy-clarification-v1`, and
`uhf-beacon-suppression-runtime-v1` plus
`per-band-stock-ground-stacks-v1` plus
`hosted-simulator-stale-reap-safety-v1` plus
`target-dual-link-claim-oracle-clarification-v1` plus
`challenge-handshake-secure-command-v1` plus
`uplink-authority-and-key-hardening-v1` plus
`target-secure-auth-proof-v1` plus
`payload-e2e-downlink-closure-v1` plus
`payload-async-capture-ack-and-target-proof-v1` plus
`payload-raw-preview-dual-artifact-v1` plus
`uhf-primary-nonquiet-autofailover-v1` on 2026-06-25.

這份文件列出本 repository **已正式證明** 的驗證路徑、其作用範圍、對應 transport / port 分層，以及**沒有**被同一份證據一起證明的相鄰路徑。

使用規則：

- 新 change 若要重用既有驗證路徑，先引用本文件或對應 archived evidence。
- 不得只因 generic F' 常識或一般 upstream usage，就假設某條路在本 repository 已經是 baseline。
- 若某條路徑不在此文件中，就代表它尚未被本 repository 正式註冊為已證明路徑。
- 若 current policy wording 引用某條 COMM path，只能引用此文件或對應
  archived evidence 已證明的部分；未被證明的相鄰行為必須維持為
  non-claim、bounded assumption、或 future follow-up。

## Registered Paths

### 1. Hosted direct `OBC -> GDS` TCP adapter path

- Path:
  - hosted `OBC` binary connects as TCP client to `fprime-gds` IP adapter
- Proven scope:
  - hosted OBC stack can connect to headless `fprime-gds`
  - direct ground TCP adapter path is alive and reviewable
- Does **not** prove:
  - `fprime-cli -> GDS` command/uplink listener path
  - target-side UART/radio/transparent link behavior
  - RF or gateway-mediated ground connectivity
- Governing evidence:
  - [docs/test-records/gds-ground-integration-v1/README.md](test-records/gds-ground-integration-v1/README.md)

### 2. Raspberry Pi direct `OBC -> GDS` TCP adapter path

- Path:
  - Raspberry Pi target OBC connects to host-side `fprime-gds` IP adapter over TCP
- Proven scope:
  - target-side GDS adapter connectivity from the governed Pi workflow
- Does **not** prove:
  - target `fprime-cli -> GDS -> OBC` command/uplink path by itself; cite
    entry 67 for the dedicated matrix-owned command proof
  - transparent UART / RS485 external comm path
- Governing evidence:
  - [docs/test-records/rpi-target-integration-v1/README.md](test-records/rpi-target-integration-v1/README.md)
  - [docs/test-records/active-topccsds-target-alignment-v1/README.md](test-records/active-topccsds-target-alignment-v1/README.md)

### 3. Hosted `fprime-cli -> GDS` command/uplink path for official data-product catalog control

- Path:
  - `fprime-cli command-send --tts-port ...` talks to the GDS internal command listener
  - GDS dispatches commands into hosted OBC
- Proven scope:
  - hosted `DpCatalog.BUILD_CATALOG` and `DpCatalog.START_XMIT_CATALOG` command dispatch through the governed alternate-port smoke path
- Does **not** prove:
  - direct `OBC -> GDS` TCP connectivity by itself
  - target-side `fprime-cli/GDS` smoke on Raspberry Pi
  - general validity of every GDS internal listener port choice
- Governing evidence:
  - [docs/test-records/housekeeping-archive-v1/README.md](test-records/housekeeping-archive-v1/README.md)

### 4. Hosted mock-radio TCP path

- Path:
  - hosted OBC comm stack talks to `radio_mock_server` over TCP
- Proven scope:
  - repo-owned `mock-text` radio adapter baseline
- Does **not** prove:
  - real UART/RS485 hardware
  - EnduroSat transparent mode
- Governing evidence:
  - [docs/test-records/comm-subsystem-v1/README.md](test-records/comm-subsystem-v1/README.md)
  - [docs/test-records/radio-protocol-adapter-v1/README.md](test-records/radio-protocol-adapter-v1/README.md)

### 5. Historical Raspberry Pi to host UART/RS485 hardware path

- Path:
  - Pi OBC comm stack used a real serial device path against a host-side mock peer
  - historical target device on `obc.local` was `/dev/serial0`
- Proven scope:
  - governed Pi-to-host UART/RS485 comm path as historical OBC-side evidence
- Does **not** prove:
  - current target serial ownership
  - real radio vendor protocol
  - RF link behavior
  - GPS live UART behavior
- Governing evidence:
  - [docs/test-records/uart-hw-integration-v1/README.md](test-records/uart-hw-integration-v1/README.md)

### 6. Legacy EnduroSat transparent UART data path

- Path:
  - Pi OBC external comm path uses legacy transparent UART settings and peer behavior
- Proven scope:
  - transparent payload transport over the governed serial path
- Does **not** prove:
  - legacy ESPS control/configuration plane
  - newer `csp-es` hardware generation
  - GDS / ground-station gateway integration
- Governing evidence:
  - [docs/test-records/endurosat-transparent-uart-v1/README.md](test-records/endurosat-transparent-uart-v1/README.md)

### 7. Transparent framed UART link

- Path:
  - repo-owned framed transport on top of transparent UART/RS485
- Proven scope:
  - binary-safe framing/deframing with CRC
  - framed repeated exchange and reconnect recovery in the governed probes
- Does **not** prove:
  - direct GDS integration over the transparent link
  - RF channel behavior
  - vendor-specific control/configuration plane
- Governing evidence:
  - [docs/test-records/transparent-link-framing-v1/README.md](test-records/transparent-link-framing-v1/README.md)
  - [docs/test-records/transparent-link-robustness-v1/README.md](test-records/transparent-link-robustness-v1/README.md)

### 8. Hosted GPS fake/replay path

- Path:
  - hosted OBC runtime consumes repository-owned GPS fake/replay input through `GpsBridge`
  - GPS cached state is surfaced through `GPS_*` telemetry/event behavior and the canonical onboard snapshot surface
- Proven scope:
  - hosted fake/replay GPS ingestion
  - bounded NMEA `GGA/RMC` parsing with valid-fix, no-fix, and malformed/checksum-invalid handling
  - GPS cached-state inclusion in current onboard-state consumers
- Does **not** prove:
  - target-side live UART hardware bring-up
  - live `GY-GPS6MV2` / `NEO-6M` hardware bring-up
  - live-sky fix reception or PPS/timing behavior
- Governing evidence:
  - [docs/test-records/gps-subsystem-v1/README.md](test-records/gps-subsystem-v1/README.md)

### 9. OBC live GPS UART path

- Path:
  - `GY-GPS6MV2` is wired directly to `obc.local:/dev/serial0`
  - `GpsBridge` runs with `OBC_GPS_SOURCE_MODE=live-uart`
  - repository-owned probe drives repeated `gps get` commands against the live UART-backed source
- Proven scope:
  - target-side live UART sentence ingestion into `GpsBridge`
  - cached GPS runtime state updates from real hardware NMEA traffic
  - `LIVE_UART` source mode, `sample=yes`, and advancing accepted-sentence count
- Does **not** prove:
  - live-sky valid fix
  - PPS / precise timing
  - comm / RF integration
  - future CSP or CAN-centered GPS architecture
- Governing evidence:
  - [docs/test-records/gps-live-uart-source-v1/README.md](test-records/gps-live-uart-source-v1/README.md)
  - [docs/test-records/gps-live-uart-hardening-v1/README.md](test-records/gps-live-uart-hardening-v1/README.md)

### 10. Hosted storage health runtime-root path

- Path:
  - hosted OBC runtime scans governed runtime roots through `StorageHealthBridge`
  - operator shell and canonical onboard-state consumers reuse the cached storage health state
- Proven scope:
  - hosted storage observability for `persistent-data`, `staging`, `logs`, and `data-products`
  - threshold-warning and missing-root behavior for the governed runtime roots
  - storage health cached-state inclusion in current onboard-state consumers
- Does **not** prove:
  - Raspberry Pi target-side disk behavior
  - automatic retention / cleanup policy
  - install-root or arbitrary filesystem health governance
- Governing evidence:
  - [docs/test-records/storage-health-v1/README.md](test-records/storage-health-v1/README.md)

### 11. Hosted internal libcsp foundation path

- Path:
  - hosted `csp_zmqproxy` or equivalent repo-local hub starts first
  - hosted OBC `CspBridge` initializes libcsp node `1` over the official ZMQHUB-backed interface
  - hosted CSP peer node accepts ping or raw-send diagnostics on the same governed hub
- Proven scope:
  - hosted internal libcsp runtime bring-up for node `1`
  - hosted ZMQHUB-backed binding and routing substrate
  - diagnostic ping and bounded raw-send semantics through `CspBridge` foundation plumbing
- Does **not** prove:
  - direct `OBC -> GDS` TCP connectivity
  - `fprime-cli -> GDS` command or uplink dispatch
  - EPS or ADCS business traffic migration
  - external comm or transparent UART behavior
- Governing evidence:
  - [docs/test-records/internal-csp-foundation-v1/README.md](test-records/internal-csp-foundation-v1/README.md)

### 12. Hosted EPS internal libcsp service path

- Path:
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - OBC/client node `1` uses the libcsp ZMQHUB-backed interface
  - EPS simulator runs as libcsp node `2`
  - EPS status, PDU, heater/config, and reset traffic uses EPS-owned application ports `10` through `13`
- Proven scope:
  - hosted EPS business traffic over the internal libcsp substrate
  - `EPS_GET_STATUS` equivalent status service request/reply
  - `EPS_SET_PDU` equivalent PDU state change request/reply
  - EPS reset service request/reply
  - CSP runtime metrics record traffic during the EPS vertical-slice smoke
- Does **not** prove:
  - direct `OBC -> GDS` TCP connectivity
  - `fprime-cli -> GDS` command or uplink dispatch
  - ADCS migration to libcsp
  - external comm, transparent UART, GPS, or real EPS hardware behavior
- Governing evidence:
  - [docs/test-records/eps-csp-vertical-slice-v1/README.md](test-records/eps-csp-vertical-slice-v1/README.md)

### 13. Hosted ADCS internal libcsp service path

- Path:
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - OBC/client node `1` uses the libcsp ZMQHUB-backed interface
  - ADCS simulator runs as libcsp node `3`
  - ADCS state, mode, target, and calibration traffic uses ADCS-owned application ports `20` through `23`
- Proven scope:
  - hosted ADCS business traffic over the internal libcsp substrate
  - `ADCS_GET_ATTITUDE` equivalent state service request/reply
  - `ADCS_SET_MODE` equivalent mode state change request/reply
  - `ADCS_SET_TARGET` equivalent target update request/reply
  - `ADCS_CALIBRATE` equivalent calibration request/reply
  - CSP runtime metrics record traffic during the ADCS vertical-slice smoke
- Does **not** prove:
  - direct `OBC -> GDS` TCP connectivity
  - `fprime-cli -> GDS` command or uplink dispatch
  - external comm, transparent UART, GPS, or real ADCS hardware behavior
  - full legacy direct-ZMQ retirement
- Governing evidence:
  - [docs/test-records/adcs-csp-vertical-slice-v1/README.md](test-records/adcs-csp-vertical-slice-v1/README.md)

### 14. Historical hosted CCSDS sequence-staging upload and official sequence-execution proof family

- Path:
  - archived hosted proof used the hosted CCSDS file-uplink path after secure
    auth was already active and current runtime file-role policy allowed staged
    upload on that ingress
  - `ComCcsds.fprimeRouter.fileOut -> FileIngressAuthority -> FileHandling.fileUplink`
  - admitted execution then uses `SequenceAdmissionController -> SeqDispatcher -> CmdSequencer`
- Proven scope:
  - archived hosted proof covered governed `.sequence-staging/<leaf>.bin`
    upload, fail-closed path rejection, direct stock-control denial, governed
    wrapper sequence validate/run/manual/cancel behavior, and official
    `CmdSequencer` / `SeqDispatcher` execution on hosted `TopCcsds`
  - official `SystemResources` telemetry channels were present on the same
    hosted baseline
- Does **not** prove:
  - current secure-baseline maintained wrapper health after
    `legacy-command-envelope-retirement-v1`
  - target / Raspberry Pi sequencing behavior
  - GPS mission-time scheduling
  - secure-auth establishment or runtime file-role policy by itself
  - generic file-uplink governance for all links or destinations
  - conflict-free multi-sequence arbitration
  - direct external operator use of stock `SeqDispatcher.RUN` / `RUN_ARGS` or raw `CmdSequencer` `CS_*`
- Governing evidence:
  - [docs/test-records/official-sequencing-system-resources-v1/README.md](test-records/official-sequencing-system-resources-v1/README.md)
  - Current secure-baseline staged upload authority instead cites
    [docs/test-records/uplink-authority-and-key-hardening-v1/README.md](test-records/uplink-authority-and-key-hardening-v1/README.md)

### 15. Legacy EPS/ADCS direct-ZMQ retirement guardrail

- Path:
  - active EPS and ADCS business traffic uses subsystem-owned CSP service payloads over libcsp
  - hosted development continues to use the allowed libcsp ZMQHUB-backed internal substrate
  - repo-local regression checker rejects restoration of the retired project-local direct-ZMQ request/reply business path
- Proven scope:
  - active EPS and ADCS simulator, bridge, transport, test, and script sources no longer retain the retired direct-ZMQ business transport
  - EPS and ADCS CSP smokes still pass after removal
  - scenario and baseline gates still exercise the active libcsp-first paths
- Does **not** prove:
  - new ground-path behavior
  - external comm, GPS, boot/update, or hardware bring-up behavior
  - real EPS or ADCS hardware compatibility
- Governing evidence:
  - [docs/test-records/legacy-zmq-retirement-v1/README.md](test-records/legacy-zmq-retirement-v1/README.md)

### 16. Historical Raspberry Pi CSP plus external comm baseline path

- Path:
  - Raspberry Pi target started the internal libcsp hub, OBC node `1`, EPS simulator node `2`, and ADCS simulator node `3`
  - the same historical target run used the external comm stack over `obc.local:/dev/serial0`
  - a host-side `radio_mock_server` peer attaches to the host serial device
- Proven scope:
  - historical target-side CSP reachability plus external comm UART coexistence on one OBC-side serial allocation
- Does **not** prove:
  - current target serial ownership
  - direct `OBC -> GDS` or `fprime-cli -> GDS` behavior
  - GPS live UART bring-up
  - RF channel behavior
  - real EPS, ADCS, or radio hardware
  - vendor-specific radio control/configuration plane
- Governing evidence:
  - [docs/test-records/rpi-csp-comm-baseline-validation-v1/README.md](test-records/rpi-csp-comm-baseline-validation-v1/README.md)

### 17. Historical serial resource allocation guardrail

- Path:
  - historical governance rule that reserved Raspberry Pi `/dev/serial0` for the external comm/radio UART path
  - later work reallocated that same device to direct GPS UART on `obc.local`
- Proven scope:
  - reviewable history of the older comm-owned serial allocation decision
- Does **not** prove:
  - current serial ownership
  - any live GPS receiver behavior
- Governing evidence:
  - [docs/test-records/serial-resource-allocation-governance-v1/README.md](test-records/serial-resource-allocation-governance-v1/README.md)

### 18. Raspberry Pi OBC to remote macOS EPS/ADCS simulator CSP path

- Path:
  - Raspberry Pi target runs only the `OBC` process as libcsp node `1`
  - remote macOS host runs `csp_zmqproxy`, EPS simulator node `2`, and ADCS simulator node `3`
  - Pi OBC points `CSP_HUB_HOST` at the macOS LAN host while keeping CSP ports separate from GDS ports
- Proven scope:
  - target-side reachability from Pi OBC node `1` to remote EPS node `2` and ADCS node `3`
  - Pi-side observed `csp ping 2`, `csp ping 3`, `eps get`, and `adcs get`
  - governed remote development-carrier topology for internal CSP over ZMQHUB/TCP/IP
- Does **not** prove:
  - `fprime-cli -> GDS` command dispatch by itself
  - external comm, GPS live UART, RF behavior, or real subsystem hardware
  - future `UART / CAN / RS485` physical-bus behavior
- Governing evidence:
  - [docs/test-records/rpi-remote-grounded-csp-validation-v1/README.md](test-records/rpi-remote-grounded-csp-validation-v1/README.md)

### 19. macOS `fprime-cli -> GDS -> Raspberry Pi OBC -> remote EPS/ADCS simulator` command path

- Path:
  - macOS host runs headless `fprime-gds` alongside the remote CSP hub and simulators
  - `fprime-cli command-send --tts-port ...` dispatches bounded subsystem commands into the Pi OBC
  - Pi OBC forwards subsystem business traffic over the remote internal CSP topology to EPS node `2` and ADCS node `3`
- Proven scope:
  - target-side ground-driven `EPS_SET_PDU(channel=2, enabled=true)` command flow through GDS
  - target-side ground-driven `ADCS_SET_MODE(POINTING)` command flow through GDS
  - Pi-side observed `eps get` and `adcs get` state changes after those commands
- Does **not** prove:
  - every `fprime-cli -> GDS` command path
  - hosted-only direct `OBC -> GDS` adapter connectivity, which remains a separate registered path
  - external comm, GPS live UART, RF behavior, or future physical-bus behavior
- Governing evidence:
  - [docs/test-records/rpi-remote-grounded-csp-validation-v1/README.md](test-records/rpi-remote-grounded-csp-validation-v1/README.md)

### 20. Dual-Pi split-host `obc.local -> subsystem.local` internal CSP path

- Path:
  - `macOS` host runs `csp_zmqproxy`
  - `obc.local` runs only OBC node `1`
  - `subsystem.local` runs EPS simulator node `2` and ADCS simulator node `3`
- Proven scope:
  - governed three-host split-host internal CSP topology
  - `obc.local` reachability to remote EPS and ADCS simulator nodes on `subsystem.local`
  - Pi-side observed `csp ping 2`, `csp ping 3`, `eps get`, and `adcs get`
- Does **not** prove:
  - `fprime-cli -> GDS` command dispatch by itself
  - external comm, GPS live UART, RF behavior, or real subsystem hardware
  - future `UART / CAN / RS485` physical-bus behavior
- Governing evidence:
  - [docs/test-records/dual-pi-subsystem-sim-host-validation-v1/README.md](test-records/dual-pi-subsystem-sim-host-validation-v1/README.md)

### 21. macOS `fprime-cli -> GDS -> obc.local -> subsystem.local` command path

- Path:
  - `macOS` runs headless `fprime-gds` alongside the governed CSP hub
  - `fprime-cli command-send --tts-port ...` dispatches bounded subsystem commands into `obc.local`
  - `obc.local` forwards subsystem business traffic over the split-host internal CSP topology to `subsystem.local`
- Proven scope:
  - bounded `EPS_SET_PDU(channel=2, enabled=true)` flow through GDS plus split-host CSP
  - bounded `ADCS_SET_MODE(POINTING)` flow through GDS plus split-host CSP
  - Pi-side observed `eps get` and `adcs get` state changes after those commands
- Does **not** prove:
  - every `fprime-cli -> GDS` command path
  - hosted-only direct `OBC -> GDS` adapter connectivity
  - external comm, GPS live UART, RF behavior, or future physical-bus behavior
- Governing evidence:
  - [docs/test-records/dual-pi-subsystem-sim-host-validation-v1/README.md](test-records/dual-pi-subsystem-sim-host-validation-v1/README.md)

### 22. Historical dual-Pi split-host CSP plus external comm coexistence path

- Path:
  - `macOS` hosts the CSP hub and the host-side `radio_mock_server`
  - `subsystem.local` hosts EPS simulator node `2` and ADCS simulator node `3`
  - `obc.local` historically ran internal CSP against the split-host subsystem path while external comm used `/dev/serial0`
- Proven scope:
  - historical proof that split-host internal CSP reachability coexisted with target-side external comm UART exchange
  - `radio enable on` and `uart raw STATUS` succeed while the same target run still reaches remote EPS and ADCS simulator nodes
- Does **not** prove:
  - current target serial ownership
  - RF behavior
  - vendor-specific radio control plane
  - transparent/framed UART paths
  - future physical-bus equivalence
- Governing evidence:
  - [docs/test-records/dual-pi-subsystem-sim-host-validation-v1/README.md](test-records/dual-pi-subsystem-sim-host-validation-v1/README.md)

### 23. Shared physical internal SocketCAN CSP path

- Path:
  - `macOS` runs headless `fprime-gds` only
  - `obc.local` runs OBC node `1` through `CSP_TRANSPORT=socketcan` on one active CAN interface
  - `subsystem.local` runs EPS simulator node `2` and ADCS simulator node `3` through one shared active SocketCAN interface
  - `subsystem.local` keeps the second CAN interface reserved and isolated from the active bus
- Proven scope:
  - first governed physical internal CSP carrier path over a CAN FD-capable Linux SocketCAN bus
  - `EPS` and `ADCS` continue to behave as separate logical CSP nodes after replacing hosted `ZMQHUB`
  - bounded `csp ping 2`, `csp ping 3`, `eps get`, and `adcs get` on the active bus
  - bounded `fprime-cli -> GDS -> OBC -> EPS_SET_PDU` and `ADCS_SET_MODE` flows over the same active bus
  - reserved subsystem CAN channel remains isolated and does not observe primary-bus traffic during the governed probe
  - active OBC and subsystem CAN interfaces remain `ERROR-ACTIVE` with no `bus-off`
- Does **not** prove:
  - independent physical EPS and ADCS controllers
  - COMM subsystem traffic or COMM node identity
  - omitted-RF TT&C or RF behavior
  - dual-bus redundancy
  - libcsp use of larger CAN FD payloads or BRS-marked frames
- Governing evidence:
  - [docs/test-records/shared-canfd-csp-bus-foundation-v1/README.md](test-records/shared-canfd-csp-bus-foundation-v1/README.md)

### 24. Hosted gateway-backed COMM omitted-RF TT&C path

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, `ground_ttc_gateway`, hosted `comm_csp_node` as node `4`, and hosted OBC node `1`
  - `ground_ttc_gateway` bridges stock GDS TCP traffic to the lab-side PTY serial-ingress stand-in
  - `comm_csp_node` bridges the serial ingress to COMM-owned internal CSP services `30` through `32`
  - `GroundLinkDriver` runs in `comm-csp` mode so `ComFprime` keeps stock F' framing while the ground transport path traverses `COMM`
- Proven scope:
  - first governed hosted omitted-RF TT&C path through `fprime-cli -> GDS -> gateway -> serial ingress -> COMM -> internal CSP -> OBC`
  - bounded `EPS_SET_PDU(channel=2, enabled=true)` command flow through the new path
  - bounded `ADCS_SET_MODE(POINTING)` command flow through the new path
  - ground-side observed command dispatch/completion events through `fprime-cli events`
  - OBC-side final `eps ... pdu=7` and `adcs mode=POINTING` readback after the bounded commands
  - optional `fprime-cli channels` capture on `GROUND_LINK_TX_BYTES` remains diagnostic only for the legacy node-4 compatibility probe because that specific channel gate is historically flaky
- Does **not** prove:
  - hosted direct `OBC -> GDS` TCP connectivity as the same proof boundary
  - split-host `obc.local -> subsystem.local` deployment
  - physical SocketCAN / shared CAN FD carrier behavior
  - controller-oriented external comm mock, framed, transparent, or historical UART baselines
  - real UART hardware, RF behavior, or file/downlink scope
- Governing evidence:
  - [docs/test-records/comm-csp-node-and-ground-gateway-v1/README.md](test-records/comm-csp-node-and-ground-gateway-v1/README.md)

### 25. Subsystem COMM UART preflight path

- Path:
  - `macOS` uses `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` as the host USB-RS485 endpoint
  - `subsystem.local` uses `/dev/serial0 -> ttyS0` as the subsystem UART endpoint
  - `subsystem.local` runs native-built `radio_mock_server --mode mock-text`
  - macOS runs repo-built `serial_link_probe` and initiates bounded `STATUS`, `ENABLE 1`, and final `STATUS` exchanges
- Proven scope:
  - physical macOS-initiated UART/RS-485 request/reply exchange with `subsystem.local` at `115200`
  - `subsystem.local` serial console/getty cleanup required for stable access:
    `serial-getty@ttyS0.service` inactive and boot cmdline using `console=tty1` only
  - bounded request/reply behavior over explicit host and subsystem serial device paths after macOS sends the first useful frame
- Does **not** prove:
  - clean `subsystem.local` cold-first unsolicited downlink or first-frame request/reply to a passive macOS receiver
  - gateway-backed TT&C over the physical serial link
  - RF or real radio behavior
  - file/downlink behavior
  - target OBC migration
  - COMM shared CAN FD participation
  - `ScenarioBridge`, `ground_pass_open`, or `link_available`
- Governing evidence:
  - [docs/test-records/subsystem-comm-uart-link-preflight-v1/README.md](test-records/subsystem-comm-uart-link-preflight-v1/README.md)

### 26. Subsystem-origin lab serial acquisition path

- Path:
  - `macOS` opens `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` as a passive raw serial receiver
  - `subsystem.local` opens `/dev/serial0 -> ttyS0` and transmits bounded acquisition records
  - the repo-owned probe sends conservative ASCII preamble lines followed by `OBCACQ1` framed marker payloads with sequence, length, and CRC validation
- Proven scope:
  - passive macOS acquisition of bounded frames initiated by `subsystem.local`
  - exact recovery of 8 framed payloads at `115200` using 20 preamble lines and 500 ms inter-frame spacing
  - reviewable evidence that the receiver can skip cold-start garbage and recover later governed marker frames
- Does **not** prove:
  - gateway-backed TT&C over the physical serial link
  - stock F' command, event, telemetry, or file/downlink frames over this link
  - simultaneous bidirectional half-duplex behavior
  - clean first-byte subsystem-origin traffic
  - RF or real radio behavior
  - target OBC migration
  - COMM shared CAN FD participation
- Governing evidence:
  - [docs/test-records/comm-lab-serial-acquisition-v1/README.md](test-records/comm-lab-serial-acquisition-v1/README.md)

### 27. Physical lab serial gateway-backed COMM uplink ingress path

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, `ground_ttc_gateway`, the hosted OBC node `1`, and the governed CSP ZMQHUB substrate
  - `ground_ttc_gateway` connects stock GDS TCP traffic to `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
  - the physical RS-485/UART lab link reaches `subsystem.local:/dev/serial0`
  - `subsystem.local` runs native-built `comm_csp_node` as COMM node `4`
  - `comm_csp_node` bridges the serial ingress to COMM-owned CSP services `30` through `32`
  - hosted OBC runs `GroundLinkDriver` in `comm-csp` mode so `ComFprime` keeps stock F' framing while the transport traverses physical lab serial and COMM
- Proven scope:
  - bounded physical lab serial uplink ingress through `fprime-cli -> GDS -> ground_ttc_gateway -> physical serial -> subsystem.local COMM node 4 -> CSP -> hosted OBC`
  - gateway serial acquisition preamble of 20 lines with 500 ms spacing before useful GDS traffic on the current lab wiring
  - bounded `EPS_SET_PDU(channel=2, enabled=true)` command flow through the physical serial ingress
  - bounded `ADCS_SET_MODE(POINTING)` command flow through the physical serial ingress
  - OBC-side final `eps ... pdu=7` and `adcs mode=POINTING` readback after bounded command retries
- Does **not** prove:
  - full physical lab serial TT&C as a registered path
  - fully bounded `fprime-cli events` and `fprime-cli channels` downlink proof
  - clean no-preamble first-byte behavior on the current physical UART link
  - RF or real radio behavior
  - file/downlink behavior
  - target OBC migration
  - COMM shared CAN FD participation
  - `ScenarioBridge`, `ground_pass_open`, or `link_available`
- Governing evidence:
  - [docs/test-records/ttc-over-comm-lab-serial-ingress-v1/README.md](test-records/ttc-over-comm-lab-serial-ingress-v1/README.md)

### 28. Physical lab serial gateway-backed COMM bounded TT&C path

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, `ground_ttc_gateway`, the hosted OBC node `1`, and the governed CSP ZMQHUB substrate
  - `ground_ttc_gateway` connects stock GDS TCP traffic to `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
  - the physical RS-485/UART lab link reaches `subsystem.local:/dev/serial0`
  - `subsystem.local` runs native-built `comm_csp_node` as COMM node `4`
  - `comm_csp_node` bridges serial ingress and downlink bytes through COMM-owned CSP services `30` through `32`
  - hosted OBC runs `GroundLinkDriver` in `comm-csp` mode so `ComFprime` keeps stock F' framing while the command/event/channel transport traverses physical lab serial and COMM
- Proven scope:
  - bounded physical lab serial TT&C through `fprime-cli -> GDS -> ground_ttc_gateway -> physical serial -> subsystem.local COMM node 4 -> CSP -> hosted OBC -> COMM downlink -> ground_ttc_gateway -> GDS -> fprime-cli`
  - gateway serial acquisition preamble of 20 lines with 500 ms spacing before useful GDS traffic on the current lab wiring
  - bounded `EPS_SET_PDU(channel=2, enabled=true)` command flow through the physical serial ingress
  - bounded `ADCS_SET_MODE(POINTING)` command flow through the physical serial ingress
  - OBC-side final `eps ... pdu=7` and `adcs mode=POINTING` readback after bounded command retries
  - ground-side observed command events through `fprime-cli events`
  - ground-side observed telemetry through `fprime-cli channels` on `GROUND_LINK_TX_BYTES`
- Does **not** prove:
  - file/downlink behavior over the COMM TT&C path
  - clean no-preamble first-byte behavior on the current physical UART link
  - RF or real radio behavior
  - target OBC migration
  - COMM shared CAN FD participation
  - `ScenarioBridge`, `ground_pass_open`, or `link_available`
- Governing evidence:
  - [docs/test-records/ttc-over-comm-lab-serial-downlink-v1/README.md](test-records/ttc-over-comm-lab-serial-downlink-v1/README.md)

### 29. Historical physical lab serial gateway-backed COMM HK fallback file/downlink path

- Path:
  - historical note: this entry records the retired HK fallback path and is not part of the current official `.fdp` baseline
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, `ground_ttc_gateway`, the hosted OBC node `1`, and the governed CSP ZMQHUB substrate
  - `ground_ttc_gateway` connects stock GDS TCP traffic to `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
  - the physical RS-485/UART lab link reaches `subsystem.local:/dev/serial0`
  - `subsystem.local` runs native-built `comm_csp_node` as COMM node `4`
  - `comm_csp_node` bridges serial ingress and downlink bytes through COMM-owned CSP services `30` through `32`
  - hosted OBC runs `GroundLinkDriver` in `comm-csp` mode and queues housekeeping archive files through stock F' `FileDownlink`
  - `fprime-gds` writes received files under the configured ground file-storage directory
- Proven scope:
  - bounded housekeeping archive file/downlink over `HK_DOWNLINK_* -> FileDownlink -> COMM downlink -> ground_ttc_gateway -> GDS file storage`
  - prerequisite bounded physical lab serial command/event/channel TT&C before file assertions
  - gateway serial acquisition preamble of 20 lines with 500 ms spacing before useful GDS traffic on the current lab wiring
  - at least two occupied HK archive slots generated with paced `HK_CAPTURE_NOW` over the same GDS/COMM path
  - `HK_DOWNLINK_INDEX` received as `hk-index.csv` and byte-matched against the OBC runtime source snapshot
  - two `HK_DOWNLINK_SLOT` files received as `hk-slot-00-g000000.bin` and `hk-slot-01-g000000.bin` and byte-matched against OBC runtime source snapshots
  - COMM node `4` and services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS` unchanged
- Does **not** prove:
  - arbitrary onboard file path downlink
  - RF or real radio behavior
  - target OBC migration
  - clean no-preamble first-byte behavior on the current physical UART link
  - full archive ring fill, generation wraparound, or retention policy
  - `ScenarioBridge`, `ground_pass_open`, or `link_available`
  - COMM shared CAN FD participation
- Governing evidence:
  - [docs/test-records/comm-ttc-file-downlink-v1/README.md](test-records/comm-ttc-file-downlink-v1/README.md)

### 30. Physical COMM SocketCAN command/event/channel TT&C path

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and `ground_ttc_gateway`
  - `ground_ttc_gateway` connects stock GDS TCP traffic to `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
  - the physical RS-485/UART lab link reaches `subsystem.local:/dev/serial0`
  - `subsystem.local` runs `comm_csp_node` as COMM node `4` on `subsystem.local:can1`
  - the shared CAN FD-capable bus reaches target OBC node `1` on `obc.local:can0`
  - EPS and ADCS remain active on `subsystem.local:can0`
  - OBC runs `GroundLinkDriver` in `comm-csp` mode so command/event/channel TT&C traverses COMM and the shared SocketCAN carrier
- Proven scope:
  - bounded command/event/channel TT&C through `fprime-cli -> GDS -> ground_ttc_gateway -> physical serial -> subsystem.local COMM node 4 on can1 -> shared SocketCAN bus -> obc.local can0 -> target OBC -> COMM downlink -> GDS -> fprime-cli`
  - corrected probe-owned CAN bring-up with `bitrate 500000 dbitrate 2000000 fd on` before judging connectivity
  - `csp ping 4`, `csp ping 2`, and `csp ping 3` from target OBC after COMM joins the shared carrier
  - formal reset-to-opposite-state and target `EPS_SET_PDU(channel=2, enabled=true)` / `ADCS_SET_MODE(POINTING)` command flow through the same path
  - OBC-side final `eps ... pdu=7` and `adcs mode=POINTING` readback
  - ground-side observed command events through `fprime-cli events`
  - ground-side observed telemetry through `fprime-cli channels` on `GROUND_LINK_TX_BYTES`
  - non-empty CAN captures on `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1`
  - all active CAN interfaces remain `ERROR-ACTIVE` with no `bus-off`
- Does **not** prove:
  - file/downlink behavior over the SocketCAN-backed COMM path
  - arbitrary onboard file path downlink
  - RF or real radio behavior
  - clean no-preamble first-byte behavior on the current physical UART link
  - dual-bus redundancy
  - independent COMM hardware beyond the `subsystem.local:can1` controller
  - `ScenarioBridge`, `ground_pass_open`, or `link_available`
- Governing evidence:
  - [docs/test-records/comm-csp-socketcan-participation-v1/README.md](test-records/comm-csp-socketcan-participation-v1/README.md)

### 31. Historical physical COMM SocketCAN HK fallback file/downlink path

- Path:
  - historical note: this entry records the retired HK fallback path and is not part of the current official `.fdp` baseline
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and `ground_ttc_gateway`
  - `ground_ttc_gateway` connects stock GDS TCP traffic to `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
  - the physical RS-485/UART lab link reaches `subsystem.local:/dev/serial0`
  - `subsystem.local` runs `comm_csp_node` as COMM node `4` on `subsystem.local:can1`
  - the shared CAN FD-capable bus reaches target OBC node `1` on `obc.local:can0`
  - EPS and ADCS remain active on `subsystem.local:can0`
  - target OBC queues housekeeping archive files through stock F' `FileDownlink`
  - `fprime-gds` writes received files under the configured ground file-storage directory
- Proven scope:
  - bounded housekeeping archive file/downlink through `HK_DOWNLINK_* -> FileDownlink -> COMM downlink over shared SocketCAN -> ground_ttc_gateway -> GDS file storage`
  - prerequisite command/event/channel TT&C over the same SocketCAN-backed COMM path before file assertions
  - TT&C under the file/downlink probe startup load with three consecutive diagnostic cycles
  - `FW_FILE_BUFFER_MAX_SIZE = 256` for bounded stock F' file data packets on the constrained physical COMM path
  - at least two occupied HK archive slots generated with paced `HK_CAPTURE_NOW` over the same GDS/COMM/SocketCAN path
  - `HK_DOWNLINK_INDEX` received as `hk-index.csv` and byte-matched against the target OBC runtime source snapshot
  - two `HK_DOWNLINK_SLOT` files received as `hk-slot-01-g000000.bin` and `hk-slot-00-g000000.bin` and byte-matched against target OBC runtime source snapshots
  - bounded whole-command `HK_DOWNLINK_*` retries gated by final byte matches
  - non-empty CAN captures on `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1`
  - all active CAN interfaces remain `ERROR-ACTIVE` with no `bus-off`
- Does **not** prove:
  - arbitrary onboard file path downlink
  - RF or real radio behavior
  - clean no-preamble first-byte behavior on the current physical UART link
  - full archive ring fill, generation wraparound, or retention policy
  - `ScenarioBridge`, `ground_pass_open`, or `link_available`
  - dual-bus redundancy
  - independent COMM hardware beyond the `subsystem.local:can1` controller
  - end-to-end missing-packet retransmission, NACK/ARQ recovery, or reliable file transfer under packet loss
- Governing evidence:
  - [docs/test-records/comm-csp-socketcan-file-downlink-v1/README.md](test-records/comm-csp-socketcan-file-downlink-v1/README.md)

### 32. Lab target COMM CSP operational baseline path

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and `ground_ttc_gateway` through the repo-owned lab ground launcher
  - `ground_ttc_gateway` connects stock GDS TCP traffic to `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
  - the physical RS-485/UART lab link reaches `subsystem.local:/dev/serial0`
  - `subsystem.local` runs the service-managed `subsystem-comm-csp-stack.target`
  - `subsystem.local` runs separate EPS node `2`, ADCS node `3`, and COMM node `4` services with distinct systemd status and journal boundaries
  - COMM node `4` runs on `subsystem.local:can1`; EPS and ADCS run on `subsystem.local:can0`
  - `obc.local` boots into the installed OBC release through `obc-comm-csp-stack.service`
  - target OBC runs `GROUND_LINK_MODE=comm-csp`, COMM node `4`, SocketCAN on `obc.local:can0`, and live GPS UART on `obc.local:/dev/serial0`
  - historical note: the original evidence used retired HK fallback file
    downlink; the current operational probe now checks official `.fdp`
    file/downlink through `DpCatalog` instead
- Proven scope:
  - first reusable service-managed lab target operational baseline for the physical COMM SocketCAN path
  - OBC installed-release service migration from `obc-installed-stack.service` to `obc-comm-csp-stack.service`
  - `obc.local` reboot followed by autostart from the installed release
  - non-root long-running OBC and subsystem runtime processes under the transitional `operator` workspace identity
  - root-owned CAN oneshot provisioning as a lab/development helper using `bitrate 500000 dbitrate 2000000 fd on`
  - separate subsystem service boundaries for EPS, ADCS, and COMM
  - bounded command/event/channel TT&C through GDS, gateway, physical serial ingress, COMM node `4`, and shared SocketCAN
  - bounded `EPS_SET_PDU(channel=2, enabled=true)` and `ADCS_SET_MODE(POINTING)` readbacks
  - live GPS UART source mode with advancing accepted-sentence count, without a valid-fix requirement
  - historical evidence includes housekeeping archive index plus two slot files
    byte-matched against target OBC runtime source snapshots
  - current reruns should record official `.fdp` / `DpCatalog` byte matches
    instead of new HK fallback assertions
  - repo-governed macOS ground launcher and operator runbook for the lab path
- Does **not** prove:
  - final flight deployment baseline
  - RF or real radio behavior
  - clean no-preamble first-byte behavior on the current physical UART link
  - arbitrary onboard file path downlink
  - end-to-end missing-packet retransmission, NACK/ARQ recovery, or reliable file transfer under packet loss
  - ScenarioBridge, `ground_pass_open`, `link_available`, or pass automation
  - final OS-level CAN provisioning
  - final dedicated non-login runtime identities such as `obc-runtime` or `subsystem-runtime`
- Governing evidence:
  - [docs/test-records/target-obc-comm-csp-lab-operational-baseline-v1/README.md](test-records/target-obc-comm-csp-lab-operational-baseline-v1/README.md)
- Supporting adjacent evidence:
  - [docs/test-records/comm-csp-socketcan-participation-v1/README.md](test-records/comm-csp-socketcan-participation-v1/README.md)
  - [docs/test-records/comm-csp-socketcan-file-downlink-v1/README.md](test-records/comm-csp-socketcan-file-downlink-v1/README.md)

### 33. Hosted onboard data-products and live beacon path

- Path:
  - hosted OBC runtime starts with the governed subsystem simulator stack and direct Native GDS command/downlink path
  - `OnboardStateSnapshotSource` reads existing cached subsystem/runtime state
  - `OnboardStateMonitor` reduces that state into onboard-only `ReducedStateV1`
  - `BeaconPublisher` emits `BeaconV1` frames through a COMM-facing raw broadcast sink
  - a hosted debug receiver captures and decodes the broadcast frame without storing mission `BEACON_HISTORY`
  - `HkTrendProductProducer` accumulates many HK samples and flushes finalized official F' product containers through `DpManager`
  - `DpWriter` writes official `.fdp` files under `<runtime-root>/data-products/`
  - `DpCatalog.BUILD_CATALOG` scans the official product directory
  - `DpCatalog.START_XMIT_CATALOG` queues pending products through `DpCatalogFileDownlinkGate` into stock `FileDownlink`
- Proven scope:
  - hosted live beacon broadcast capture and CRC decode with populated critical EPS raw measurements
  - 17-second beacon cadence with sequence advancement and no ground ACK/retry requirement
  - hosted 30-second HK trend sample capture plus finalized chunked official F' data product file generation
  - official `DpWriter` / `DpCatalog` integration under the governed `data-products` runtime root
  - `DpCatalogFileDownlinkGate` filters shared `FileDownlink.FileComplete` callbacks by request context so unrelated file completions cannot advance the data-product catalog
  - absence of repo-local `catalog.csv` and mission `BEACON_HISTORY` files in the new baseline
  - original component telemetry/event paths through Native GDS remain distinct from this beacon/product path
  - new real components retain classic F' L2 harness coverage and helper/support logic retains direct L1 coverage
- Does **not** prove:
  - real RF or vendor radio beacon broadcast
  - Raspberry Pi target-side SD/storage behavior
  - byte-matched GDS-received data product files
  - arbitrary onboard file path downlink
  - packet-loss recovery, NACK/ARQ, CFDP, segment retry, compression, or pass/window-aware downlink scheduling
  - `TlmPacketizer` telemetry packet/rate design
- Governing evidence:
  - [docs/test-records/onboard-data-products-and-live-beacon-v1/README.md](test-records/onboard-data-products-and-live-beacon-v1/README.md)

### 34. Hosted official FDP parity path

- Path:
  - hosted OBC runtime starts with isolated `<runtime-root>` and direct Native GDS command/downlink path
  - `HkTrendProductProducer` maps cached `OnboardStateSnapshotSource` state into chunked HK trend payloads
  - stock `DpManager` / `DpWriter` write official `.fdp` files under `<runtime-root>/data-products/`
  - `StorageHealthBridge` observes the same `data-products` runtime root as `DATA_PRODUCTS`
  - `DpCatalog.BUILD_CATALOG` scans official `.fdp` files
  - `DpCatalog.START_XMIT_CATALOG` queues catalog-selected products through `DpCatalogFileDownlinkGate` and stock `FileDownlink`
  - GDS receives a `.fdp` file under its file-storage directory
  - the probe byte-compares the GDS-received `.fdp` against the matching source file and decodes the received file
- Proven scope:
  - hosted official HK `.fdp` generation under the governed data-products root
  - data-products storage root visibility through storage-health state and telemetry contract
  - historical `HkTrendRecord` V2 payload decode with `version = 2`; later mode-model-v2 evidence updates mode semantics to payload `version = 3`, `hk-data-product-alignment-v2` advances the historical hosted CCSDS S-band payload to `version = 4`, the prior single-record active baseline used `version = 5`, and the current chunked baseline now uses `version = 6`
  - `DpCatalog` catalog build and controlled xmit queueing for official product files
  - GDS-received `.fdp` byte match against source runtime product file
  - no repo-local `data-products/catalog.csv`, OPD1 shim, mission `BEACON_HISTORY`, or arbitrary path downlink is part of the path
- Does **not** prove:
  - RF `.fdp` parity or vendor radio behavior
  - physical COMM `.fdp` byte-match
  - Raspberry Pi target-side SD/storage behavior
  - packet-loss recovery, NACK/ARQ, CFDP, segment retry, compression, or pass/window-aware downlink scheduling
  - backward decode of old V1, V2, or V3 `.fdp` files with the current dictionary
- Governing evidence:
  - [docs/test-records/hk-trend-chunked-fdp-v1/README.md](test-records/hk-trend-chunked-fdp-v1/README.md)
  - [docs/test-records/onboard-state-data-fdp-parity-v1/README.md](test-records/onboard-state-data-fdp-parity-v1/README.md)

### 35. Hosted primary mode model v2 path

- Path:
  - hosted `OBC` runtime starts from the native build with ground link disabled
  - operator CLI drives `status` and the guarded lowercase shell requests `mode safe`, `mode idle`, `mode payload`, `mode ttc`, and `mode hell`
  - retired, unknown, and mixed-case shell spellings remain parser errors
  - component/helper tests exercise `ModeManager`, onboard state snapshots, reduced state, live beacon mode encode/decode, and HK trend mode records
- Proven scope:
  - primary mode model v2 legal set and numeric values: `SAFE = 0`, `IDLE = 1`, `HELL = 2`, `PAYLOAD = 3`, `TTC = 4`
  - runtime CLI parsing/help/status and guarded `SYS_MODE_CHANGE` / `SYS_MODE_TRANSITION_REJECTED` presentation for v2 mode names
  - hosted shell operator admission now proves `SAFE -> IDLE` only when cached EPS SoC is greater than `50%`, rejects `SAFE -> IDLE` at `<= 50%`, and fails closed when the cached EPS status is unavailable
  - hosted shell operator admission now proves `IDLE -> PAYLOAD` only when cached EPS SoC is greater than `70%`, rejects `IDLE -> PAYLOAD` at `<= 70%`, and leaves `IDLE -> TTC` outside the new SoC admission rule
  - first-version shell topology remains `PAYLOAD -> SAFE`, `IDLE -> TTC`, and `TTC -> IDLE` through valid shell sequences; direct `SAFE -> PAYLOAD/TTC`, direct `PAYLOAD <-> TTC`, and operator entry to `HELL` remain rejected
  - operator shell cannot enter `HELL`; `mode hell` parses to the enum and is rejected by the guarded operator path unless already in `HELL`
  - rejection of retired primary mode CLI names such as `nominal`, `low-power`, `debug`, and `update`
  - hosted beacon/HK/snapshot data-surface handling of the v2 primary mode set without changing beacon wire size
  - beacon schema version `2` and HK trend payload `version = 3` distinguish v2 mode semantics from pre-v2 captures/products
  - active topology no longer schedules or runtime-binds the provisional `MissionExecutive` low-power interface
- Does **not** prove:
  - COMM, CCSDS, storage-policy, storage-health, or file/downlink behavior
  - FDIR, MissionExecutive/autonomy threshold policy, load shedding, or HELL/SAFE/IDLE hysteresis
  - UHF/S-band, RF, real radio, target hardware, reliable transfer, or pass scheduler behavior
- Governing evidence:
  - [docs/test-records/mode-model-v2-v1/README.md](test-records/mode-model-v2-v1/README.md)
  - [docs/test-records/payload-ttc-mode-entry-v1/README.md](test-records/payload-ttc-mode-entry-v1/README.md)
  - [docs/test-records/mode-soc-admission-and-exit-v1/README.md](test-records/mode-soc-admission-and-exit-v1/README.md)

### 36. Hosted SoC-driven mode safety policy path

- Path:
  - hosted native `OBC` runtime starts with ground link disabled and active topology-bound `ModeSafetyController`
  - repository-owned probe starts isolated `csp_zmqproxy`, `eps_simulator --initial-soc <pct>`, and `radio_mock_server` instances on alternate local ports
  - OBC observes simulator-owned EPS state through `EpsBridge` cached status, then `ModeSafetyController` requests fallback through `ModeManager`
  - focused helper, classic component, integration, and affected `EpsBridge` tests cover strict threshold behavior and non-fatal EPS critical-battery alarm behavior
- Proven scope:
  - `SAFE -> HELL` when cached EPS SoC is less than `10%`
  - `HELL -> SAFE` when cached EPS SoC is greater than `15%`, covered by helper/component/integration tests and internal safety path evidence rather than operator `mode hell` setup
  - `PAYLOAD -> IDLE` when cached EPS SoC is less than `60%`
  - `IDLE`, `PAYLOAD`, and `TTC -> SAFE` when cached EPS SoC is less than `40%`
  - for `PAYLOAD`, the `< 40%` transition to `SAFE` remains higher priority than the `< 60%` exit to `IDLE`
  - high-SoC `SAFE` remains `SAFE`; `SAFE -> IDLE` is manual
  - missing EPS cache and already-target conditions do not request duplicate or non-authoritative transitions
  - active topology uses `ModeSafetyController` rather than the provisional `MissionExecutive` for this policy
- Does **not** prove:
  - COMM split-link, CCSDS, storage, watchdog, broader FDIR, subsystem timeout/retry/reset, EPS load shedding, ADCS pointing/detumble, RF, target hardware, reliable transfer, payload scheduling, TTC pass automation, or TLE handling
  - any future automatic `SAFE -> IDLE` recovery or runtime threshold configurability
- Governing evidence:
  - [docs/test-records/mode-safety-policy-v1/README.md](test-records/mode-safety-policy-v1/README.md)
  - [docs/test-records/payload-ttc-mode-entry-v1/README.md](test-records/payload-ttc-mode-entry-v1/README.md)
  - [docs/test-records/mode-soc-admission-and-exit-v1/README.md](test-records/mode-soc-admission-and-exit-v1/README.md)

### 37. Hosted dual-link COMM simulator identity/coexistence path

- Path:
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted OBC runs as CSP node `1` with the ground link disabled
  - hosted `comm_csp_node` runs as generic compatibility COMM node `4`
  - hosted `sband_comm_csp_node` runs as S-band COMM node `5`
  - hosted `uhf_comm_csp_node` runs as UHF COMM node `6`
  - each COMM simulator identity uses a distinct PTY serial stand-in and the existing COMM CSP services `30` through `32`
- Proven scope:
  - explicit executable identities, startup logs, default node IDs, and CSP interface names for generic, S-band, and UHF hosted COMM simulators
  - generic node `4` compatibility identity remains available as `comm_csp_node`
  - hosted OBC node `1` can `csp ping` nodes `4`, `5`, and `6` on the same governed internal CSP substrate
  - each hosted COMM identity responds to `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS` through the focused service probe
  - generic, S-band, and UHF simulator identities can coexist in one hosted runtime without changing COMM service ports or packet layouts
- Does **not** prove:
  - complete S-band GDS path behavior
  - UHF UART/RS485/USB/macOS backup behavior
  - CCSDS behavior
  - RF behavior
  - reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
  - file/downlink behavior
  - target hardware behavior
  - Raspberry Pi deployment
- Governing evidence:
  - [docs/test-records/comm-dual-link-sim-foundation-v1/README.md](test-records/comm-dual-link-sim-foundation-v1/README.md)

### 38. Hosted S-band TCP gateway-backed COMM command/event/channel TT&C path

- Path:
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted `sband_comm_csp_node` runs as S-band COMM node `5` with a TCP listen endpoint
  - hosted `ground_ttc_gateway` keeps the stock GDS-facing TCP side and connects its southbound side to the S-band TCP endpoint with `--link-identity sband`
  - hosted OBC runs as CSP node `1` in `GROUND_LINK_MODE=comm-csp` and targets COMM node `5`
  - `fprime-cli` sends bounded commands through `fprime-gds -> ground_ttc_gateway -> S-band TCP -> sband_comm_csp_node(node 5) -> CSP -> hosted OBC`, and command events/channels return through the same S-band-through-COMM path
- Proven scope:
  - bounded command dispatch, command completion events, and channel readback through the S-band TCP gateway-backed COMM path
  - `sband_comm_csp_node` participates as node `5`; the proof is not direct `GDS -> TCP -> OBC`
  - the simulated S-band TCP segment is distinct from the GDS TCP endpoint
  - generic `comm_csp_node` node `4` compatibility and UHF node `6` foundation behavior remain out of the S-band proof scope
- Does **not** prove:
  - UHF UART/RS485/USB/macOS backup behavior
  - CCSDS behavior
  - RF behavior
  - reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
  - file/downlink behavior; cite entry 38 when received files and byte comparisons are part of the claim
  - target hardware behavior
  - Raspberry Pi deployment
  - arbitrary onboard file downlink
- Governing evidence:
  - [docs/test-records/sband-tcp-ground-link-v1/README.md](test-records/sband-tcp-ground-link-v1/README.md)

### 39. Historical hosted S-band TCP gateway-backed COMM HK fallback file/downlink path

- Path:
  - historical note: this entry records the retired HK fallback path and is not part of the current official `.fdp` baseline
  - reuses the hosted S-band TCP command/event/channel TT&C path in entry 37 as the command prerequisite
  - hosted OBC creates bounded housekeeping archive records through existing `HousekeepingArchive` command surfaces
  - `HK_DOWNLINK_INDEX` and selected `HK_DOWNLINK_SLOT` commands drive `FileDownlink -> COMM downlink write -> sband_comm_csp_node(node 5) -> S-band TCP -> ground_ttc_gateway -> fprime-gds file storage`
- Proven scope:
  - bounded housekeeping archive index and two selected occupied slot files downlink over the S-band TCP gateway-backed COMM path
  - received `hk-index.csv` and `hk-slot-*.bin` files byte-match source snapshots from the OBC runtime tree
  - no new COMM CSP service port, packet layout, or arbitrary file/downlink command is introduced
- Does **not** prove:
  - arbitrary onboard file path downlink
  - UHF UART/RS485/USB/macOS backup behavior
  - CCSDS behavior
  - RF behavior
  - reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
  - target hardware behavior
  - Raspberry Pi deployment
  - pass scheduling or contact automation
- Governing evidence:
  - [docs/test-records/sband-tcp-ground-link-v1/README.md](test-records/sband-tcp-ground-link-v1/README.md)

### 40. Hosted UHF serial gateway-backed COMM command/event/channel ingress path

- Path:
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted `uhf_comm_csp_node` runs as UHF COMM node `6` with a PTY-backed serial endpoint
  - hosted `ground_ttc_gateway` keeps the stock GDS-facing TCP side and connects its southbound side to the UHF PTY serial endpoint with `--link-identity uhf`
  - hosted OBC runs as CSP node `1` in `GROUND_LINK_MODE=comm-csp` and targets COMM node `6`
  - `fprime-cli` sends bounded commands through `fprime-gds -> ground_ttc_gateway -> hosted UHF serial -> uhf_comm_csp_node(node 6) -> CSP -> hosted OBC`, and command events/channels return through the same UHF-through-COMM path
- Proven scope:
  - bounded command dispatch, command completion events, and channel readback through the hosted UHF serial gateway-backed COMM path
  - `uhf_comm_csp_node` participates as node `6`; the proof is not direct `GDS -> TCP -> OBC`, generic node `4`, or S-band node `5`
  - hosted PTY serial is used as the UART/USB/RS485 stand-in
- Does **not** prove:
  - full UHF command authority or autonomous failover policy
  - file/downlink behavior
  - CCSDS behavior
  - RF behavior
  - reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
  - target hardware behavior
  - Raspberry Pi deployment
  - physical USB serial hardware or physical RS485 electrical behavior
- Governing evidence:
  - [docs/test-records/uhf-uart-backup-link-v1/README.md](test-records/uhf-uart-backup-link-v1/README.md)

### 41. Hosted UHF node-6 BeaconV1 side-channel capture path

- Path:
  - hosted OBC BeaconPublisher emits BeaconV1 frames to the UHF CSP beacon side channel
  - hosted `uhf_comm_csp_node` runs as UHF COMM node `6` and receives BeaconV1 frames on the bounded beacon push service
  - `uhf_comm_csp_node` writes the BeaconV1 frame to a separate hosted PTY serial beacon endpoint for capture/decode
- Proven scope:
  - BeaconV1 frame capture from the UHF node-6 side channel
  - decoded BeaconV1 schema version, fixed wire size, CRC, and representative populated state fields
  - beacon evidence remains separate from command ingress evidence
- Does **not** prove:
  - command response behavior
  - session-aware beacon suppress/runtime; cite entry 41A for that dedicated proof
  - full UHF command authority or autonomous failover policy
  - file/downlink behavior or arbitrary downlink
  - CCSDS behavior
  - RF behavior
  - reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
  - target hardware behavior
  - Raspberry Pi deployment
  - physical USB serial hardware or physical RS485 electrical behavior
- Governing evidence:
  - [docs/test-records/uhf-uart-backup-link-v1/README.md](test-records/uhf-uart-backup-link-v1/README.md)

### 41A. Hosted UHF node-6 beacon suppress/runtime compatibility path

- Path:
  - `fprime-cli` sends routed commands through `fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay or serial southbound) -> uhf_comm_csp_node(node 6) -> CSP -> hosted OBC`
  - hosted `CommController` owns the active UHF session boundary and the bounded beacon suppress gate
  - hosted `BeaconPublisher` feeds the bounded beacon side channel only when `CommController` leaves suppress cleared
  - hosted `uhf_comm_csp_node` writes BeaconV1 frames to the hosted beacon serial capture endpoint for probe-owned decode
- Proven scope:
  - historical hosted beacon visibility before suppress starts
  - historical hosted suppress start after accepted authenticated UHF `SESSION_OPEN(seq0)` on the retained legacy path
  - historical hosted accepted same-session UHF command activity, including read/status, refreshes the bounded active window
  - fixed `60`-tick inactivity timeout resume on the current `1 Hz` baseline
  - accepted S-band activity and rejected/non-qualifying UHF traffic do not trigger suppress
  - hosted proof keeps `CommController` as policy owner and `BeaconPublisher` as session-agnostic cadence/encode owner
- Does **not** prove:
  - preferred secure-baseline suppress start at auth-success session synthesis
  - legacy command-envelope retirement
  - simultaneous dual-link runtime arbitration
  - UHF reliable transfer, ARQ, NACK, or CFDP
  - RF behavior
  - target hardware or Raspberry Pi deployment
  - broader UHF handshake state beyond the accepted `SESSION_OPEN(seq0)` compatibility boundary
- Governing evidence:
  - [docs/test-records/uhf-beacon-suppression-runtime-v1/README.md](test-records/uhf-beacon-suppression-runtime-v1/README.md)

### 41B. Hosted UHF primary packet-quiet compatibility path

- Path:
  - hosted default `OBC` keeps the active `TopCcsds` baseline and starts with the normal S-band primary role state
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted `ground_ttc_gateway` keeps separate stock GDS-facing TCP sides for the governed S-band and UHF hosted proofs
  - hosted `sband_comm_csp_node` runs as COMM node `5` and hosted `uhf_comm_csp_node` runs as COMM node `6`
  - `CommController` switches the active primary role set to UHF
  - `CommEgressMux` applies the formal UHF primary packet-quiet gate while official file/downlink routing remains available
- Proven scope:
  - historical hosted packet quiet suppresses live formal UHF `event/tlm` packet egress before any accepted qualifying legacy `SESSION_OPEN(seq0)`
  - accepted qualifying legacy `SESSION_OPEN(seq0)` still belongs to the separate beacon suppress/runtime path rather than starting packet quiet
  - official file/data-product downlink remains formal while UHF primary packet quiet is active
- Does **not** prove:
  - current quiet versus nonquiet authority; use
    `uhf-primary-nonquiet-runtime-v1` for that question
  - legacy command-envelope retirement
  - simultaneous dual-link runtime arbitration
  - UHF reliable transfer, ARQ, NACK, or CFDP
  - RF behavior
  - broader target non-quiet closure
- Governing evidence:
  - [docs/test-records/uhf-primary-packet-quiet-decoupling-v1/README.md](test-records/uhf-primary-packet-quiet-decoupling-v1/README.md)

### 42. Hosted CCSDS S-band node-5 gateway-backed spike command/event/telemetry and file/downlink path

- Path:
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted `sband_comm_csp_node` runs as S-band COMM node `5` with a TCP listen endpoint
  - hosted `ground_ttc_gateway` keeps a raw byte relay between `fprime-gds` and the S-band TCP endpoint
  - `fprime-gds` runs with `space-packet-space-data-link` framing, SCID `0x44`, VCID `1`, and TM frame size `4096`
  - hosted `OBC_CcsdsGroundLinkSpike` runs as CSP node `1` in `GROUND_LINK_MODE=comm-csp` and targets COMM node `5`
  - `fprime-cli` sends bounded commands through `fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> CSP -> OBC_CcsdsGroundLinkSpike`
  - historical command events, telemetry, and retired HK fallback files return
    through the same CCSDS-framed S-band-through-COMM path
- Proven scope:
  - bounded command dispatch and completion for `EPS_SET_PDU` and `ADCS_SET_MODE`
  - event and telemetry return over the CCSDS-hosted S-band path, including `GROUND_LINK_TX_BYTES`
  - bounded `HK_DOWNLINK_INDEX` and two selected `HK_DOWNLINK_SLOT` files byte-match OBC runtime source snapshots as historical retired-fallback evidence
  - gateway compatibility is transparent raw byte movement of CCSDS-framed traffic only
  - APID flow coverage for command `0`, telemetry `1`, log/event `2`, and file `3` through the upstream `ComCcsds`/`ComCfg.Apid` path
- Does **not** prove:
  - default `OBC` topology migration from `ComFprime` to `ComCcsds`
  - UHF CCSDS behavior
  - RF behavior
  - reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
  - target hardware behavior
  - Raspberry Pi deployment
  - command authority, failover policy, pass scheduling, or contact automation
  - arbitrary onboard file downlink
  - independent APID sequence-counter decoding outside the stock `ComCcsds.apidManager` implementation
- Governing evidence:
  - [docs/test-records/ccsds-ground-link-spike-v1/README.md](test-records/ccsds-ground-link-spike-v1/README.md)

### 43. Default hosted CCSDS S-band node-5 adoption path

- Path:
  - hosted default `OBC` imports the CCSDS communication topology while retaining the operator namespace `OBCApp.*`
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted `sband_comm_csp_node` runs as S-band COMM node `5` with a TCP listen endpoint
  - hosted `ground_ttc_gateway` keeps a transparent raw byte relay between `fprime-gds` and the S-band TCP endpoint and can tee both relay directions for verification-only capture
  - `fprime-gds` runs with `space-packet-space-data-link` framing, SCID `0x44`, VCID `1`, and TM frame size `4096`
  - `fprime-cli` sends bounded commands through `fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> CSP -> OBC`
  - historical command events, telemetry, and retired HK fallback files returned through the same CCSDS-framed S-band-through-COMM path; current official `.fdp` parity is entry 44
- Proven scope:
  - default hosted `OBC` adoption of the CCSDS S-band path
  - preserved default operator namespace `OBCApp.*`
  - bounded command dispatch and completion for `EPS_SET_PDU` and `ADCS_SET_MODE`
  - bounded command dispatch and completion for guarded `OBCApp.modeManager.MODE_SET`, including SoC-gated `SAFE -> IDLE`, SoC-gated `IDLE -> PAYLOAD`, unchanged `IDLE -> TTC`, rejected transition events for threshold or unavailable-cache failures, and bounded `SYS_MODE` telemetry readback over the same default CCSDS S-band path
  - event and telemetry return over the CCSDS-hosted S-band path, including `GROUND_LINK_TX_BYTES`
  - historical bounded `HK_DOWNLINK_INDEX` and two selected `HK_DOWNLINK_SLOT` files byte-match OBC runtime source snapshots
  - decoded raw relay capture shows expected SCID, VCID, TM frame size, TC/TM traffic, and APID flows for command `0`, telemetry `1`, event/log `2`, and file `3`
- Does **not** prove:
  - rerunnable current secure-auth hosted closeout authority on the branch-head
    retirement baseline; the retained wrapper entrypoint is now historical-only
    because it still depended on public `SESSION_OPEN`
  - UHF CCSDS behavior
  - RF behavior
  - reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
  - target hardware behavior
  - Raspberry Pi deployment
  - command authority, failover policy, pass scheduling, or contact automation
  - arbitrary onboard file downlink
  - repository-level reliable transfer from CCSDS sequence counts
- Governing evidence:
  - [docs/test-records/ccsds-sband-hosted-adoption-v1/README.md](test-records/ccsds-sband-hosted-adoption-v1/README.md)

### 43A. Default hosted UHF CCSDS node-6 adoption path

- Path:
  - hosted default `OBC` keeps the active `TopCcsds` baseline and starts with the normal S-band primary role state
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted `uhf_comm_csp_node` runs as UHF COMM node `6` with a PTY-backed serial endpoint
  - hosted `ground_ttc_gateway` keeps the stock GDS-facing TCP side and connects its southbound side to the UHF PTY serial endpoint with `--link-identity uhf`
  - `fprime-gds` runs with `space-packet-space-data-link` framing, SCID `0x44`, VCID `2`, and TM frame size `4096`
  - hosted COMM runtime explicitly switches the active command / telemetry / file roles to UHF before the bounded proof traffic is sent
  - after that switch, `fprime-cli` sends bounded commands through `fprime-gds(CCSDS) -> ground_ttc_gateway(link=uhf serial) -> hosted serial -> uhf_comm_csp_node(node 6) -> CSP -> default hosted OBC`
- Proven scope:
  - active `TopCcsds` UHF command/file/downlink path is CCSDS-backed and no longer depends on local `OBCComFprime`
  - bounded command uplink and bounded `.fdp` file/downlink work on the active hosted UHF primary path
  - decode artifacts show `SCID 0x44`, `VCID 2`, `TM frame size 4096`, uplink APID `0`, and the governed file/downlink APID class required by the current hosted proof
- Does **not** prove:
  - rerunnable current secure-auth hosted closeout authority on the branch-head
    retirement baseline; the retained wrapper entrypoint is now historical-only
    because it still depended on public `SESSION_OPEN`
  - pre-switch physical node-`6` `uhf-backup` secure-auth bootstrap or
    bounded read/status continuity; cite entry 43E for that dedicated proof
  - current formal UHF live `event/tlm` visibility once UHF-primary packet quiet is part of the active baseline
  - current band-driven UHF primary packet-quiet semantics
  - RF behavior
  - reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
  - target hardware behavior
  - Raspberry Pi deployment
  - broader command-authority expansion or legacy retirement
- Governing evidence:
  - [docs/test-records/uhf-ccsds-hosted-adoption-v1/README.md](test-records/uhf-ccsds-hosted-adoption-v1/README.md)

### 43B. Hosted maintained per-band stock ground/operator baseline

- Path:
  - one shared hosted `OBC` / `TopCcsds` runtime stays under one owned runtime root
  - one stock `fprime-gds(CCSDS)` plus one `ground_ttc_gateway(raw relay)` exposes the S-band node-`5` operator surface
  - one stock `fprime-gds(CCSDS)` plus one `ground_ttc_gateway(raw relay)` exposes the UHF node-`6` operator surface
  - one combined repo-owned wrapper may start and stop both stacks together while keeping the operator surfaces distinct
- Proven scope:
  - maintained repo-owned launcher entrypoints exist for hosted S-band only, hosted UHF only, and combined start/stop composition
  - launcher manifests make reviewable the per-band GDS/TTS ports, southbound endpoints, file stores, runtime roots, owned logs, startup order, and shutdown order
  - the combined wrapper keeps separate stock GDS surfaces, separate gateway processes, and separate southbound paths on the same hosted runtime
  - launcher reruns keep unrelated active EPS/ADCS simulator-backed hosted runs alive when those runs use different CSP hub ports, while still allowing orphan-leftover cleanup for shared simulator identities
- Does **not** prove:
  - one stock GDS consuming heterogeneous upstream feeds
  - one `ground_ttc_gateway` instance multiplexing simultaneous S-band and UHF
  - simultaneous dual-link runtime arbitration
  - S-band or UHF command/file semantics by itself; cite the dedicated S-band and UHF entries when those behaviors matter
  - target/lab simultaneous S-band plus UHF proof
  - RF behavior
- Governing evidence:
  - [docs/test-records/per-band-stock-ground-stacks-v1/README.md](test-records/per-band-stock-ground-stacks-v1/README.md)
  - [docs/test-records/hosted-simulator-stale-reap-safety-v1/README.md](test-records/hosted-simulator-stale-reap-safety-v1/README.md)

### 43C. Hosted dual-link orchestration owner path

- Path:
  - one hosted-only thin lifecycle owner starts above the maintained `43B`
    per-band stock-stack baseline
  - the owner delegates one shared hosted `TopCcsds` runtime plus distinct
    S-band and UHF stock `fprime-gds` + `ground_ttc_gateway` surfaces to the
    maintained layer-1 baseline beneath it
  - the owner writes orchestration-owned manifest, status, lifecycle-history,
    failure, and cleanup artifacts for the combined hosted surface
- Proven scope:
  - a distinct hosted layer-2 owner exists above `43B`
  - the owner records `preflight -> starting -> ready -> stopping -> stopped`
    for the happy path
  - the owner records a bounded `startup_failed` result for a declared
    startup-surface conflict
  - the owner records listener-closure cleanup summary for its declared hosted
    surface without relying on older COMM semantic oracles
- Does **not** prove:
  - command authority ownership
  - gateway relay ownership or one-gateway multiplexer behavior
  - one-stock-GDS heterogeneous multi-upstream behavior
  - COMM runtime ownership inside OBC
  - simultaneous dual-link runtime arbitration on target hardware
  - RF behavior
- Governing evidence:
  - [docs/test-records/comm-dual-link-orchestration-v1/README.md](test-records/comm-dual-link-orchestration-v1/README.md)

### 43D. Hosted switched UHF node-6 reliable-transfer path

- Path:
  - one shared hosted `OBC` / `TopCcsds` runtime stays under a launcher-owned
    runtime root
  - the maintained per-band stock launcher exposes distinct S-band and UHF
    stock `fprime-gds(CCSDS)` plus `ground_ttc_gateway(raw relay)` surfaces
  - hosted default role state still bootstraps on S-band node `5`
  - the reliable-transfer proof explicitly switches the active file path to
    `uhf-primary-after-failover` on node `6`
  - the repo-owned node-`6` reliable-transfer receiver writes output under the
    governed hosted proof root instead of stock GDS file storage
- Proven scope:
  - happy-path official HK `.fdp` whole-file reliable transfer on the explicit
    switched hosted UHF node-`6` path
  - one resend-before-success hosted case on the same bounded path
  - one retry-exhausted hosted case with no final artifact promotion
  - the helper `160`-byte `DATA` segment ceiling stays a helper-path fact, not
    a generic repo MTU claim
- Does **not** prove:
  - `uhf-backup` reliable transfer
  - automatic failover-to-UHF reliable transfer
  - generic arbitrary-file authority
  - target/lab hardware behavior
  - RF behavior
  - broad CFDP, one-GDS aggregation, one-gateway multiplexing, or generic
    simultaneous closure
- Governing evidence:
  - [docs/test-records/uhf-reliable-transfer-v1/README.md](test-records/uhf-reliable-transfer-v1/README.md)

### 43E. Hosted challenge-auth secure-command path

- Path:
  - one shared hosted `OBC` / `TopCcsds` runtime stays under a probe-owned
    runtime root
  - the maintained per-band stock launcher exposes distinct S-band and UHF
    stock `fprime-gds(CCSDS)` plus `ground_ttc_gateway(raw relay)` surfaces
  - a repo-local `security-server-sim` on macOS returns `sKey` by
    `GetSessionKey(serviceId, challenge)` and does not alter GDS UI
  - S-band secure auth uses `ServiceID = 1` over the default hosted node-`5`
    surface
  - UHF secure auth uses `ServiceID = 2` over the default hosted physical
    node-`6` `uhf-backup` surface before any explicit switch to UHF primary
  - handshake packets use APID `0x00FE`, and post-auth secure command v2 keeps
    APID `0x0000`
  - successful auth synthesizes repo-internal opened-session truth without a
    wire-level `SESSION_OPEN`, then bounded secure commands continue through
    `CommandIngressAuthority`
  - later explicit promotion to `uhf-primary-after-failover` revokes the old
    UHF auth/session state and requires re-auth before a higher-authority
    command may succeed
- Proven scope:
  - hosted reviewable `ReqAuth -> Challenge -> Response -> Authenticated`
    bootstrap on the current comm-managed S-band and UHF surfaces
  - S-band secure auth plus secure command v2 `EPS_GET_STATUS(seq=1)`
  - default hosted pre-switch physical node-`6` `uhf-backup` secure auth plus
    bounded read/status continuity
  - current hosted `uhf-backup` authority boundary remains intact after auth:
    high-authority `MODE_SET` stays denied
  - `uhf-primary-after-failover` requires re-auth after role invalidation
    before higher-authority command acceptance
  - `180 s` inactivity-timeout clearing of secure auth/session state
- Does **not** prove:
  - encryption
  - target/lab or RF behavior
  - `uhf-backup` reliable transfer
  - generic file or unknown-packet uplink authority
  - hardware-backed key storage, persistent secure key storage, or full secure
    boot
  - full replay protection outside challenge-auth bootstrap plus strict active
    secure-session sequence
  - removal of legacy command envelope v1
- Governing evidence:
  - [docs/test-records/challenge-handshake-secure-command-v1/README.md](test-records/challenge-handshake-secure-command-v1/README.md)
  - Review note: the evidence record explicitly tracks the narrow
    `lib/fprime/Svc/Subtopologies/ComCcsds/*` delta required to add formal
    handshake downlink queueing for `FW_PACKET_HAND`, and notes that the
    parent repo carries it through a pinned forked submodule commit

### 43F. Hosted staged-file and unknown-uplink authority closure on keystore-backed auth

- Path:
  - one shared hosted `OBC` / `TopCcsds` runtime stays under a probe-owned
    runtime root
  - the maintained per-band stock launcher exposes distinct S-band and UHF
    stock `fprime-gds(CCSDS)` plus `ground_ttc_gateway(raw relay)` surfaces
  - hosted OBC and repo-owned ground helper tooling both load
    `config/security/command-auth.ini`
  - `SecureLinkAuthorizer` remains the only consumer of comm-managed unknown
    uplink and admits only handshake APID `0x00FE`
  - `FileIngressAuthority` governs `.sequence-staging/<leaf>` file upload and
    now requires both active secure auth and current runtime file-role allow
  - S-band staged upload uses secure auth `ServiceID = 1`
  - UHF staged upload stays denied on `uhf-backup`, then becomes admissible
    only after explicit promotion to `uhf-primary-after-failover` plus re-auth
- Proven scope:
  - hosted comm-managed secure auth reads the shared tracked keystore asset
    instead of hosted runtime key injection, and the hosted OBC runtime no
    longer exposes a keystore override flag
  - malformed or unsupported APID `0x00FE` handshake traffic is rejected
    without creating or mutating auth state
  - S-band secure auth plus `.sequence-staging/<leaf>` upload succeeds and the
    admitted official sequence wrapper path remains reviewable
  - UHF backup secure auth plus staged upload attempt is denied
  - explicit-switched `uhf-primary-after-failover` re-auth plus staged upload
    succeeds
- Does **not** prove:
  - target/lab or RF behavior
  - generic arbitrary file-uplink authority beyond `.sequence-staging/<leaf>`
  - generic unknown-uplink authority beyond APID `0x00FE`
  - encryption
  - persistent secure key storage or hardware-backed key storage
- Governing evidence:
  - [docs/test-records/uplink-authority-and-key-hardening-v1/README.md](test-records/uplink-authority-and-key-hardening-v1/README.md)

### 43G. Hosted node-5 S-band observability tier-selection path

- Path:
  - one shared hosted `OBC` / `TopCcsds` runtime stays under a probe-owned
    runtime root
  - the maintained per-band stock launcher exposes distinct S-band and UHF
    stock `fprime-gds(CCSDS)` plus `ground_ttc_gateway(raw relay)` surfaces
  - S-band secure auth uses `ServiceID = 1` over the default hosted node-`5`
    surface
  - hosted proof observes packetized live `event/tlm` only through the
    governed node-`5` S-band path and uses bounded UHF adjacency only for
    close/invalidated-session proof
- Proven scope:
  - pre-auth node-`5` S-band packetized live `event/tlm` stays quiet at
    startup
  - accepted S-band secure auth opens hosted node-`5` curated live summary
    visibility rather than the old wholesale scheduled surface
  - the fresh public-release proof uses periodic
    `GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS` as the representative
    ground-link-health visibility oracle; it does not require a duplicate
    `GROUND_LINK_HEALTH_S_BAND_AVAILABLE` sample because that channel is
    formally `update on change`
  - representative authenticated ambient summary now tracks the current
    operator-facing keep-live set instead of the older all-families scheduled
    assumption: the hosted proof requires ambient `EPS` and `ADCS` summary
    fields plus retained reviewable comm/resource surfaces, but does not
    require ambient `GPS`, `RADIO`, or detailed `STORAGE` telemetry
  - one representative bounded detailed readback,
    `EPS_GET_STATUS -> EPS_POWER_OUT`, remains available on that same
    authenticated node-`5` path without reopening broad live chatter, and the
    detailed readback remains reviewable through a bounded command-specific
    packet-path artifact instead of relying only on a long-running passive
    channel listener
  - bounded cached readback such as `GET_RESET_CAUSE` also remains available on
    the same authenticated node-`5` path
  - explicit switch to `uhf-primary-after-failover` closes the old node-`5`
    live-observability gate rather than leaving ambient S-band chatter open,
    and the governed hosted proof confirms that passive S-band observer output
    stops after close while the packet capture remains a reviewable adjunct
    artifact rather than a raw-size quiet gate
- Does **not** prove:
  - pre-auth broad S-band packet chatter as a maintained requirement
  - that every diagnostics-only residual channel has been removed from runtime
    output
  - any reinterpretation of `GROUND_LINK_TX_BYTES`, `GROUND_LINK_UP/DOWN`,
    `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
    `CSP_OWNER_TIMEOUT` / `CSP_OWNER_TOTAL_TIMEOUTS`, or S-band
    `CommEgressMux` counters as mere diagnostics; those remain formal
    reviewable observability on this branch
  - a second summary-routing plane or auth-free `GET_*` readback class
  - target/lab or RF behavior
  - UHF reliable transfer, broad non-quiet UHF observability, or one-GDS
    heterogeneous aggregation
- Governing evidence:
  - [docs/test-records/sband-live-observability-tier-selection-v1/README.md](test-records/sband-live-observability-tier-selection-v1/README.md)
    remains the last fully requalified hosted proof for the representative
    detailed `GET_*` packetized readback path
  - [docs/test-records/node5-observability-residual-cleanup-v1/README.md](test-records/node5-observability-residual-cleanup-v1/README.md)
    is the active same-change rebuild record for residual governance and
    detailed `GET_*` requalification on this branch until fresh hosted rerun
    closes again
  - [docs/test-records/node5-proof-oracle-hardening-v1/README.md](test-records/node5-proof-oracle-hardening-v1/README.md)
    records the packet-path quiet requalification follow-up on the maintained
    hosted proof
  - [docs/test-records/public-thesis-submission-v1/README.md](test-records/public-thesis-submission-v1/README.md)
    records the fresh public-tree rerun and the bounded `update on change`
    oracle correction

### 44. Default hosted CCSDS S-band official FDP parity path

- Path:
  - hosted default `OBC` imports the CCSDS communication topology while retaining the operator namespace `OBCApp.*`
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted `sband_comm_csp_node` runs as S-band COMM node `5` with a TCP listen endpoint
  - node `5` may probe `39 DOWNLINK_CONTROL_V3(STATUS)` and, when available, use `39 DOWNLINK_CONTROL_V3` plus one-way `40 DOWNLINK_DATA_V3` below the same stock `DpCatalog -> CommController -> FileDownlink` ownership boundary; `31 DOWNLINK_WRITE` remains the fallback transport
  - hosted `ground_ttc_gateway` keeps a transparent raw byte relay between `fprime-gds` and the S-band TCP endpoint
  - `fprime-gds` runs with `space-packet-space-data-link` framing, SCID `0x44`, VCID `1`, and TM frame size `4096`
  - `HkTrendProductProducer` maps cached `OnboardStateSnapshotSource` state into `HkTrendRecord` V6 sample arrays plus `HkTrendChunkMeta`
  - stock `DpManager` / `DpWriter` write official `.fdp` files under `<runtime-root>/data-products/`
  - `DpCatalog.BUILD_CATALOG` scans official `.fdp` files
  - `DpCatalog.START_XMIT_CATALOG` queues catalog-selected products through `DpCatalogFileDownlinkGate` and stock `FileDownlink`
  - GDS receives a `.fdp` file through `OBC -> sband_comm_csp_node(node 5) -> S-band TCP -> ground_ttc_gateway(raw relay) -> fprime-gds(CCSDS)`
  - the probe byte-compares the GDS-received `.fdp` against the matching source file and decodes the received file with the current dictionary
- Proven scope:
  - default hosted CCSDS S-band official HK `.fdp` generation, cataloging, xmit queueing, byte-match, and decode
  - current `HkTrendRecord` payload decode with `version = 6` plus `HkTrendChunkMeta` record decode
  - `DpCatalog` remains the official `.fdp` catalog/index surface for this path
  - the maintained node-`5` uplift only proves transport decoupling below stock `FileDownlink`; it does not redefine the formal official `.fdp` path family
  - no repo-local `data-products/catalog.csv`, OPD1 shim, mission `BEACON_HISTORY`, or arbitrary path downlink is part of the path
- Does **not** prove:
  - direct Native GDS `.fdp` parity for this V6 evidence; cite entry 33 for the older direct path baseline
  - RF `.fdp` parity or vendor radio behavior
  - UHF CCSDS behavior
  - physical COMM `.fdp` byte-match
  - Raspberry Pi target-side SD/storage behavior
  - packet-loss recovery, NACK/ARQ, CFDP, segment retry, compression, or pass/window-aware downlink scheduling
  - arbitrary onboard file path downlink
  - backward decode of historical V1/V2/V3/V4/V5 `.fdp` files with the V6 dictionary
- Governing evidence:
  - [docs/test-records/hk-trend-chunked-fdp-v1/README.md](test-records/hk-trend-chunked-fdp-v1/README.md)
  - [docs/test-records/hk-data-product-alignment-v2/README.md](test-records/hk-data-product-alignment-v2/README.md)
  - [docs/test-records/node5-comm-csp-downlink-v2/README.md](test-records/node5-comm-csp-downlink-v2/README.md)
  - branch-local focused proof `bash scripts/run_node5_comm_csp_downlink_v2_transport_hosted_probe.sh` now verifies the node-`5` `v2` transport layer in isolation against a slow local TCP sink, including byte-match and observable accepted-before-flushed queue gap below the stock `DpCatalog -> CommController -> FileDownlink` ownership boundary; it is not, by itself, a requalification of the full hosted CCSDS `ground_ttc_gateway -> fprime-gds` official `.fdp` parity path
  - [docs/test-records/node5-comm-csp-downlink-v3/README.md](test-records/node5-comm-csp-downlink-v3/README.md)
  - branch-local focused proof `bash scripts/run_node5_comm_csp_downlink_v3_transport_hosted_probe.sh` verifies the node-`5` `v3` transport layer in isolation against a slow local TCP sink using the repo-local `2048/24/32/32/16` libcsp sizing baseline plus the `FW_FILE_BUFFER_MAX_SIZE=2032` / `DOWNLINK_V3_MAX_DATA_BYTES=2032` one-frame-per-stock-file-buffer envelope; the current 2026-07-08 rerun requalified this focused path after fixing a hosted-only `csp_zmqproxy` capture-task `1024`-byte limit
  - branch-local rerun `bash scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh` now requalifies the maintained hosted official CCSDS `.fdp` parity path on the `2048/2032` `v3` envelope with node `5` selecting `v3`
  - branch-local target debug companion `bash scripts/run_node5_comm_csp_downlink_v3_target_adaptive_search.sh` now exists to reuse the faster maintained secure-auth target path as a transport comparator with probe-only `COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES`, `COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE`, `COMM_GROUNDLINK_DOWNLINK_V3_INTERFRAME_DELAY_USEC`, and `COMM_CSP_SOCKETCAN_TX_FRAME_DELAY_USEC` overrides; it is a current debug aid for classifying `size/window/pacing` failure modes on real SocketCAN, not by itself a requalification of any official `.fdp` path
  - the adaptive comparator also preserves node-`5` `downlink-v3-data-drop packet-length=<n>` observations because the previous V3 inter-frame delay only spaces whole CSP packets and cannot isolate a failure that already occurs inside one large CSP packet's own CAN-fragment burst
  - this adaptive-search comparator is the earlier pre-CANFD-override phase for the target `2032` slowdown investigation; its historical `240/416/1000/2032` evidence must not be retroactively explained by the later probe-owned CAN FD override owner bug
  - historical 2026-07-08 adaptive-search evidence tightened the working interval from the coarse `240 pass / 1000 fail / 2032 fail` anchors down to at least `240 pass / 416 fail`, and captured target-side partial-ingress shapes such as node-`5` accepting `frame-index=1` without the prefix frames; this remains useful debugging ancestry, but it is no longer the latest branch-head target truth
  - current 2026-07-09 branch-local governed reruns of the maintained secure-auth comparator now sharpen the actual target requirement: default `2032/window=3/no-CANFD` fails, forced `2032/window=1/no-CANFD` also fails, and default `2032/window=3` passes only after adding the scoped node-`5`/`6` COMM CAN FD override; evidence roots: `/private/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.QqDAZL`, `/tmp/target-secure-auth-v3-2032-window1-no-canfd`, `/tmp/target-secure-auth-v3-2032-window3-canfd`
  - branch-local target debug companion `bash scripts/run_node5_comm_csp_downlink_v3_hk_target_probe.sh` now exercises the stock official HK `.fdp` path with a non-trivial `HK_TREND_TARGET_FILE_BYTES=10240` sample and rejects runs whose generated `.fdp` stays below `8192` bytes; it is the current official-file comparator below the slower payload preview/raw path
  - current 2026-07-09 governed rerun of that HK comparator passes in reuse mode with `node5-transport=v3` and byte-match on an `8498`-byte source `.fdp`; evidence root: `/tmp/node5-v3-hk-target-reuse-r8`
  - branch-head target ownership promotes that scoped node-`5`/`6`, port-`40`
    CAN FD condition to the governed A-layer baseline: A installs/verifies the
    OBC/S-band/UHF service profile, while C only verifies and consumes it;
    EPS/ADCS stay on classical CAN traffic
  - this is CAN FD frame-format evidence only: libcsp does not set `CANFD_BRS`,
    so BRS, measured 2 Mbit/s data-phase, RF, and OTA closure remain non-claims
  - current 2026-07-09 branch-local governed rerun `bash scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh` now also passes in archived-artifact reuse mode on that same scoped CAN FD condition set, with `source-provisioning-mode=reuse-existing-summary`, preview/raw family byte-match, and preview/raw artifact-hash closure; evidence root: `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-r3`
  - that wrapper now treats archived payload-family source summaries as the canonical historical-source oracle for reuse mode; if current `payload_fdp_extract.py` cannot re-decode the archived source family, the wrapper falls back to archived `sourceFamilySummary` plus family-level SHA-256 equality instead of falsely declaring the maintained target downlink broken

### 45. Hosted legacy command ingress authority compatibility path (historical only)

- Historical-only note:
  - this entry records archived legacy command-envelope v1 / `SESSION_OPEN`
    compatibility evidence
  - current branch-head runtime no longer accepts public `SESSION_OPEN` and no
    longer treats this family as a maintained closeout gate
  - cite this entry only when a change explicitly needs historical legacy
    compatibility ancestry
  - the retained hosted official-sequencing wrapper defaults to refusal unless
    explicitly invoked with `ALLOW_HISTORICAL_WRAPPER=1`

- Path:
  - hosted default `OBC` imports the CCSDS communication topology while retaining the operator namespace `OBCApp.*`
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted `sband_comm_csp_node` runs as S-band COMM node `5` with a TCP listen endpoint
  - hosted `ground_ttc_gateway` keeps a transparent raw byte relay between `fprime-gds` and the S-band TCP endpoint
  - `fprime-gds` runs with `space-packet-space-data-link` framing, SCID `0x44`, VCID `1`, and TM frame size `4096`
  - `fprime-cli` sends bounded commands through the existing default hosted CCSDS S-band routed `Fw.Com` command path
  - hosted OBC supplies an explicit `CommandIngressAuthority` authority profile and keystore-backed legacy auth tuple for each proof run: `sband-primary` or `uhf-backup`
  - current hosted default CCSDS topology wires authority ingress index `0` only
  - `CommandIngressAuthority` sits between `ComCcsds.fprimeRouter.commandOut` and `CdhCore.cmdDisp.seqCmdBuff`, and returns status through the mirrored upstream status path
  - repo-owned envelope injector can send an outer `FW_PACKET_COMMAND` using project pseudo-opcode `0x0BC10001` through the same hosted GDS TTS path
  - recognized command envelope v1 packets carry authenticated `source_id`, `key_slot`, `session_id`, `sequence_number`, inner payload, and `HMAC-SHA256` MAC material
  - `CommandIngressAuthority` parses recognized command envelopes, verifies auth first, then evaluates authority on the inner opcode, then applies lifecycle and sequence checks before forwarding only the inner F Prime command to `CmdDispatcher`
  - `CommandIngressAuthority` accepts `SESSION_OPEN(sequence=0)` as the only v1 session open / replace / resync surface for one active session per source epoch
- Proven scope:
  - historical hosted OBC-side legacy command ingress authority enforcement for routed `Fw.Com` command packets before `Svc::CommandDispatcher`
  - topology-configured source-index mapping for the currently hosted ingress port `0`
  - authenticated envelope verification for the currently hosted routed ingress port `0`, with tracked-keystore-backed shared-key config and source-bound `HMAC-SHA256`
  - `sband-primary` configured authority allows representative high-authority command execution through `CmdDispatcher`
  - `uhf-backup` configured authority allows a read/status command and rejects a representative high-authority mode command before `CmdDispatcher`
  - rejected command evidence includes `COMMAND_AUTHORITY_REJECTED` with ingress port and link identity fields, plus absence of downstream command dispatch/mode-change events for the denied command
  - authority profiles are explicit OBC configuration by ingress port index, not inferred from `Fw.Com.context` or gateway metadata
  - enveloped S-band primary `SESSION_OPEN(seq0)` explicitly opens the session, and later inner commands require a matching open session before sequence evaluation or dispatch
  - enveloped S-band primary `MODE_SET IDLE` binds authenticated `source_id`, `key_slot`, `session_id`, `sequence_number`, and inner payload, and changes `SYS_MODE` only after `parse -> auth -> authority -> lifecycle -> sequence -> dispatch`
  - same-source fresh `SESSION_OPEN(new session_id)` replaces the prior active session; stale old-session traffic fails closed
  - reboot or runtime restart preserves the highest accepted reopen floor per source epoch; replayed or lower/equal `SESSION_OPEN(seq0)` values fail closed and only a higher reopen epoch can resume comm-managed traffic
  - reboot or runtime restart still clears in-memory accepted per-command sequence state; after a fresh higher reopen succeeds, later sequence traffic resumes from the new in-memory session
  - enveloped UHF backup `SESSION_OPEN` is accepted, read/status traffic is observed and dispatched, and high-authority mode-change remains rejected by the existing authority policy before downstream dispatch
  - authority-allowed enveloped commands use active strict-monotonic sequence enforcement after the lifecycle match succeeds: first/increasing sequence values dispatch, duplicate/lower sequence values emit `COMMAND_SEQUENCE_REJECTED` and do not reach `CmdDispatcher`
  - malformed auth envelope, unknown key slot, source mismatch, and bad MAC all fail closed before authority/lifecycle/sequence mutation and are not reported as authenticated command acceptance
  - auth-pass but authority-denied traffic does not implicitly open a session, does not mutate active session metadata, and does not consume accepted sequence state
  - auth-pass but lifecycle-denied traffic does not consume accepted sequence state
  - authority-denied, malformed, unopened, same-session reopen, and mismatched-session lifecycle cases do not consume accepted session/sequence state
- Does **not** prove:
  - preferred current secure-auth baseline command-session truth
  - legacy command-envelope retirement
  - hosted proof for authority ingress port `1`
  - simultaneous S-band/UHF routed command ingress
  - hosted UHF serial node-6 command authority enforcement
  - physical link provenance, RF behavior, physical USB serial hardware, or physical RS485 electrical behavior
  - full link authority or full uplink authority
  - file packet uplink authority or unknown packet routing authority
  - signature-based auth, full replay protection, persistent secure session storage, persistent secure key storage, nonce-based replay defense, boot trust chain, or dynamic UHF primary failover
  - target hardware behavior, Raspberry Pi deployment, or reliable transfer behavior
- Governing evidence:
  - [docs/test-records/command-ingress-authority-v1/README.md](test-records/command-ingress-authority-v1/README.md)
  - [docs/test-records/command-ingress-source-index-v1/README.md](test-records/command-ingress-source-index-v1/README.md)
  - [docs/test-records/command-envelope-metadata-v1/README.md](test-records/command-envelope-metadata-v1/README.md)
  - [docs/test-records/command-session-sequence-v1/README.md](test-records/command-session-sequence-v1/README.md)
  - [docs/test-records/command-session-lifecycle-v1/README.md](test-records/command-session-lifecycle-v1/README.md)
  - [docs/test-records/command-auth-envelope-v1/README.md](test-records/command-auth-envelope-v1/README.md)
  - [docs/test-records/persistent-command-freshness-v1/README.md](test-records/persistent-command-freshness-v1/README.md)

### 46. Hosted EPS timeout FDIR path

- Path:
  - hosted default `OBC` uses the active `TopCcsds` baseline with ground link disabled
  - hosted `csp_zmqproxy` starts the governed internal CSP hub and hosted `eps_simulator` runs as EPS node `2`
  - `EpsBridge` performs scheduled EPS status polls over the existing internal libcsp EPS service path and publishes deterministic poll-health state
  - focused `EpsFdirController` runs in the next schedule slot, consumes `EpsBridge` poll-health state, and requests `SAFE` through `ModeManager` with the dedicated internal source `FdirSubsystemFault`
  - repository-owned hosted probe stops and restarts `eps_simulator` on isolated local ports/runtime roots to create real repeated timeout windows and observe recovery
- Proven scope:
  - repeated EPS poll timeout or equivalent repeated transport failure detection on the active hosted `TopCcsds` baseline
  - bounded retry with no mode escalation on the first two consecutive failures
  - third consecutive failure latches an EPS fault and triggers exactly one `SAFE` fallback request for `IDLE`, `PAYLOAD`, and `TTC`
  - `SAFE` and `HELL` record the fault without duplicate mode requests, covered by classic component tests on the same runtime owner boundary
  - first successful recovery poll clears the latched fault and emits explicit recovery evidence without auto-exiting `SAFE`
  - active runtime ownership stays split between `EpsBridge` transport/cache state, `EpsFdirController` timeout/escalation policy, `ModeManager` mode authority, and `ModeSafetyController` SoC policy
- Does **not** prove:
  - a broad all-subsystem FDIR framework
  - EPS reset, power-cycle, or load-shedding actions
  - hardware watchdog, process heartbeat supervision, or persistent event storage
  - command session/authentication, COMM QoS, TTC pass scheduling, payload control, RF, or target-hardware behavior
- Governing evidence:
  - [docs/test-records/fdir-subsystem-timeout-v1/README.md](test-records/fdir-subsystem-timeout-v1/README.md)

### 47. Hosted boot trust-chain path

- Path:
  - hosted default `OBC` uses the active `TopCcsds` baseline with ground link disabled for the focused probe
  - hosted `csp_zmqproxy` starts the governed internal CSP hub and hosted EPS/ADCS simulators keep the runtime stack representative enough for normal OBC execution
  - `BootManager` owns the boot/update lifecycle and resolves staging-root-relative image paths against the configured staging root
  - a staged image has an adjacent `<staged-image>.manifest-v1` manifest
  - manifest v1 includes image size, SHA-256 digest, target slot, image identity, software version, signer identity, key slot, signature algorithm, and signature
  - hosted runtime config supplies the boot trust anchor: `--boot-trust hmac-sha256`, trusted signer identity, key slot, and hex key
  - `BOOT_PREPARE_UPDATE`, `BOOT_VERIFY_STAGED_IMAGE`, `BOOT_ACTIVATE_STAGED_IMAGE`, `BOOT_CONFIRM`, status output, events, telemetry, and persisted metadata expose the trust decision and lifecycle state
- Proven scope:
  - staged image activation requires size/digest match plus accepted signed manifest
  - runtime-config-backed signer/key-slot trust anchor is enforced
  - `HMAC-SHA256` manifest signatures are verified using the repository crypto helper shared with authenticated command ingress
  - unknown/malformed/invalid/downgrade trust decisions fail closed in component coverage; hosted probe covers invalid signature and downgrade through runtime status
  - activation advances the monotonic `lastAcceptedVersion` floor and pending confirm/confirm/rollback keep trust metadata truthful
  - metadata reload preserves pending trust state and rejects ambiguous legacy pending/staged metadata in component coverage
  - hosted runtime status surfaces `trustStatus`, `trustRejectReason`, staged/active software versions, signer, key slot, and last accepted version
- Does **not** prove:
  - hardware-backed key storage, secure element behavior, or asymmetric production signing
  - Raspberry Pi bootloader behavior
  - partition handoff, SD-card power-loss recovery, or full flight secure boot chain
  - target hardware behavior for `boot-trust-chain-v1`; the RPi boot probe has been updated but fresh target evidence must be recorded separately
  - command replay protection, link authority, RF behavior, or reliable transfer
- Governing evidence:
  - [docs/test-records/boot-trust-chain-v1/README.md](test-records/boot-trust-chain-v1/README.md)

### 48. Hosted COMM session/link-role command-policy and shared-arbitration path

- Path:
  - hosted default `OBC` uses the active `TopCcsds` baseline
  - hosted `fprime-gds` keeps the default CCSDS S-band node-`5` command/event/channel path through `ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5)`
  - hosted `fprime-gds` also keeps a bounded UHF serial node-`6` path through `ground_ttc_gateway(serial southbound) -> uhf_comm_csp_node(node 6)`
  - `CommandIngressAuthority` owns authenticated envelope verification, lifecycle, and sequence; `CommController` drives the active ingress role/profile for both ingress ports
  - `DpCatalog.fileOut` now traverses a COMM-owned scheduler before stock `FileDownlink`
- Proven scope:
  - default startup role state is `S-band primary` for command / telemetry / file and UHF is a bounded backup ingress
  - accepted `SESSION_OPEN(seq0)` is the historical legacy session boundary
    for the bounded UHF ingress policy cited by this entry
  - authenticated S-band primary high-authority command admission works on the active runtime path
  - authenticated UHF backup read/status traffic is admitted while high-risk mode control is rejected by COMM-driven policy
  - `COMM_STOP_PASS` remains observe-only rather than acting as a command/file admission gate
  - active official data-product ownership and UHF low-risk continuity during S-band file activity are reviewable runtime behavior
- Does **not** prove:
  - preferred secure-auth session-boundary authority for later secure-baseline
    work; use the dedicated secure-auth proof family instead
  - loss of the S-band primary path revokes the old S-band command session, clears the active downlink owner, and moves the primary role set to UHF
- Does **not** prove:
  - session-aware beacon suppress/runtime; cite entries 41A and 60A for the dedicated hosted and target quiet-path proofs
  - simultaneous dual-link routed ingress or runtime arbitration
  - UHF-over-CCSDS
  - RF behavior
  - reliable transfer, ARQ/NACK, or retransmission
  - arbitrary onboard file downlink
  - Raspberry Pi / target hardware closure
  - persistent anti-replay state or hardware-backed key storage
- Governing evidence:
  - [docs/test-records/comm-session-and-downlink-qos-v1/README.md](test-records/comm-session-and-downlink-qos-v1/README.md)

### 49. Historical hosted bounded UHF-primary node-6 HK fallback file/downlink path

- Path:
  - historical note: this entry records the retired HK fallback path and is not part of the current official `.fdp` baseline
  - reuses the hosted UHF serial node-`6` gateway-backed path after COMM primary-link switch
  - hosted default `OBC` starts from the active `TopCcsds` baseline and switches the COMM primary command / telemetry / file roles to UHF
  - a fresh authenticated UHF-primary session opens on ingress port `1`
  - `HK_DOWNLINK_INDEX` is issued through the UHF node-`6` command path and `FileDownlink` returns the file through the same UHF gateway-backed path to GDS file storage
- Proven scope:
  - COMM-owned `UHF primary` policy grants bounded authenticated file/downlink authority after explicit primary switch
  - stock `FileDownlink` can start a bounded UHF-primary transfer on the active baseline after COMM has quiesced packet egress and transferred ownership
  - the received `hk-index.csv` byte-matches the runtime source file on the hosted UHF node-`6` path
- Does **not** prove:
  - generic UHF reliable transfer
  - arbitrary onboard file downlink
  - RF behavior
  - UHF-over-CCSDS
  - Raspberry Pi / target hardware closure
- Governing evidence:
  - [docs/test-records/comm-session-and-downlink-qos-v1/README.md](test-records/comm-session-and-downlink-qos-v1/README.md)

### 50. Hosted watchdog-v1 runtime liveness supervision path

- Path:
  - hosted default `OBC` uses the active `TopCcsds` baseline with ground link disabled
  - hosted `csp_zmqproxy` starts the governed internal CSP hub, hosted `eps_simulator` runs as EPS node `2`, and `radio_mock_server` keeps the COMM path representative enough for normal OBC execution
  - `WatchdogSupervisor` runs as the single active-baseline watchdog owner and consumes explicit heartbeat outputs from `EpsBridge`, `EpsFdirController`, `ModeSafetyController`, and `CommController`
  - repository-owned hosted probe uses isolated runtime roots and local ports plus bounded probe-only beat suppression to make one supervised source stale and then observe recovery
- Proven scope:
  - bounded software-watchdog supervision over the active hosted `TopCcsds` baseline
  - explicit per-source heartbeat freshness for `EpsBridge`, `EpsFdirController`, `ModeSafetyController`, and `CommController`
  - deterministic warning-only threshold crossing with feed still eligible
  - watchdog fault latch with truthful `LATCHED_FAULT` vs `SAFE_REQUESTED` reporting
  - at-most-one `IDLE/PAYLOAD/TTC -> SAFE` fallback through the normal `ModeManager` internal source path once the current fault epoch sees a requestable mode
  - continued stale progression to supervisor-side watchdog feed suppression with reviewable `feedEligible=no` state
  - first-beat recovery clear for the stale source and aggregate recovery only after no enabled supervised source remains faulted or suppressed
  - migrated CPU/RSS threshold monitoring under `WatchdogSupervisor`
- Does **not** prove:
  - Raspberry Pi hardware watchdog stroking or watchdog-caused reset
  - boot-safe-image recovery, reset-cause persistence, process restart executors, or subsystem reset executors
  - broad multi-subsystem FDIR, persistent fault/event storage, RF behavior, or target-hardware closure
- Governing evidence:
  - [docs/test-records/watchdog-v1/README.md](test-records/watchdog-v1/README.md)

### 51. Hosted shared recovery-executor path

- Path:
  - hosted default `OBC` uses the active `TopCcsds` baseline with ground link disabled for the focused probe
  - hosted `csp_zmqproxy` starts the governed internal CSP hub, hosted `eps_simulator` runs as EPS node `2`, hosted `adcs_simulator` keeps the internal subsystem stack representative, and `radio_mock_server` keeps the COMM/runtime wiring representative enough for normal OBC execution
  - `WatchdogSupervisor` and `EpsFdirController` remain detector-local owners but emit shared recovery requests and clears into `RecoveryExecutor`
  - `RecoveryExecutor` owns per-incident normalization, recovery-level progression, single-shot action gating, shared mode-fallback ownership, bounded EPS reset ownership, reboot intent persistence, and hosted reboot-equivalent exit
  - `BootManager` persists recovery-caused reset/boot metadata, and the repository-owned hosted probe relaunches the same runtime root after a reboot-equivalent exit to verify the resulting boot truth
- Proven scope:
  - watchdog stale and EPS timeout incidents use the same shared recovery-executor path on the active hosted `TopCcsds` baseline
  - one hosted watchdog subcase proves a suppression-triggered reboot-equivalent closure with truthful `RECOVERY_WATCHDOG` boot metadata on same-root relaunch
  - a separate repeated-watchdog subcase proves the bounded safe-fallback clamp plus later `R6` reboot-equivalent closure when detector-side suppression proves the incident persists
  - EPS timeout keeps failures `1-2` retry-only in the detector, enters the shared executor at failure `3`, executes bounded `R3` EPS interface reset as the first shared action, and escalates to `R6` reboot-equivalent closure under sustained outage
  - shared recovery status is reviewable through runtime status and bounded command/status surfaces for active incident, level, last action, pending reboot, and relatch count
  - boot metadata truth across same-root relaunch is reviewable for `reset_cause`, `boot_count`, `consecutive_reset_count`, `last_recovery_source`, `last_recovery_level`, and boot-safe-fallback clamp behavior
  - repeated failures escalate recovery level forward instead of repeating the same warning or one-shot `SAFE` action forever
- Does **not** prove:
  - Raspberry Pi hardware watchdog stroking or target-side reboot proof
  - broad all-subsystem FDIR, non-EPS subsystem reset policy, or a generic persistent fault manager
  - current hosted watchdog `R2` process-restart execution; cite entry 55 for that distinct proof
  - power-loss resilience, hardware secure boot, RF behavior, or target-hardware closure
- Governing evidence:
  - [docs/test-records/recovery-executors-v1/README.md](test-records/recovery-executors-v1/README.md)

### 52. Historical hosted bounded multi-subsystem shared recovery closure

- Path:
  - hosted default `OBC` uses the active `TopCcsds` baseline
  - hosted `csp_zmqproxy` starts the governed internal CSP hub, hosted `eps_simulator` runs as EPS node `2`, hosted `adcs_simulator` runs as ADCS node `3`, hosted `radio_mock_server` keeps the flight-side COMM wiring active, and hosted `uhf_comm_csp_node` node `6` is used for the bounded COMM failover case
  - EPS and ADCS hosted cases keep a healthy local direct-TCP S-band peer so the proof isolates the target subsystem line instead of allowing background COMM faults to reopen the shared executor
  - COMM hosted case uses a local S-band TCP peer plus a local UHF external TCP peer to drive bounded `SBAND -> UHF` failover, clear, and relatch behavior without claiming RF proof
  - `EpsFdirController`, `AdcsFdirController`, and `CommController` remain detector-local owners; `RecoveryExecutor` remains the single shared recovery-action owner
  - same-runtime-root relaunch is required to verify truthful `BootManager` recovery metadata after reboot-equivalent escalation
- Proven scope:
  - the archived `multi-subsystem-fdir-v1` hosted closure remains the last repo-owned three-subsystem `EPS + ADCS + COMM` shared-recovery proof on the hosted `TopCcsds` baseline
  - that archived closure proved `EPS_TIMEOUT`, `ADCS_POLL_TRANSPORT`, and `COMM_PRIMARY_UNAVAILABLE` entering the same shared `RecoveryExecutor` path, with bounded subsystem action ownership and same-root relaunch truth
  - the archived closure also proved the then-governed EPS `R3 + SAFE -> relatch R6`, ADCS first-fault clear/reopen progression, and COMM failover/clear/relatch behavior recorded in its governing evidence
- Does **not** prove:
  - that `bash scripts/run_multi_subsystem_fdir_v1_probe.sh` remains a maintained rerunnable current proof surface on the post-ADCS-R3 baseline
  - current hosted rerunnable Route 3 closure; use the scoped `run_recovery_executors_v1_probe.sh` wrappers and `scripts/chapter5_routes/hosted/route3_recovery_chain_pre_reboot.sh` for the maintained current hosted path
  - generic all-subsystem FDIR or future GPS/payload/TTC/storage recovery lines
  - target-side ADCS recovery closure beyond the hosted `R3` reset and hosted relatch reboot-equivalent proof
  - RF behavior, hardware reboot proof, or target-hardware reset proof
  - persistent recovery policy configuration or a generic fault/event store
- Governing evidence:
  - [docs/test-records/multi-subsystem-fdir-v1/README.md](test-records/multi-subsystem-fdir-v1/README.md)

### 53. Hosted TTC pass-window policy automation path

- Path:
  - hosted default `OBC` uses the active `TopCcsds` baseline
  - hosted `csp_zmqproxy`, `eps_simulator`, `adcs_simulator`, and `radio_mock_server` keep the normal subsystem and COMM wiring representative
  - hosted GPS replay input drives bounded cached UTC state through `GpsBridge`
  - `TtcPassManager` consumes cached GPS state, hosted COMM runtime availability, and the normal internal mode-control path to evaluate a single uploaded epoch pass window
  - hosted direct-TCP ground-link peer is used only as the bounded `sbandAvailable` source for the v1 COMM-loss proxy
- Proven scope:
  - bounded runtime `TT&C` config and single-window epoch pass contract are active on the hosted baseline
  - `TtcPassManager` is the single owner of hosted `IDLE -> TTC -> IDLE` pass-window policy automation
  - pass-window input alone does not enter `TTC` when `enabled=false`
  - hosted `IDLE -> TTC` auto-entry occurs only when `TT&C enabled`, the epoch window is active, and cached GPS time basis is valid/fresh
  - hosted `TTC -> IDLE` auto-exit is directly proven on window end
  - hosted direct-TCP COMM-loss on this baseline is preempted by the existing `COMM_PRIMARY_UNAVAILABLE` shared recovery path before TTC policy `COMM_LOSS_TIMEOUT` exit is observed
  - existing mode safety / shared recovery paths retain precedence over TTC policy when low-SoC safety or hosted direct-TCP COMM-loss triggers during `TTC`
  - manual TTC entry/exit remains on the normal `ModeManager` path while TTC policy remains the single automation owner
- Does **not** prove:
  - default hosted CCSDS S-band GDS uplink behavior
  - generic time-tagged scheduler behavior
  - payload automation
  - ADCS ground-target tracking
  - Raspberry Pi / target deployment behavior
  - RF behavior or hardware radio proof
- Governing evidence:
  - [docs/test-records/ttc-pass-window-mode-v1/README.md](test-records/ttc-pass-window-mode-v1/README.md)

### 54. Hosted persistent fault ring same-root relaunch path

- Path:
  - hosted default `OBC` uses the active `TopCcsds` baseline with a local
    direct-TCP hold server keeping the S-band path healthy enough to avoid
    unrelated `COMM_PRIMARY_UNAVAILABLE` reopening
  - hosted `csp_zmqproxy`, `eps_simulator`, `adcs_simulator`, and
    `radio_mock_server` keep the normal subsystem and COMM wiring representative
  - `BootManager` and `RecoveryExecutor` are the only v1 writers into
    `PersistentFaultManager`
  - `PersistentFaultStore` persists dual-copy snapshots under
    `persistent-data/recovery/fault-ring-{a,b}.bin`
  - the repository-owned hosted probe drives a COMM FDIR reboot-equivalent
    cycle, relaunches the same runtime root, corrupts the newer copy, and
    relaunches the same runtime root again before reading history through the
    hosted shell `fault history [count]`
- Proven scope:
  - persistent recovery breadcrumb storage is active on the hosted baseline
  - `PersistentFaultManager` is the single public readback owner and the hosted
    shell surface can review newest-first persistent history
  - `BootManager` boot-observed breadcrumbs and `RecoveryExecutor` recovery
    lifecycle breadcrumbs survive same-runtime-root relaunch
  - corrupting the newer copy still leaves the older valid copy readable, and
    the next relaunch can continue from that fallback state without losing the
    bounded history contract
  - the persistent fault ring remains distinct from `.fdp`, live beacon, and
    existing boot metadata truth
- Does **not** prove:
  - target power-loss resilience, filesystem durability on target hardware, or
    Raspberry Pi reboot persistence
  - `.fdp` summary export, `OnboardState` summary fields, or beacon payload
    changes
  - detector-local duplicate writers outside `BootManager` and
    `RecoveryExecutor`
  - persistent anti-replay state, boot trust hardening, RF behavior, or target
    deployment closure
- Governing evidence:
  - [docs/test-records/persistent-fault-ring-v1/README.md](test-records/persistent-fault-ring-v1/README.md)

### 55. Hosted watchdog R2 process-restart and R6 reboot-equivalent separation path

- Path:
  - hosted default `OBC` uses the active `TopCcsds` baseline with isolated
    runtime roots and representative hosted subsystem services
  - watchdog-source faults enter the shared `RecoveryExecutor` path as
    `R2_RESTART_SOFTWARE_COMPONENT`
  - `RecoveryExecutor` persists boot metadata through `BootManager`, sets
    process-restart pending/count state, and returns a typed
    `PROCESS_RESTART` runtime exit request
  - hosted runtime returns exit code `31` for `R2` process restart and keeps
    exit code `32` for `R6` reboot-equivalent closure
  - same-runtime-root relaunch verifies `reset_cause`, `last_recovery_source`,
    `last_recovery_level`, `boot_count`, and safe-fallback loop guard behavior
- Proven scope:
  - current watchdog `R2` sources are no longer intent-only on the active
    hosted baseline
  - `PROCESS_RESTART` is the active action vocabulary for R2 execution, while
    historical intent vocabulary remains compatibility-only
  - R2 process-restart pending/count state is distinct from R6 reboot
    pending/count state
  - R2 metadata is persisted before exit and reloads distinctly from R6
  - a consumed R2 exit request is not overwritten by later background COMM/R6
    scheduling before the runtime exits
- Does **not** prove:
  - Raspberry Pi hardware watchdog stroking or hardware-caused reset
  - RF behavior, reliable transfer, or generic all-subsystem FDIR
  - target service-managed restart; cite entry 56 for that path
- Governing evidence:
  - [docs/test-records/target-recovery-closure-v1/README.md](test-records/target-recovery-closure-v1/README.md)

### 56. Historical Raspberry Pi service-managed ADCS R2 process-restart path

- Path:
  - active OBC release is installed under `$OBC_HOME/obc-deploy/current`
    and run by `obc-comm-csp-stack.service` from the active `TopCcsds` `OBC`
    package
  - the governing target COMM path is the default node-`5` path:
    `fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> subsystem.local sband_comm_csp_node(node 5) -> shared SocketCAN -> obc.local OBC`
  - the archived `target-recovery-closure-v1` evidence used a repository-owned
    target probe that stopped `subsystem-adcs-csp.service` to induce the ADCS
    scheduled-poll transport fault when that line still mapped to `R2`
  - OBC exits with the R2 process-restart status, `run_obc_comm_csp_stack.sh`
    propagates the nonzero status, and systemd restarts
    `obc-comm-csp-stack.service` through `Restart=on-failure`
  - target boot metadata is read back from the active runtime root after the
    service-managed restart
- Proven scope:
  - historical ADCS-driven `R2` caused a real managed OBC process restart on
    the Raspberry Pi lab target path when ADCS scheduled-poll faults still
    entered at `R2_RESTART_SOFTWARE_COMPONENT`
  - OBC service PID changes and `NRestarts` increments under systemd
  - the relaunched OBC reports `RECOVERY_ADCS_FDIR`,
    `ADCS_POLL_TRANSPORT`, and `R2_RESTART_SOFTWARE_COMPONENT`
  - the target proof uses the active `TopCcsds` OBC package path, not legacy
    `OBC/Top` or `OBC_ComFprimeLegacy`
- Does **not** prove:
  - current watchdog-owned target `R2` restart proof
  - Raspberry Pi hardware watchdog stroking or hardware-caused reset
  - Linux reboot, bootloader or partition handoff, RF behavior, or final
    flight deployment behavior
  - current ADCS first-fault `R3` reset behavior, which requires refreshed
    target-side evidence separate from this archived historical path
- Governing evidence:
  - [docs/test-records/target-recovery-closure-v1/README.md](test-records/target-recovery-closure-v1/README.md)

### 57. Legacy Top retirement cleanup boundary

- Path:
  - active maintained OBC build, scripts, policy catalogs, and current docs use
    the `OBC` deployment and `TopCcsds` topology
  - old legacy Top / ComFprime commands and scripts remain visible only in
    archived evidence records when they describe past validation work
- Proven scope:
  - the maintained build no longer registers a legacy OBC deployment
  - current command authority policy is keyed only to active `OBCApp.*`
    dictionary commands
  - current verification inventory helper paths point at `TopCcsds` sources
  - current script index separates active hosted, active target/lab,
    historical evidence, and retired script names
- Does **not** prove:
  - any new RF, reliable-transfer, payload, target timing, or hardware watchdog
    behavior
  - that old evidence transcripts are rerunnable after the retirement
- Governing evidence:
  - [docs/test-records/legacy-top-retirement-docs-reorg-v1/README.md](test-records/legacy-top-retirement-docs-reorg-v1/README.md)

### 58. Raspberry Pi hardware watchdog board-reset path

- Path:
  - active OBC release is installed under `$OBC_HOME/obc-deploy/current`
    and run by `obc-comm-csp-stack.service` from the active `TopCcsds` `OBC`
    package
  - the governing target COMM path is the default node-`5` path:
    `fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> subsystem.local sband_comm_csp_node(node 5) -> shared SocketCAN -> obc.local OBC`
  - target runtime owns `/dev/watchdog0` through `LinuxWatchdogSink` and the
    governed watchdog udev/group/service contract
  - the repository-owned target probe applies a temporary
    `DIAGNOSTIC_QUIET_PACKET_EGRESS=1` systemd override, restarts the active
    service, and uses the current secure-auth command path on the same
    governed COMM lab node-`5` path
  - the bounded `SET_WATCHDOG_PROBE_SUPPRESSION` trigger lets a real
    watchdog-source incident first enter shared recovery at
    `R2_RESTART_SOFTWARE_COMPONENT` / `PROCESS_RESTART`, then stop stroking the
    hardware watchdog without adding a probe-only public command
- the board reboots through Raspberry Pi `bcm2835-wdt` timeout, the boot
  marker changes, and post-reboot secure-auth readback confirms the recorded
  recovery truth before the probe restores normal non-quiet service mode
  - fresh 2026-06-25 reruns additionally prove that A-layer baseline repair
    removes a stale external `HARDWARE_WATCHDOG=disabled` drop-in and returns
    the installed target baseline to `HARDWARE_WATCHDOG=linux-device`
- Proven scope:
  - active target OBC service owns `/dev/watchdog0` and can keepalive/close it
    compatibly with the systemd service model
  - watchdog-source stale suppression causes a real Raspberry Pi board reset on
    the active target path
  - post-reboot readback reports `RECOVERY_WATCHDOG`,
    `WATCHDOG_ADCS_FDIR`, and `R6_OBC_REBOOT`
  - the probe-owned quiet egress override restores the service to normal mode
    before exit
- Does **not** prove:
  - general non-quiet CCSDS serial stability under background TM
  - target `R2` service-managed restart; cite entry 56 for that path
  - generic Linux reboot, bootloader or partition handoff, power-loss
    recovery, RF behavior, or final flight deployment behavior
- Governing evidence:
  - [docs/test-records/target-hardware-watchdog-reset-proof-v1/README.md](test-records/target-hardware-watchdog-reset-proof-v1/README.md)

### 59. Target/lab default node-5 operational path

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and probe-owned `ground_ttc_gateway`
  - `ground_ttc_gateway(raw relay)` connects GDS TCP traffic to the S-band TCP southbound
  - `subsystem.local` runs `sband_comm_csp_node` as COMM node `5` on `can1` with listener `0.0.0.0:18520`
  - the shared SocketCAN carrier reaches target OBC node `1` on `obc.local:can0`
  - `obc-comm-csp-stack.service` runs `TARGET_COMM_PROFILE=sband`, `COMM_CSP_NODE=5`, and `COMMAND_AUTHORITY_PROFILE=sband-primary`
- Proven scope:
  - bounded secure-auth bootstrap and secure-command-v2 `GET_RESET_CAUSE`
    through the default target node-`5` path
  - current Route `3` target ADCS first-fault `R3_RESET_SUBSYSTEM_INTERFACE`
    proof remains governed by the same default node-`5` command path
  - current Route `3` target EPS first-fault `R3_RESET_SUBSYSTEM_INTERFACE`
    plus `R5 SAFE` fallback proof remains governed by the same default
    node-`5` command path
  - target hardware-watchdog board-reset proof also remains governed by the same default node-`5` command path, even though its proof trigger uses a temporary quiet egress override
- Does **not** prove:
  - UHF node-`6` ingress; cite entry 60 for that bounded quiet path
  - non-quiet serial stability under background TM
  - simultaneous dual-link runtime, beacon/session arbitration, or persistent ground-software residency
- Governing evidence:
  - [docs/test-records/target-comm-node56-migration-v1/README.md](test-records/target-comm-node56-migration-v1/README.md)
  - [docs/test-records/target-route3-pre-reboot-recovery-v1/README.md](test-records/target-route3-pre-reboot-recovery-v1/README.md)
  - [docs/test-records/target-hardware-watchdog-reset-proof-v1/README.md](test-records/target-hardware-watchdog-reset-proof-v1/README.md)
  - current Route 3 aggregate ledger:
    [docs/test-records/chapter5-integrated-route-closure-v1/README.md](test-records/chapter5-integrated-route-closure-v1/README.md)

### 60. Target/lab bounded quiet node-6 UHF compatibility path

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and probe-owned `ground_ttc_gateway`
  - `ground_ttc_gateway(link=uhf serial)` connects GDS TCP traffic to the physical lab serial southbound
  - `subsystem.local` runs `uhf_comm_csp_node` as COMM node `6` on `can1` and `/dev/serial0`
  - the shared SocketCAN carrier reaches target OBC node `1` on `obc.local:can0`
  - bounded proofs keep `DIAGNOSTIC_QUIET_PACKET_EGRESS=1`
- Proven scope:
  - `uhf-backup` bounded secure-auth bootstrap and command/readback through node `6`
  - `uhf-primary-after-failover` bounded re-auth plus secure-command
    continuity through node `6` after explicit node-`5` bootstrap and
    `COMM_SET_ACTIVE(UHF)` switch
  - target node-`6` ingress now uses the same governed UHF VCID shape as the hosted baseline
  - official `.fdp` file/downlink byte-match through quiet
    `uhf-primary-after-failover` node `6`
  - quiet `uhf-primary-after-failover` same-path official sequence upload,
    non-reject `SEQ_VALIDATE`, `SEQ_RUN(..., WAIT)`, and EPS/ADCS readback
    through node `6`
  - bounded `sband -> uhf-primary-after-failover` failover continuity with
    explicit node-`5` loss, node-`6` session reopen, and post-failover command
    completion
- Does **not** prove:
  - the maintained current autonomous failover path; cite entry `60C`
  - standalone target UHF bootstrap from a cold node-`6` session-open path
  - general non-quiet serial stability under background TM
  - session-aware beacon suppress/runtime; cite entry 60A for that dedicated quiet-path proof
  - simultaneous dual-link routed ingress or runtime arbitration
  - reboot-class target proofs; cite entries 56, 58, and 59 for node-`5`-governed reboot-class evidence
- Governing evidence:
  - [docs/test-records/target-comm-node56-migration-v1/README.md](test-records/target-comm-node56-migration-v1/README.md)
  - [docs/test-records/target-can-node6-matrix-closure-v1/README.md](test-records/target-can-node6-matrix-closure-v1/README.md)
  - [docs/test-records/challenge-handshake-secure-command-v1/README.md](test-records/challenge-handshake-secure-command-v1/README.md)
  - current Chapter 5 target closure ledger:
    [docs/test-records/chapter5-integrated-route-closure-v1/README.md](test-records/chapter5-integrated-route-closure-v1/README.md)
  - current maintained non-quiet UHF benchmark and detector-triggered failover follow-on:
    [docs/test-records/uhf-primary-nonquiet-runtime-v1/README.md](test-records/uhf-primary-nonquiet-runtime-v1/README.md)
    and
    [docs/test-records/target-autonomous-uhf-failover-v1/README.md](test-records/target-autonomous-uhf-failover-v1/README.md)

### 60C. Target/lab maintained non-quiet UHF primary autonomous-failover path

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and probe-owned
    `ground_ttc_gateway`
  - default bootstrap reuses entry `59`: node-`5` `S-band secure-auth` on the
    governed target/lab baseline
  - the governed helper creates a bounded
    `subsystem-sband-csp.service` unavailable window instead of using explicit
    manual `COMM_SET_ACTIVE(UHF)` switching
  - `subsystem.local` continues to host `uhf_comm_csp_node` as COMM node `6`
    on `can1` plus the physical UHF UART southbound
  - `RecoveryExecutor` owns failover actuation after
    `COMM_PRIMARY_UNAVAILABLE`; `UHF primary` remains non-quiet by default
- Proven scope:
  - detector-triggered `COMM_PRIMARY_UNAVAILABLE` latch on loss of the current
    node-`5` primary COMM stand-in
  - executor-owned promotion to `UHF primary`
  - stale primary-side auth invalidation and required `UHF secure-auth`
    re-bootstrap
  - auth-triggered `COMM_UHF_BEACON_SUPPRESS_STARTED`
  - bounded `GET_RESET_CAUSE` and `GET_PERSISTENT_FAULT_HISTORY` ground
    readback on the maintained non-quiet `UHF primary` path
  - fresh current benchmark closure for resend-ground-readback policy on the
    maintained `UHF primary` path
- Does **not** prove:
  - autonomous restore back to nominal `S-band primary`
  - quiet-path compatibility ancestry; cite entries `60`, `60A`, or `60B`
  - broad reliable-transfer or arbitrary file authority on `UHF primary`
  - generic simultaneous dual-link operator closure
  - Route `2` TTC/ADCS semantics or Route `3` watchdog semantics by itself;
    cite the Chapter 5 aggregate record for those route-level conclusions
- Governing evidence:
  - [docs/test-records/uhf-primary-nonquiet-runtime-v1/README.md](test-records/uhf-primary-nonquiet-runtime-v1/README.md)
  - [docs/test-records/target-autonomous-uhf-failover-v1/README.md](test-records/target-autonomous-uhf-failover-v1/README.md)
  - [docs/test-records/chapter5-integrated-route-closure-v1/README.md](test-records/chapter5-integrated-route-closure-v1/README.md)

### 60A. Target/lab quiet node-6 beacon suppress/runtime compatibility path

- Historical-only note:
  - this entry records archived target/lab proof that still depended on legacy
    command-envelope v1 / `SESSION_OPEN(seq0)` ingress
  - current branch-head runtime no longer accepts public `SESSION_OPEN` and no
    longer treats this path as a maintained closeout gate
  - the repository-owned wrapper now refuses by default unless explicitly
    invoked with `ALLOW_HISTORICAL_WRAPPER=1`

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and probe-owned `ground_ttc_gateway`
  - `ground_ttc_gateway(link=uhf serial)` relays CCSDS traffic to the physical lab serial southbound
  - `subsystem.local` runs `uhf_comm_csp_node` as COMM node `6` on `can1` and `/dev/serial0`
  - the shared SocketCAN carrier reaches target OBC node `1` on `obc.local:can0`
  - target `CommController` owns the qualifying session boundary and the bounded beacon suppress gate
  - probe-owned beacon capture on `subsystem.local` records BeaconV1 frames before suppress and after resume
  - bounded proofs keep `DIAGNOSTIC_QUIET_PACKET_EGRESS=1`
- Proven scope:
  - quiet-path baseline beacon visibility before suppress starts
  - accepted authenticated UHF `SESSION_OPEN(seq0)` starts suppress on the target/lab node-`6` retained legacy path
  - accepted authenticated same-session UHF read/status activity refreshes the bounded active window
  - resumed beacon capture appears only after fixed `60`-tick inactivity timeout clear
  - accepted S-band activity and rejected/non-qualifying UHF traffic do not trigger suppress
  - journal/probe markers keep suppress start and refresh reviewable on the governed quiet path
- Does **not** prove:
  - preferred secure-baseline suppress start at auth-success session synthesis
  - legacy command-envelope retirement
  - general non-quiet serial stability under background TM
  - simultaneous dual-link runtime arbitration
  - UHF reliable transfer, ARQ, NACK, or CFDP
  - RF behavior
  - broader UHF handshake state beyond the accepted `SESSION_OPEN(seq0)` compatibility boundary
  - cold-boot standalone node-`6` target bootstrap outside the current explicit switch/quiet-path proof family
- Governing evidence:
  - [docs/test-records/uhf-beacon-suppression-runtime-v1/README.md](test-records/uhf-beacon-suppression-runtime-v1/README.md)

### 60B. Target/lab quiet switched node-6 reliable-transfer path

- Historical-only note:
  - this entry records archived target/lab proof that still depended on legacy
    command-envelope v1 / `SESSION_OPEN(seq0)` ingress
  - current branch-head runtime no longer accepts public `SESSION_OPEN` and no
    longer treats this path as a maintained closeout gate
  - the repository-owned wrapper now refuses by default unless explicitly
    invoked with `ALLOW_HISTORICAL_WRAPPER=1`

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and probe-owned
    `ground_ttc_gateway`
  - target proof still bootstraps on default node-`5` `sband-primary`
  - the probe issues explicit `COMM_SET_ACTIVE(UHF)` before judging UHF
    reliable-transfer truth
  - `ground_ttc_gateway(link=uhf serial)` relays CCSDS traffic to the physical
    lab serial southbound
  - `subsystem.local` runs `uhf_comm_csp_node` as COMM node `6` on `can1` and
    `/dev/serial0`
  - the shared SocketCAN carrier reaches target OBC node `1` on `obc.local:can0`
  - bounded proofs keep `DIAGNOSTIC_QUIET_PACKET_EGRESS=1` only as
    probe-owned diagnostic control
  - the repo-owned node-`6` reliable-transfer receiver writes output under
    `/tmp/comm-reliable-transfer-node6`
- Proven scope:
  - happy-path official HK `.fdp` whole-file reliable transfer on quiet
    explicit-switched `uhf-primary-after-failover` node `6`
  - target journal first for node-`5` bootstrap session, explicit UHF switch,
    node-`6` session-open, and `COMM_RT_*` transfer truth
  - post-proof removal of the RT admission override, RT receiver override, and
    probe-owned quiet override, with services restored to the governed normal
    non-quiet baseline
- Does **not** prove:
  - hosted degraded resend/retry-exhausted cases on target
  - `uhf-backup` reliable transfer
  - automatic failover-to-UHF reliable transfer
  - nominal non-quiet UHF reliable-transfer closure
  - RF behavior, restart-persistent resume, broad CFDP, one-GDS aggregation,
    one-gateway multiplexing, or generic simultaneous closure
- Governing evidence:
  - [docs/test-records/uhf-reliable-transfer-v1/README.md](test-records/uhf-reliable-transfer-v1/README.md)

### 61. Hosted payload-ops contract path on default CCSDS S-band node 5

- Historical-only note:
  - this entry records archived hosted payload proof that still depended on
    legacy command-envelope v1 / `SESSION_OPEN(seq0)` ingress
  - current branch-head runtime no longer accepts public `SESSION_OPEN` and no
    longer treats this path as a maintained closeout gate
  - the repository-owned hosted wrapper now refuses by default unless
    explicitly invoked with `ALLOW_HISTORICAL_WRAPPER=1`

- Path:
  - hosted `OBC` runtime starts from the active `TopCcsds` deployment with the
    default CCSDS S-band command path
  - `fprime-gds(CCSDS)` and probe-owned `ground_ttc_gateway(raw relay)` feed
    `sband_comm_csp_node(node 5)`
  - authenticated `sband-primary` envelope/session ingress is opened through
    `SESSION_OPEN(seq0)` on routed ingress port `0`
  - the repository-owned hosted payload probe drives `MODE_SET(IDLE)`,
    `MODE_SET(PAYLOAD)`, official `SEQ_VALIDATE` / `SEQ_RUN`, and
    `PAYLOAD_GET_STATUS`
  - hosted payload execution uses the stub camera backend while preserving the
    public `PAYLOAD_*` contract
- Proven scope:
  - bounded `PAYLOAD_PREPARE -> PAYLOAD_CAPTURE_STILL -> PAYLOAD_SHUTDOWN`
    through the official sequence-admission surface on the default hosted CCSDS
    S-band node-`5` path
  - `PAYLOAD` mode entry remains side-effect free until explicit payload
    commands run
  - proxy EPS notification on reserved channel `3`
  - payload result/status readback and deterministic capture-path reporting
    under `<runtime-root>/persistent-data/payload/camera/`
  - hosted probe cleanup now preflight-removes stale probe state and tears down
    detached hosted-workspace `fprime_gds` child processes after the probe
- Does **not** prove:
  - real `libcamera` sensor interaction or Raspberry Pi camera hardware
  - a physically switched EPS camera rail
  - target/lab node-`5` or node-`6` ingress
- Governing evidence:
  - [docs/test-records/payload-ops-contract-v1/README.md](test-records/payload-ops-contract-v1/README.md)

### 62. Raspberry Pi helper-backed target payload path on the Pi-local GDS adapter

- Historical-only note:
  - this entry records archived target payload proof that still depended on
    legacy command-envelope v1 / `SESSION_OPEN(seq0)` ingress on the Pi-local
    adapter path
  - current branch-head runtime no longer accepts public `SESSION_OPEN` and no
    longer treats this path as a maintained closeout gate
  - the repository-owned target wrapper now refuses by default unless
    explicitly invoked with `ALLOW_HISTORICAL_WRAPPER=1`

- Path:
  - Raspberry Pi target `OBC` runtime starts from the active `TopCcsds`
    deployment with the governed direct target `OBC -> GDS` adapter path
  - repository-owned target payload probe runs local `fprime-gds` plus
    authenticated `sband-primary` envelope/session ingress on the Pi
  - the same public `PAYLOAD_*` contract drives one shared camera-ready
    prepare, `AUTO -> inspect actual metadata -> DETERMINISTIC` capture-policy
    checks, bounded `RAW_SENSOR` gating, and payload status readback
  - target payload execution on the currently recorded verification workspace
    proves the helper-backed payload contract and payload artifact path from
    the Pi-local GDS adapter path
  - `PayloadOpsController` still toggles EPS simulator PDU channel `3` as the
    lab proxy notification surface
- Proven scope:
  - helper-backed `libcamera` target execution on OV5647 through the Raspberry
    Pi CSI path
  - bounded `PAYLOAD_PREPARE -> PAYLOAD_CAPTURE_AUTO ->
    PAYLOAD_GET_LAST_CAPTURE_METADATA -> PAYLOAD_CAPTURE_DETERMINISTIC ->
    PAYLOAD_SHUTDOWN` behavior under the governed payload contract on the
    Pi-local direct target adapter path
  - deterministic JPEG storage and metadata sidecars under
    `<runtime-root>/persistent-data/payload/camera/`
  - target metadata readback records backend `libcamera`, camera model
    `OV5647`, requested/applied settings truth, actual `AUTO`
    exposure/gain/AWB truth, and deterministic exposure/gain truth for the
    direct target path
  - proxy EPS channel `3` notification alongside the target-local payload
    lifecycle
  - `RAW_SENSOR` mode remains governed and target-visible: the target probe can
    enter `RAW_SENSOR` session and receives an explicit unsupported rejection
    for sensor register access on the current real backend
  - cleanup-hardened repository-owned rerun can exit without leaving
    verification-workspace `fprime_gds` child processes on the Raspberry Pi
- Does **not** prove:
  - target/lab node-`5` COMM ingress; cite entry 59 separately for that path
  - a physically switched EPS camera rail
  - target official sequencing closure on the Pi-local direct path; cite hosted
    payload capture / payload-ops evidence separately for governed sequence
    proof on the active baseline
  - real OV5647 raw-register round-trip closure; the current target truth is an
    explicit bounded non-claim with governed unsupported rejection
  - payload downlink closure, scheduler behavior, timing/WCET closure, or a
    separately target-proven real-backend abort timing boundary
  - recovery from a lower-level hard hang inside a single blocking `libcamera`
    or kernel call that never returns during prepare
- Governing evidence:
  - [docs/test-records/payload-target-backend-hardening-v1/README.md](test-records/payload-target-backend-hardening-v1/README.md)

### 63. Hosted internal payload CSP shim path on OBC node 1

- Path:
  - hosted `csp_zmqproxy` starts the governed internal CSP hub
  - hosted `OBC` node `1` starts from the active `TopCcsds` deployment with
    ground-link disabled and the active internal CSP subsystem stack
  - `PayloadCspService` exposes a read-oriented payload shim on local node-`1`
    ports `34` (`STATUS`), `35` (`CAPABILITIES`), and `36`
    (`LAST_CAPTURE_METADATA`)
  - repository-owned `payload_csp_probe_main` runs as non-OBC client node `8`
    and performs request/reply reads against those node-`1` shim ports over the
    same governed hosted internal CSP runtime
- Proven scope:
  - node-`1` payload CSP shim service for status, capabilities, and last
    capture metadata is live and reviewable on the hosted baseline
  - the payload shim reuses `PayloadOpsController` runtime snapshots instead of
    creating a second ground operator or sequencing plane
  - hosted internal payload service proof remains distinct from the hosted
    CCSDS node-`5` operator path and from future node-`7` split deployment
  - the first implementation avoids libcsp client source-port collisions by
    using bindable local shim ports `34..36` while ports `40..49` remain
    reserved for future node-`7` service allocation
- Does **not** prove:
  - a live payload process on node `7`
  - any second ground operator path
  - target Pi payload-node closure
  - target real `libcamera` capture or raw-register closure
- Governing evidence:
  - [docs/test-records/payload-virtual-csp-node-v1/README.md](test-records/payload-virtual-csp-node-v1/README.md)

### 64. Hosted payload capture-modes v2 path on default CCSDS S-band node 5

- Historical-only note:
  - this entry records archived hosted payload proof that still depended on
    legacy command-envelope v1 / `SESSION_OPEN(seq0)` ingress
  - current branch-head runtime no longer accepts public `SESSION_OPEN` and no
    longer treats this path as a maintained closeout gate
  - the repository-owned hosted wrapper now refuses by default unless
    explicitly invoked with `ALLOW_HISTORICAL_WRAPPER=1`

- Path:
  - hosted `OBC` runtime starts from the active `TopCcsds` deployment with the
    default CCSDS S-band command path
  - `fprime-gds(CCSDS)` and probe-owned `ground_ttc_gateway(raw relay)` feed
    `sband_comm_csp_node(node 5)`
  - authenticated `sband-primary` envelope/session ingress is opened through
    `SESSION_OPEN(seq0)` on routed ingress port `0`
  - the repository-owned hosted payload v2 probe drives `MODE_SET(IDLE)`,
    `MODE_SET(PAYLOAD)`, `PAYLOAD_SET_AUTO_DEFAULTS`, one-shot
    `SEQ_VALIDATE(.sequence-staging/pcm2.bin)` bounded by no reject, wrapper
    `SEQ_RUN(.sequence-staging/pcm2.bin, WAIT)`, direct `AUTO` capture, last
    metadata readback, and final payload status/shutdown
  - hosted payload execution uses the stub camera backend while preserving the
    public `AUTO`/`DETERMINISTIC` payload v2 contract
- Proven scope:
  - bounded direct `AUTO` session behavior through the hosted CCSDS node-`5`
    path, including `.jpg + .json` capture artifacts, capabilities readback,
    metadata readback, and lab-proxy channel `3` notification
  - bounded deterministic wrapper sequencing through the official
    `SequenceAdmissionController` surface on the same hosted path
  - the current repo truth that `SEQ_VALIDATE` has no dedicated success event
    on the wrapper surface; hosted proof therefore treats validate as a
    bounded non-reject preflight and treats `SEQ_RUN` success as the formal
    end-to-end wrapper proof
- Does **not** prove:
  - target OV5647 real-image behavior
  - target `libcamera` closure
  - real raw-register round-trip behavior
  - a physically switched EPS camera rail
- Governing evidence:
  - [docs/test-records/payload-capture-modes-v2/README.md](test-records/payload-capture-modes-v2/README.md)

### 65. Hosted payload RAW_SENSOR register-controls path on default CCSDS S-band node 5

- Historical-only note:
  - this entry records archived hosted payload proof that still depended on
    legacy command-envelope v1 / `SESSION_OPEN(seq0)` ingress
  - current branch-head runtime no longer accepts public `SESSION_OPEN` and no
    longer treats this path as a maintained closeout gate
  - the repository-owned hosted wrapper now refuses by default unless
    explicitly invoked with `ALLOW_HISTORICAL_WRAPPER=1`

- Path:
  - hosted `OBC` runtime starts from the active `TopCcsds` deployment with the
    default CCSDS S-band command path
  - `fprime-gds(CCSDS)` and probe-owned `ground_ttc_gateway(raw relay)` feed
    `sband_comm_csp_node(node 5)`
  - authenticated `sband-primary` envelope/session ingress is opened through
    `SESSION_OPEN(seq0)` on routed ingress port `0`
  - the repository-owned hosted raw-register probe drives `MODE_SET(IDLE)`,
    `MODE_SET(PAYLOAD)`, one-shot `SEQ_VALIDATE(.sequence-staging/psrc1.bin)`
    bounded by no reject, wrapper `SEQ_RUN(.sequence-staging/psrc1.bin, WAIT)`,
    direct `RAW_SENSOR` register write/read, raw capture, metadata readback,
    and final payload status/shutdown
  - hosted payload execution uses the stub camera backend's deterministic fake
    OV5647-like register map
- Proven scope:
  - governed hosted `RAW_SENSOR` session contract on the CCSDS node-`5` path
  - official wrapper sequencing for `.sequence-staging/psrc1.bin` on that same
    hosted path
  - hosted fake register read/write observability, raw capture, metadata
    sidecars, and lab-proxy channel `3` notification
  - the same validate-success boundary as entry 64: `SEQ_VALIDATE` now
    requires envelope observation plus bounded no-reject preflight, and
    `SEQ_RUN` is the formal success proof
- Does **not** prove:
  - real OV5647 register round-trip closure
  - safe streaming writes for every exposure/gain/frame-timing register
  - target `libcamera` raw-register backend support
  - a physically switched EPS camera rail
- Governing evidence:
  - [docs/test-records/payload-sensor-register-controls-v1/README.md](test-records/payload-sensor-register-controls-v1/README.md)

### 66. Three-host target TCP development-carrier COMM matrix path

- Path:
  - `macOS` runs `csp_zmqproxy`, headless `fprime-gds`, `fprime-cli`,
    probe-owned `ground_ttc_gateway`, ground-side listeners, and file stores
  - `ground_ttc_gateway(raw relay)` connects GDS TCP traffic to node-`5`
    S-band TCP southbound on `subsystem.local`
  - a governed TCP-based UHF southbound stand-in feeds node `6` on
    `subsystem.local` without claiming physical UART provenance
  - `subsystem.local` runs `sband_comm_csp_node` as COMM node `5`,
    `uhf_comm_csp_node` as COMM node `6`, plus EPS node `2` and ADCS node `3`
  - `obc.local` runs only `OBC` node `1` and reaches subsystem traffic over
    the governed split-host internal CSP topology
- Proven scope:
  - governed target TCP reachability to nodes `2`, `3`, `5`, and `6`
  - authenticated node-`5` S-band command/readback, official `.fdp`
    file/downlink byte-match, and same-path official sequence validate/run
    with EPS/ADCS readback
  - authenticated node-`6` `uhf-primary-after-failover` command/readback and
    official `.fdp` file/downlink byte-match after explicit
    `COMM_SET_ACTIVE(UHF)` switch
  - bounded `sband -> uhf-primary-after-failover` failover continuity with
    node-`5` loss, node-`6` session reopen, and post-failover command
    completion
- Does **not** prove:
  - target direct `OBC -> GDS` adapter behavior
  - target physical UHF UART provenance; cite entry 60 for that physical path
  - node-`6` same-path sequence closure on the development carrier
  - real subsystem hardware wiring beyond the governed simulator-backed
    development-carrier topology
- Governing evidence:
  - [docs/test-records/target-tcp-southbound-parity-foundation-v1/README.md](test-records/target-tcp-southbound-parity-foundation-v1/README.md)
  - [docs/test-records/target-tcp-matrix-closure-v1/README.md](test-records/target-tcp-matrix-closure-v1/README.md)

### 67. Matrix-owned target direct `fprime-cli -> GDS -> OBC` command path

- Path:
  - macOS runs headless `fprime-gds` plus bounded `fprime-cli command-send`
    direct-control listeners under a probe-owned runtime root
  - Raspberry Pi target `OBC` runs from the active `TopCcsds` deployment on
    the governed direct `OBC -> GDS` adapter path without
    `ground_ttc_gateway`, COMM node `5`, or COMM node `6`
  - target TCP proof keeps EPS/ADCS online on the dev-carrier topology while
    the ground path itself remains direct
  - target CAN proof keeps EPS/ADCS online on shared SocketCAN while the
    ground path itself remains direct
- Proven scope:
  - dedicated target `fprime-cli -> GDS -> OBC` plain-command completion on
    the governed direct adapter path
  - bounded plain `MODE_GET` and `GET_RESET_CAUSE` completion with
    target-visible `OpCodeCompleted`
  - the active `TopCcsds` direct uplink framing truth for target command proof
    is `space-packet-space-data-link(scid=68, vcid=1, frame-size=1024)`, not
    the older connectivity-only `fprime` framing used by historical adapter
    evidence
  - the same dedicated matrix-owned direct wrapper closes both target TCP and
    target CAN direct-control cells
- Does **not** prove:
  - target TCP southbound parity, node-`5`, or node-`6` COMM ingress
  - target CAN S-band or UHF satcom closure by itself
  - target physical UHF UART provenance
  - direct `OBC -> GDS` connectivity without command completion as a distinct
    proof boundary; cite entry 2 for that historical connectivity claim
- Governing evidence:
  - [docs/test-records/target-direct-control-matrix-cases-v1/README.md](test-records/target-direct-control-matrix-cases-v1/README.md)

### 68. Service-managed target timing/WCET observation path on default node 5 (retired historical)

- Retired-now note:
  - this entry remains reviewable as archived timing evidence
  - current maintained secure-baseline closeout flows do not rerun the timing
    wrappers from this family as unrelated proof gates
  - the retained wrapper entrypoints now fail closed immediately so developers
    do not mistake them for current maintained probes

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and probe-owned
    `ground_ttc_gateway`
  - `ground_ttc_gateway(raw relay)` targets the S-band TCP southbound at
    `subsystem.local:18520`
  - `subsystem.local` runs `sband_comm_csp_node` as COMM node `5`
  - `obc.local` runs the installed `obc-comm-csp-stack.service` baseline with
    `TARGET_COMM_PROFILE=sband`, `COMM_CSP_NODE=5`, and
    `COMMAND_AUTHORITY_PROFILE=sband-primary`
  - the repository-owned timing probe is:
    `scripts/run_target_timing_empirical_ceiling_freeze_v1_probe.sh`
- Proven scope:
  - target-flightlike structural timing facts are frozen for the active
    service-managed baseline: base tick `1000 ms`, divisors `{1, 5, 1}`,
    nominal rates `1 Hz / 0.2 Hz / 1 Hz`, and F' slip-based missed-tick
    semantics
  - the current repository-owned probe path, workload windows, required
    telemetry, queue-depth diagnostics, and slip verdict criteria are
    checked-in evidence surfaces instead of broad undocumented timing `TBD`
    prose
  - fresh 2026-05-25 three-run node-`5` service-managed timing evidence froze
    empirical ceilings for the declared representative workload:
    - aggregated `RgMaxTime` ceilings:
      fast `837381 us`, slow `87658 us`, data `9451 us`
    - aggregated governed GDS inter-arrival bounds:
      fast `0.412 .. 2.823 s`, slow `4.998 .. 5.002 s`,
      data `0.995 .. 2.003 s`
  - the same three fresh restarted runs recorded:
    - `QueueOverflow = 0`
    - `RateGroupCycleSlip = 0`
    - `RgCycleSlips delta = 0`
    across both timing windows on the active node-`5` path
- Does **not** prove:
  - final flight-processor hard real-time closure
  - automatic transfer of the same ceilings to other target ground paths,
    workloads, or future flight processors
  - radio-metrics, reliable-transfer, or scheduler-redesign behavior
- Governing evidence:
  - [docs/test-records/target-timing-wcet-profile-proof-v1/README.md](test-records/target-timing-wcet-profile-proof-v1/README.md)
  - [docs/test-records/target-timing-empirical-ceiling-freeze-v1/README.md](test-records/target-timing-empirical-ceiling-freeze-v1/README.md)
  - [docs/test-records/target-node5-telemetry-backpressure-and-restart-stability-v1/README.md](test-records/target-node5-telemetry-backpressure-and-restart-stability-v1/README.md)
  - [docs/test-records/target-node5-rg3-csp-runtime-contention-fix-v1/README.md](test-records/target-node5-rg3-csp-runtime-contention-fix-v1/README.md)

### 69. Physical target-bearing dual-link proof on target CAN plus UHF UART

- Path:
  - `macOS` runs headless `fprime-gds`, `fprime-cli`, and probe-owned
    `ground_ttc_gateway`
  - `ground_ttc_gateway(raw relay)` targets `subsystem.local:18520` for
    S-band node `5` and the physical serial southbound for UHF node `6`
  - `subsystem.local` runs `sband_comm_csp_node(node 5)` and
    `uhf_comm_csp_node(node 6)` with the active EPS/ADCS services on the
    shared SocketCAN lab baseline
  - `obc.local` runs the installed `obc-comm-csp-stack.service` baseline on
    the shared target CAN path
  - the repository-owned proof wrapper is
    `scripts/run_target_dual_link_proof.sh`
- Proven scope:
  - the repository's first implementation-bearing target-bearing dual-link
    family now closes on the physical target CAN plus UHF UART topology
  - exact proven branch:
    - phase A default node-`5` `sband-primary` target truth
    - phase B non-quiet node-`6` `uhf-backup` adjunct with one minimal
      allowlisted `GET_RESET_CAUSE`
    - explicit `COMM_SET_ACTIVE(UHF)` switch closure on the historical
      pre-autonomous branch
    - phase C non-quiet node-`6` `uhf-primary-after-failover` target truth
  - the governing official branch landed as:
    - `target-claim=PASS`
    - `operator-observability=PASS`
    - `quiet-rescue=false`
  - target-truth acceptance is target-journal-first for phase A, phase B, and
    post-switch phase C
  - the official UHF ground-side artifacts stayed reviewable through gateway
    logs and byte captures, while phase B completion was corroborated on the
    still-primary S-band surface and phase C followed the then-current
    historical explicit-switch branch rather than the later maintained
    autonomous-failover branch
- Does **not** prove:
  - the current maintained autonomous failover path; cite entry `60C`
  - simultaneous full-authority commands on both links
  - a generic clean nominal non-quiet node-`6` operator surface beyond this
    exact official branch
  - quiet rescue closure; the official successful branch did not use it
  - one stock `fprime-gds` heterogeneous multi-upstream handling
  - one `ground_ttc_gateway` simultaneous S-band/UHF multiplexer behavior
  - RF closure
  - UHF reliable-transfer redesign
  - official file/downlink continuity as part of the main PASS boundary
- Adjacent citations only:
  - entry `59` for default node-`5` primary truth ancestry
  - entry `60` for quiet/switched node-`6` command/file/sequence ancestry
  - entry `60A` for quiet node-`6` beacon suppress/runtime ancestry
  - entry `66` for target TCP comparator ancestry only
  - entry `67` for direct target command-path adjacency only
- Governing evidence:
  - [docs/test-records/target-dual-link-proof-v1/README.md](test-records/target-dual-link-proof-v1/README.md)

### 70. Target secure-auth and bounded uplink-authority proof on node 5 plus physical node 6

- Path:
  - S-band reuses entry `59`: macOS headless `fprime-gds`,
    `ground_ttc_gateway(raw relay)`, `subsystem.local` S-band node `5`,
    SocketCAN, and `obc.local` installed `obc-comm-csp-stack.service`
  - UHF reuses entry `69`: physical node-`6` `uhf-backup` adjunct plus
    explicit switch to `uhf-primary-after-failover`
  - the repository-owned proof wrapper is
    `scripts/run_target_secure_auth_proof.sh`
  - the target proof loads the tracked keystore from the installed release at
    `$OBC_HOME/obc-deploy/current/config/security/command-auth.ini`
    instead of runtime `COMMAND_AUTH_*` or `--command-auth-*` injection
  - hosted secure-auth entries `43E` and `43F` are ancestry only; they are not
    target proof
- Proven scope:
  - installed-release provenance gate for `current` symlink, service
    `WorkingDirectory`, bundled keystore SHA, manifest SHA, expected target
    and subsystem service identities, and absence of forbidden
    `COMMAND_AUTH_*` service environment or `--command-auth*` CLI injection
  - target S-band APID `0x00FE` challenge auth and secure command v2 on the
    command APID
  - first accepted target S-band secure command may use non-`1` sequence `41`,
    later accepted commands require strict next sequence, and duplicate
    sequence `42` is rejected
  - malformed S-band handshake fails closed without auth/session mutation
  - target S-band `.sequence-staging/<leaf>` staged upload is admitted after
    secure auth
  - physical node-`6` UHF secure auth uses `ServiceID = 2`
  - `uhf-backup` read/status secure command is accepted, high-authority
    `MODE_SET` is denied, and the denied sequence is not consumed
  - `uhf-backup` staged upload remains denied even after secure auth
  - explicit switch to `uhf-primary-after-failover` invalidates old UHF
    auth/session state
  - `uhf-primary-after-failover` requires re-auth before secure-command
    acceptance
  - handshake and auth-status confirmation may advance on the governed wire
    capture or native packet-log surface, provided the proof records which
    maintained source observed the next fresh handshake step first and keeps
    the same target secure-auth path identity
  - gateway byte captures, target journal snapshots, checkpoint JSONL, summary
    JSON, provenance JSON, and cleanup status are preserved under the proof
    root
- Does **not** prove:
  - UHF primary staged-upload success
  - encryption
  - RF closure
  - boot-trust expansion
  - hardware-backed or persistent secure key storage
  - generic arbitrary file authority beyond `.sequence-staging/<leaf>`
  - one stock `fprime-gds` heterogeneous multi-upstream handling
  - one `ground_ttc_gateway` simultaneous S-band/UHF multiplexer behavior
  - legacy command envelope v1 retirement
- Adjacent citations only:
  - entry `43E` for hosted challenge-auth secure-command ancestry
  - entry `43F` for hosted keystore-backed staged-file and unknown-uplink
    authority ancestry
  - entry `59` for default target node-`5` S-band path reuse
  - entry `69` for the physical target-bearing non-quiet UHF node-`6` path
    family reused by this bounded secure-auth proof
- Governing evidence:
  - [docs/test-records/target-secure-auth-proof-v1/README.md](test-records/target-secure-auth-proof-v1/README.md)
  - [docs/test-records/node5-proof-oracle-hardening-v1/README.md](test-records/node5-proof-oracle-hardening-v1/README.md)
    records the source-aware handshake-oracle hardening follow-up on the same
    maintained target secure-auth path

### 70A. Service-managed target node-5 S-band observability tier-selection path

- Path:
  - S-band reuses entry `59`: macOS headless `fprime-gds`,
    `ground_ttc_gateway(raw relay)`, `subsystem.local` S-band node `5`,
    SocketCAN, and `obc.local` installed `obc-comm-csp-stack.service`
  - bounded UHF adjacency is present only to prove close on explicit
    role-switch and to restore S-band before exit; it does not widen the
    maintained node-`5` operator baseline
  - the repository-owned proof wrapper is
    `scripts/run_target_sband_observability_governance_probe.sh`
- Proven scope:
  - pre-auth service-managed node-`5` S-band packetized live `event/tlm`
    remains quiet
  - accepted S-band APID `0x00FE` secure auth opens target node-`5` curated
    live summary visibility on the maintained path
  - representative keep-live summary for scheduled `EPS`, `ADCS`, `RADIO`,
    and `STORAGE` remains reviewable on that authenticated node-`5` path
  - GPS keep-live summary channels remain part of the family contract when
    scheduled polls produce parseable samples, but the maintained live-UART
    target proof does not require an ambient sample at every proof moment
  - one representative bounded detailed readback,
    `EPS_GET_STATUS -> EPS_IBAT`, remains available on that same
    authenticated node-`5` path without reopening broad live chatter, and the
    detailed readback remains reviewable through a bounded command-specific
    packet-path artifact instead of relying only on passive observer silence
  - `WatchdogSupervisor` `SYS_*` remains the formal node-`5` resource
    keep-live truth
  - `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`, transport-error growth,
    `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
    `CSP_OWNER_TIMEOUT` / `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band
    `CommEgressMux` counters remain formal reviewable observability rather than
    pass-time keep-live summary
  - bounded cached readback such as `GET_RESET_CAUSE` also remains available on
    the same authenticated node-`5` path
  - explicit `COMM_SET_ACTIVE(UHF)` closes the old node-`5` live-observability
    gate, invalidates the superseded session, and requires UHF re-auth before
    later high-authority UHF traffic while confirming that the governed
    S-band passive observer stops receiving new operator-visible traffic after
    close; the packet capture remains a reviewable adjunct artifact rather
    than a raw-size quiet gate
  - proof cleanup restores S-band primary before exit so shared target
    baseline ownership remains with `A -> B`, not the probe
- Does **not** prove:
  - auth-free node-`5` summary readback or pre-auth live chatter
  - that every diagnostics-only residual channel has been removed from runtime
    output
  - any reinterpretation of `GROUND_LINK_TX_BYTES`, `GROUND_LINK_UP/DOWN`,
    `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
    `CSP_OWNER_TIMEOUT` / `CSP_OWNER_TOTAL_TIMEOUTS`, or S-band
    `CommEgressMux` counters as mere diagnostics; those remain formal
    reviewable observability on this branch
  - general non-quiet UHF operator closure
  - reliable transfer, RF behavior, one-GDS aggregation, or one-gateway
    simultaneous multiplexer behavior
  - replacement of the broader target secure-auth proof in entry `70`
- Governing evidence:
  - [docs/test-records/sband-live-observability-tier-selection-v1/README.md](test-records/sband-live-observability-tier-selection-v1/README.md)
    remains the last fully requalified target proof for the representative
    detailed `GET_*` packetized readback path
  - [docs/test-records/node5-observability-residual-cleanup-v1/README.md](test-records/node5-observability-residual-cleanup-v1/README.md)
    is the active same-change rebuild record for residual governance and
    detailed `GET_*` requalification on this branch until fresh hosted and
    target reruns close again
  - [docs/test-records/node5-proof-oracle-hardening-v1/README.md](test-records/node5-proof-oracle-hardening-v1/README.md)
    records the packet-path quiet requalification follow-up on the maintained
    target proof

### 71. Historical hosted payload dual-artifact `.fdp` family downlink closure on default CCSDS S-band node 5

- Path:
  - hosted `OBC` runtime starts from the active `TopCcsds` deployment with the
    default CCSDS S-band node-`5` command and file/downlink path
  - hosted payload capture uses `PayloadOpsController` with the stub payload
    backend, stores local `PIC%02X.bin` plus `PIC%02X.jpg`, auto-publishes one
    preview payload `.fdp` family, and promotes raw `.fdp` only on explicit
    follow-up command through `DpManager -> DpWriter`
  - the repository-owned proof wrapper is
    `scripts/run_payload_raw_preview_dual_artifact_v1_hosted_probe.sh`
  - the proof then uses the stock official operator flow
    `BUILD_CATALOG -> START_XMIT_CATALOG(NO_WAIT)` so `DpCatalog`,
    `CommController`, and `FileDownlink` deliver the selected payload `.fdp`
    into GDS file storage
  - repo-owned decode tooling
    `scripts/payload_fdp_extract.py` validates the received payload `.fdp`
- Historical proven scope:
  - accepted `PAYLOAD_CAPTURE_*` requests acknowledge dispatch immediately,
    while final preview success still requires canonical preview payload `.fdp`
    family publication on the hosted official node-`5` path
  - the canonical payload artifacts are distinct from the local diagnostic
    `PIC%02X.bin` and `PIC%02X.jpg` files under
    `persistent-data/payload/camera/`
  - stock `BUILD_CATALOG` plus `START_XMIT_CATALOG(NO_WAIT)` can downlink the
    selected payload `.fdp` without adding any payload-specific browse/list/
    select/download command family
  - the received GDS payload `.fdp` families byte-match the OBC source
    families for preview and bounded raw artifacts
  - repo-owned decode tooling can extract the payload header fields,
    validate preview JPEG bytes, and hash-match both preview and raw artifacts
    against the source local files
  - the hosted proof samples `PRESET_VGA_640X480`, `PRESET_HD_1280X720`, and
    `PRESET_FULL_3280X2464` capture requests on the same official path, with
    bounded raw official publication for `VGA` and `HD` plus deferred `FULL`
    raw rejection
- Current status:
  - archived hosted ancestry only; do not cite this entry as the current
    maintained payload-delivery authority
- Does **not** prove:
  - current maintained payload-delivery authority; cite entry `72` plus the
    archived payload dual-artifact record for the active branch truth
  - target `libcamera` real-image behavior or broad target camera size envelope
  - payload-family reliable-transfer widening beyond the current official HK
    `.fdp` scope
  - raw-register round-trip closure
  - a physically switched EPS camera rail
  - UHF nonquiet runtime stability or any target dual-link broadening
- Adjacent citations only:
  - entry `64` for earlier hosted payload `.jpg + .json` capture and wrapper
    sequencing ancestry
  - entry `3` for hosted `fprime-cli -> GDS` catalog-control ancestry
  - entry `59` only when target node-`5` path reuse is separately proven
- Governing evidence:
  - [docs/test-records/payload-raw-preview-dual-artifact-v1/README.md](test-records/payload-raw-preview-dual-artifact-v1/README.md)

### 72. Governed target node-5 payload dual-artifact `.fdp` family downlink closure on S-band

- Path:
  - the governed target node-`5` baseline starts from the active target COMM
    lab workflow and the stock target node-`5` S-band command plus
    file/downlink path
  - this path inherits the canonical `A -> B -> C` target governance default,
    so `scripts/ensure_target_comm_lab_baseline.sh` must require both
    `subsystem-sband-csp.service` and `subsystem-uhf-csp.service` unless an
    explicitly narrower exception is documented for a different path
  - target payload capture uses `PayloadOpsController` on the active target
    `libcamera` backend, stores local `PIC%02X.bin` plus `PIC%02X.jpg`,
    auto-publishes preview payload `.fdp`, and promotes raw `.fdp` only on
    explicit follow-up command through `DpManager -> DpWriter`
  - the repository-owned proof wrapper is
    `scripts/run_payload_raw_preview_dual_artifact_v1_target_probe.sh`
  - the same wrapper also supports archived-artifact reuse through
    `PAYLOAD_TARGET_REUSE_SUMMARY_JSON`, staging the archived payload `.fdp`
    family back into the governed target runtime so the stock official
    `BUILD_CATALOG -> START_XMIT_CATALOG(NO_WAIT)` path can be rerun without
    re-capturing a new photo
  - the proof uses the stock official operator flow
    `BUILD_CATALOG -> START_XMIT_CATALOG(NO_WAIT)` so `DpCatalog`,
    `CommController`, and `FileDownlink` deliver the bounded selected payload
    `.fdp` family into GDS file storage
  - repo-owned decode tooling
    `scripts/payload_fdp_extract.py` validates the received payload `.fdp`
    family
- Proven scope:
  - accepted `PAYLOAD_CAPTURE_*` requests acknowledge dispatch immediately,
    while final preview success is determined by payload state and metadata
    surfaces after target capture plus canonical publication complete
  - the governed target node-`5` baseline proves preview payload `.fdp`
    downlink plus bounded raw `.fdp` downlink for `PRESET_VGA_640X480`
  - the received GDS `vga` preview and raw payload `.fdp` families
    byte-match the target source families
  - repo-owned decode tooling can extract the payload header fields, validate
    the preview JPEG bytes, and hash-match both the received preview JPEG and
    raw `.bin` against the target source artifacts
  - current 2026-07-09 governed rerun on the `node5-comm-csp-downlink-v3`
    branch also passes in archived-artifact reuse mode under the scoped
    node-`5`/`6` COMM CAN FD condition set, so the official target downlink
    path can be requalified from existing route1 artifacts without new target
    capture
  - current later 2026-07-09 rerun on the same branch, after restoring
    `FileDownlink.Run` to the `1 Hz` data-group cadence, further reduces
    `START_XMIT_CATALOG -> CatalogXmitCompleted` from `270.567 s` to
    `176.091 s`; remaining cost is concentrated in
    `SendStarted -> FileSent`, not in `CatalogXmitCompleted -> final GDS`
  - final 2026-07-12 closeout rerun uses fresh live target capture/publish on
    installed release `v0.1.0-232-g4f6a6ad88`; A/B preflight and postflight
    return `READY/no-action-needed`, C verifies the A-owned scoped CAN FD
    profile, VGA preview plus ten-slice raw families byte-match and extract-hash
    `PASS`, and node-`5` reports accepted/flushed `1892352/1892352` with zero
    queued or duplicate frames; timing is `204.929 s` to catalog completion and
    `204.752 s` to final GDS arrival; evidence root:
    `/private/tmp/node5-v3-pr1-official-target-final`
  - `PRESET_HD_1280X720` is also proven as target local canonical raw
    publication on the same target path without being elevated to governed
    downlink proof
  - `PRESET_FULL_3280X2464` preview remains allowed on the official path, but
    `FULL` raw publication is an explicit deferred diagnostic on the current
    `2 MiB` payload-family ceiling and surfaces
    `PRESULT_STORAGE_FAILED detail 24`
- Does **not** prove:
  - governed target raw payload-family downlink closure beyond the bounded
    `vga` case
  - target source-image content validity by itself; cite entry `73` for the
    governed real-camera source-image sanity slice
  - broad target payload throughput adequacy
  - payload-family reliable-transfer widening beyond the current official HK
    `.fdp` scope
  - raw-register round-trip closure
  - a physically switched EPS camera rail
  - UHF nonquiet runtime stability or any target dual-link broadening
- Adjacent citations only:
  - entry `62` for Pi-local direct target payload capture ancestry
  - entry `59` for generic governed target node-`5` official path reuse
  - entry `71` for the hosted official payload `.fdp` family reference path
- Governing evidence:
  - [docs/test-records/payload-raw-preview-dual-artifact-v1/README.md](test-records/payload-raw-preview-dual-artifact-v1/README.md)
  - current branch-local governed rerun roots:
    `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-r3`
    `/private/tmp/payload-raw-preview-dual-artifact-v1-target.reuse-canfd-runfix`

### 73. Governed target real-camera payload source-image sanity on OV5647 `libcamera`

- Path:
  - the governed target `A -> B -> C` baseline starts from the active target
    COMM lab workflow and current secure-auth node-`5` command path
  - target payload capture uses `PayloadOpsController` on the active OV5647
    `libcamera` backend with bounded warm-up before final still persistence
  - the repository-owned proof wrapper is
    `scripts/run_payload_target_capture_sanity_v1_target_probe.sh`
  - the proof validates onboard source artifacts and their decoded source-side
    preview product; it does not rely on Route 1 ground/downlink success as
    the image-content oracle
- Proven scope:
  - target secure-auth plus `PAYLOAD` mode entry can drive one shared
    `PAYLOAD_PREPARE` camera-ready window, then execute real-camera `AUTO`
    and `DETERMINISTIC` VGA captures on the governed target path
  - target source artifacts exist for both cases as onboard `PIC%02X.bin` and
    `PIC%02X.jpg`, mirrored back by SSH for review
  - target capture metadata records backend `libcamera`, camera model
    `OV5647`, `PRESET_VGA_640X480`, actual `AUTO` exposure/gain/AWB result,
    and deterministic requested-versus-resulting exposure/gain alignment
  - source-image validity is closed by the repo-owned raw-frame luma oracle,
    not by artifact existence alone
  - target deterministic preview controls now have fresh bounded proof for:
    preview-JPEG `jpegQuality` selection and preview-path `hflip/vflip`
    effect corroboration
  - the earlier target black-image cold-start defect is closed in bounded form
    for current VGA `AUTO` and `DETERMINISTIC`
- Does **not** prove:
  - Route 1 official payload `.fdp` downlink closure; cite entry `72` for the
    governed transport/hash/downlink authority
  - broad target payload throughput or admitted size-envelope closure
  - full generic target camera-control parity for `meteringMode`, `evComp`,
    `brightness`, `contrast`, `saturation`, or `sharpness`
  - any claim that current target `hflip/vflip` closes a separate raw-artifact
    orientation contract; the bounded proof is on the preview/source-image path
  - raw-register round-trip closure
  - any UHF, failover, or dual-link payload behavior
- Adjacent citations only:
  - entry `74` for the maintained persistent-session lifecycle and
    `PAYLOAD_CAPTURE_STILL` retirement authority
  - entry `72` for governed target payload `.fdp` transport/downlink closure
  - entry `62` only for older Pi-local direct target capture ancestry
- Governing evidence:
  - [docs/test-records/payload-target-capture-sanity-v1/README.md](test-records/payload-target-capture-sanity-v1/README.md)

### 74. Payload persistent-session lifecycle and `PAYLOAD_CAPTURE_STILL` retirement

- Path:
  - current hosted semantic evidence uses
    `scripts/run_payload_persistent_session_hosted_probe.sh`
  - current target evidence reuses the governed target `A -> B -> C`
    secure-auth payload wrapper
    `scripts/run_payload_target_capture_sanity_v1_target_probe.sh`
  - both proofs exercise one normal non-RAW `PAYLOAD_PREPARE` window, then
    `AUTO -> PAYLOAD_GET_LAST_CAPTURE_METADATA -> DETERMINISTIC` without a
    second prepare
- Proven scope:
  - `PAYLOAD_SET_CAMERA_DEFAULTS` owns the shared non-RAW session signature
    (`resolutionPreset`, `jpegQuality`, `hflip`, `vflip`)
  - `PAYLOAD_PREPARE` establishes one shared persistent non-RAW camera-ready
    session for maintained still capture
  - `PAYLOAD_CAPTURE_AUTO` and `PAYLOAD_CAPTURE_DETERMINISTIC` are the only
    maintained normal still-capture policies on that shared session
  - a follow-up deterministic capture reuses the same live session instead of
    forcing a second full cold-start warm-up lifecycle
  - mutating `PAYLOAD_SET_CAMERA_DEFAULTS` while prepared is explicitly
    rejected instead of implicitly reconfiguring the live session
  - changing the shared session signature still requires operator-owned
    shutdown plus re-prepare
  - `PAYLOAD_CAPTURE_STILL` is retired from the current public payload surface
    and remains historical compatibility evidence only
- Does **not** prove:
  - Route 1 official payload `.fdp` downlink closure; cite entry `72`
  - raw-register closure
  - broad target camera-control parity for `brightness`, `contrast`,
    `saturation`, or `sharpness`
  - Route 1 official payload downlink closure after the public-surface rename;
    cite entry `72`
- Adjacent citations only:
  - entry `73` for target source-image validity, luma oracle results, and
    bounded preview-orientation corroboration
  - entry `61` and entry `62` for historical `PAYLOAD_CAPTURE_STILL`
    and Pi-local direct-path ancestry
- Governing evidence:
  - [docs/test-records/payload-persistent-session-still-retirement-v1/README.md](test-records/payload-persistent-session-still-retirement-v1/README.md)

### 75. Route 1 governed sequence-driven payload verification path

- Path:
  - hosted: active OBC with the required per-band node-5 and node-6 baseline,
    secure-auth S-band ingress, governed sequence staging, and stock GDS
    receipt of the deterministic preview .fdp
  - target: active target node-5 secure-auth path under A -> B -> C -> A -> B,
    where C is the Route 1 functional scenario and UHF/node-6 remains a
    required baseline dependency
- Proven scope:
  - checked-in Route 1 source compiles with the active dictionary and reaches
    only .sequence-staging/<leaf>
  - repo-owned SEQ_VALIDATE plus SEQ_RUN(..., WAIT) drive AUTO and
    DETERMINISTIC payload completion through the official sequence path
  - the proof records SoC/mode admission and rejects stale artifacts,
    command errors, and failed sequence terminal state as PASS
  - a target C result is accepted only when its required A/B postflight is
    recorded ready; Route 1 wrappers return failure rather than hiding a
    postflight baseline failure
  - every hosted attempt that actually executes, including an automatic retry
    or pending resume, first revalidates campaign identity and completes a
    fresh native OBC build under an attempt-specific log; `SKIP_DEPLOY` skips
    target synchronization, packaging, and installation only, while a retained
    authoritative hosted PASS remains skipped without being rebound
  - the manual-auth service profile needed by Route 1 is established through
    the A-layer baseline option before B; neither C-stage wrapper applies or
    restarts a shared service after A/B, and both C entrypoints pin their
    service-profile expectations to the governed node-`5` S-band values rather
    than inheriting caller profile controls; caller-provided manual timeout
    values and timeout-drop-in names are cleared before A, and externally
    managed C verifies all requested profile fields and fails on drift without
    applying a drop-in
  - after hosted completion, every target C attempt, including an automatic
    retry and a pending `SKIP_DEPLOY` or resumed attempt, revalidates campaign
    identity, requests A's governed forced OBC restart, and generates fresh
    target provenance, preventing a previously running release or preceding
    attempt provenance from surviving a runner-owned, external, or
    between-attempt `current` symlink swap; the ordinary fresh A -> B readiness
    preflight remains, and attempt-specific log labels preserve each gate
  - the formal-rerun wrapper blocks target C and authoritative PASS promotion
    unless its local branch/head, synchronized remote workspace marker, remote
    build metadata, installed release metadata/pointer, and installed OBC hash
    identify the same intended revision; `SKIP_DEPLOY` and resume do not
    bypass this gate, and untracked paths are accepted only when the same
    runner excludes their explicitly local-only roots from target sync
  - fresh deployment retains the rendered OBC systemd unit as a trusted
    install receipt; after A's restart, target provenance requires the active
    fragment to match that receipt, permits only the exact A-owned CAN-FD and
    Beacon drop-ins, rejects unknown drop-ins, verifies the effective launch
    path, and binds the service process tree to the installed release;
    `SKIP_DEPLOY=1` requires both package and rendered-unit receipts
  - the formal runner and both target C entrypoints reject any
    `OBC_COMM_CSP_SERVICE_NAME` other than
    `obc-comm-csp-stack.service`, keeping A, C, the installer, and provenance
    on one process identity
  - the no-`.git` workspace marker enumerates every serialized Git-indexed
    superproject/submodule path, binds a deterministic SHA-256 over path,
    Git-normalized mode, and content, and retains a package-path-keyed
    marker-time SHA-256 map for all eight remote-build inputs copied into the
    RPi bundle; before sync, the checker compares those serialized bytes and
    modes with committed superproject/submodule trees so index flags, staged
    index content, or `core.filemode=false` cannot hide uncommitted inputs;
    every target attempt recomputes the live workspace digest and all governed
    build-input hashes, rejects changed, missing, unsafe, duplicated, or
    omitted paths, and requires the installed-manifest package entries to
    match the marker even when a later build/package/install chain is
    internally consistent
  - formal target sync serializes only Git-indexed superproject and initialized
    submodule files into a staged replacement workspace, so ignored, untracked,
    or stale prior build inputs cannot enter the active target workspace;
    fresh deployment initializes and aligns recursive submodules before first
    campaign identity capture, while `SKIP_DEPLOY`, resume, and ordinary
    developer sync retain their existing behavior
  - a formal campaign records one branch/head/project-version identity before
    its first attempt; resume validates that identity against both the current
    checkout and the identity already recorded in the retained manifest before
    loading prior attempts, every invocation revalidates it after hosted
    execution, and every target attempt revalidates it before target
    restart/provenance, so hosted and target PASS records cannot span revisions
  - resume with a retained authoritative target PASS revalidates and preserves
    that attempt's original target revision provenance and requires its file
    SHA-256 to match the digest stored on the target attempt; it does not
    restart target services or overwrite the historical record with a
    same-head rebuild/current-install hash, and invalid retained authority
    fails closed
  - the frozen functional-observation checker requires and hash-verifies the
    DETERMINISTIC source FDP plus its claimed ground-received FDP; AUTO remains
    source-only and does not acquire a new downlink claim
  - directional gateway captures plus the distinct prepare-stage native-CLI
    and pipeline raw receive surfaces and the SoC-fallback CLI raw receive
    surface are independently hash-locked
  - the prepare-stage StandardPipeline channel and event observations are
    independently hash-locked alongside its raw receive surface
  - the StandardPipeline `pipeline-store` received FDP is independently
    hash-locked from the byte-identical GDS-runtime received FDP because each
    records a distinct ground consumer observation
  - the frozen observation's exact sequence source and compiled execution
    binary are independently hash-locked rather than inferred from the current
    working-tree example
  - every governing document is checked for explicit historical,
    non-authoritative classification of the 2026-07-12 proof and 2026-07-20
    functional observation, plus pending target requalification; retaining
    only the dates, links, or `not` inside an authority-promoting word is
    insufficient, and adding a contradictory positive authority statement
    fails even when the approved assertion remains; `while`/`although`
    contrast clauses are split independently, and a positive `remains`
    predicate fails validation; common numeric, English month-name, and Chinese
    spellings of the two governed dates receive the same bounded check
  - attempt labels are confined to one safe component below the selected
    route/surface evidence directory before the importer writes artifacts
  - hosted and target evidence identify their distinct provenance and
    observation boundaries
- Does **not** prove:
  - mission scheduler or persistent onboard schedule
  - generic payload throughput, RF closure, or OTA receipt closure
  - a new sequence-control plane, raw stock sequencer control, or generic
    payload/downlink closure beyond the bounded Route 1 assertions
- Adjacent citations only:
  - entry 72 for governed target payload dual-artifact family downlink closure
  - entry 73 for target real-camera source-image sanity
  - the earlier integrated Route 1 record for historical staged-route context
- Governing evidence:
  - [docs/test-records/route1-sequence-verification-v1/README.md](test-records/route1-sequence-verification-v1/README.md)
  - 2026-07-12 historical functional proof, not current target authority:
    it lacks retained remote workspace/build/install provenance artifacts
  - thesis-backed 2026-07-20 functional observation, not A/B/C authority:
    [externalized campaign artifacts](test-records/chapter5-integrated-route-closure-v1/ARTIFACTS.json)
  - decoded-JSON canonicalization and retained-artifact hashes:
    [artifact digest index](test-records/chapter5-integrated-route-closure-v1/ARTIFACTS.json)
  - the 7/20 observation preserves Chapter 5 values but does not satisfy the
    path's A/B/C ownership or complete revision-provenance requirements; the
    hosted/target path and bounded non-claims above are unchanged
  - Route 1 target requalification remains pending

### 76. Mission Console hosted and target beacon viewer operator surface

- Path:
  - manual dual-GDS surface exports per-band UHF beacon capability metadata
  - Mission Console consumes that capability and presents a bounded dashboard
    summary plus `/beacon` latest/history/detail view
- Proven scope:
  - hosted manual-surface beacon capability discovery
  - target manual-surface beacon capability discovery
  - dashboard Beacon summary with only latest beacon time and sequence
  - dedicated `/beacon` viewer with provenance, decode state, capture metadata,
    full decoded payload, and bounded history
  - explicit source distinction between hosted local PTY capture and target
    remote sidecar mirror
- Does **not** prove:
  - stock `fprime-gds` beacon display
  - explicit `GET_*` readback behavior
  - RF/OTA beacon receipt
  - UHF auth, command ingress, failover, or beacon suppress semantics
  - launcher detach hardening when manual surfaces are started under an
    external exec harness
- Governing evidence:
  - [docs/test-records/mission-console-beacon-viewer-v1/README.md](test-records/mission-console-beacon-viewer-v1/README.md)

## Path Selection Rules

- `OBC -> GDS` TCP connectivity and `fprime-cli -> GDS` command dispatch are related but distinct paths.
- A path is only reusable as baseline if its governing evidence is cited.
- When a later change validates one path while depending on another, the evidence record must say which path is newly proven and which path is only being reused.
- EPS and ADCS hosted business traffic SHALL remain on the registered libcsp internal paths; libcsp ZMQHUB is an allowed internal CSP substrate, not the retired project-local direct-ZMQ business path.
- `obc.local:/dev/serial0` current evidence is GPS evidence; older comm evidence that used the same device is historical and must be cited as historical.
- GPS hosted fake/replay and GPS live UART are separate paths; cite the matching entry for the behavior under review.
- Remote Pi-to-macOS CSP evidence proves a development-carrier topology over ZMQHUB/TCP/IP; it does not by itself prove any future physical bus or real subsystem wiring.
- The three-host split-host CSP entries are separate from the older two-host remote-macOS topology; cite the matching entry for the actual host layout under review.
- Physical lab serial COMM uplink ingress, bounded physical lab serial COMM TT&C, and physical lab serial COMM file/downlink are separate registry entries; cite the file/downlink entry only when received ground files and byte comparisons are part of the evidence.
- Hosted payload dual-artifact `.fdp` family closure, governed target node-`5`
  payload dual-artifact `.fdp` family closure, governed target real-camera
  source-image sanity, and Pi-local direct payload capture are separate paths.
  Cite entry `71` for hosted proof, entry `72` for governed target
  transport/downlink proof, entry `73` for governed target source-image
  validity, and entry `62` only for Pi-local direct payload capture.
- Physical lab serial COMM TT&C and physical COMM SocketCAN command/event/channel TT&C are separate registry entries; cite the SocketCAN entry only when COMM node `4` participates through `subsystem.local:can1` and the target OBC runs on `obc.local:can0`.
- Physical COMM SocketCAN command/event/channel TT&C and historical physical COMM SocketCAN HK fallback file/downlink are separate registry entries; cite the historical file/downlink entry only when GDS-received housekeeping archive files byte-match target OBC runtime source snapshots.
- The current target/lab COMM node-`5` path and maintained non-quiet autonomous-failover node-`6` path are separate from the older node-`4` SocketCAN TT&C and file/downlink entries. Cite the node-`4` entries only for archived compatibility evidence; cite entry `59` for the active target node-`5` baseline and entry `60C` for the active target node-`6` maintained baseline. Cite entry `60` only for the older quiet/manual-switch compatibility family.
- The three-host target TCP development-carrier COMM matrix path is separate
  from the physical target/lab node-`6` paths. Cite entry `66` only when the
  evidence uses the governed TCP-based southbound stand-ins on
  `subsystem.local`; cite entry `60C` for the maintained physical UART plus
  CAN autonomous-failover baseline, and cite entry `60` only for the older
  quiet/manual-switch compatibility slice.
- The matrix-owned target direct `fprime-cli -> GDS -> OBC` command path is
  separate from historical target direct `OBC -> GDS` connectivity. Cite
  entry 67 for plain-command completion claims and entry 2 only for
  connectivity-only direct adapter claims.
- The hosted payload-ops contract path and the Raspberry Pi direct payload-ops
  adapter path are separate entries. Cite entry 61 for hosted CCSDS node-`5`
  contract/sequencing proof, and cite entry 62 only when the claim includes
  real Raspberry Pi camera backend behavior on the direct target adapter path.
- Hosted `R2` process-restart closure and Raspberry Pi service-managed `R2`
  restart closure are separate paths. Cite entry 55 for hosted exit-code and
  metadata semantics, and entry 56 only when systemd service restart evidence
  on the target is part of the claim.
- The hosted payload capture-modes v2 path, the hosted payload RAW_SENSOR
  register-controls path, and the earlier hosted payload-ops v1 path are
  separate entries. Cite entry 61 only for the v1 payload contract surface,
  entry 64 for the hosted `AUTO`/`DETERMINISTIC` v2 surface, and entry 65 for
  the hosted `RAW_SENSOR` register-controls surface.
- Active OBC `TopCcsds` paths and historical legacy Top / ComFprime evidence
  are separate. Cite entry 57 when a change needs the cleanup boundary that
  removed the old maintained build/script surface.
- The hosted onboard data-products and live beacon path proves generated `.fdp` files, `DpCatalog` build behavior, hosted COMM-facing beacon capture/decode, and `FileDownlink` queueing only; cite the hosted official FDP parity path when hosted GDS-received `.fdp` byte-match and decode are part of the claim, and cite a physical COMM file/downlink path separately for physical link byte-match claims.
- The hosted primary mode model v2 path proves the primary mode contract, guarded hosted shell mode behavior, and hosted mode/HK/beacon integration only; cite separate registry entries for any COMM, CCSDS, storage, FDIR, RF, target-hardware, reliable-transfer, or pass-scheduler claim.
- The hosted SoC-driven mode safety policy path proves cached-EPS SoC fallback through hosted topology only; `payload-ttc-mode-entry-v1` keeps operator `HELL` entry rejected and covers guarded manual `HELL -> SAFE` through component guard tests. Cite separate registry entries for COMM, CCSDS, target hardware, RF, broader FDIR/watchdog, pass scheduling, or reliable-transfer claims.
- The hosted dual-link COMM simulator identity/coexistence path proves executable identity, node coexistence, OBC ping reachability, and existing COMM service behavior only; cite separate future evidence for any complete S-band GDS path, UHF UART backup path, CCSDS, RF, reliable transfer, file/downlink, target hardware, or Pi deployment claim.
- The hosted S-band TCP TT&C path and historical hosted S-band TCP HK fallback file/downlink path are separate registry entries. Cite the TT&C entry only for bounded command/event/channel proof through `sband_comm_csp_node` node `5`; cite the historical file/downlink entry only when GDS-received housekeeping archive files byte-match hosted OBC runtime source snapshots. Neither entry proves direct `GDS -> TCP -> OBC`, UHF backup, CCSDS, RF, reliable transfer, target hardware, Pi deployment, pass scheduling, or arbitrary onboard file downlink.
- The historical hosted UHF serial backup TT&C path, the active hosted UHF CCSDS adoption path, the hosted UHF node-6 BeaconV1 side-channel path, the hosted node-6 beacon suppress/runtime path, the hosted UHF primary packet-quiet path, and the hosted switched-UHF reliable-transfer path are separate registry entries. Cite the historical TT&C entry only for the old bounded `ComFprime` command/event/channel ingress claim, cite entry 43A when the claim is active UHF CCSDS command/file/downlink behavior on default hosted `OBC`, cite entry 43D when the claim is the explicit-switched hosted UHF reliable-transfer slice, cite entry 41 only for BeaconV1 capture/decode through the node-6 side channel, cite entry 41A when the claim includes suppress start, same-session refresh, timeout resume, or negative no-suppress proof, and cite entry 41B when the claim is the band-driven start of formal UHF live-packet suppression or file-downlink preservation during that packet-quiet window.
- The hosted challenge-auth secure-command path is separate from the older hosted UHF CCSDS adoption and legacy v1 command-session records. Cite entry 43E when the claim includes APID `0x00FE` handshake bootstrap, auth-success synthesis of repo-internal opened-session truth, default hosted pre-switch node-`6` `uhf-backup` secure-auth/read continuity, UHF re-auth after failover-primary role invalidation, or secure-command-v2 timeout clearing.
- The hosted staged-file and unknown-uplink authority closure path is separate from the narrower secure-command bootstrap entry. Cite entry 43F when the claim includes keystore-backed comm-managed auth defaults, malformed/unsupported APID `0x00FE` reject-without-mutation behavior, S-band staged upload success under secure auth, UHF backup staged-upload denial, or failover-primary staged-upload success after re-auth.
- Hosted node-`5` tier-selected observability is separate from both the
  broader hosted S-band adoption path and the hosted secure-auth authority
  path. Cite entry `43G` when the claim is specifically quiet startup,
  post-auth curated node-`5` live summary, fresh bounded detailed `GET_*`
  readback, cached onboard readback continuity, or live close after explicit
  primary switch that invalidates the superseded session.
- The target secure-auth proof is separate from hosted entries `43E` and `43F`
  and from the broader target dual-link proof in entry `69`. Cite entry `70`
  only when the claim includes installed-release keystore provenance, target
  S-band APID `0x00FE` secure auth, target secure command v2 sequence
  behavior, S-band staged upload after secure auth, bounded physical node-`6`
  UHF backup denial, role-switch invalidation, or
  `uhf-primary-after-failover` secure-command acceptance after re-auth. Do not
  cite entry `70` for UHF primary staged-upload success, encryption,
  hardware-backed key storage, one-GDS aggregation, or one-gateway
  simultaneous multiplexing.
- The service-managed target node-`5` tier-selected observability proof is
  narrower than entry `70`. Cite entry `70A` when the claim is specifically
  quiet startup on node `5`, post-auth curated live summary, fresh bounded
  detailed `GET_*` readback on node `5`, cached onboard readback continuity,
  or live close after explicit primary switch with restored S-band baseline on
  exit.
- The maintained hosted per-band stock ground/operator baseline is separate from both the default hosted CCSDS S-band adoption entry and the hosted UHF CCSDS adoption/suppress/packet-quiet entries. Cite entry 43B only when the claim is the maintained hosted operator start/stop/manifests boundary with two distinct stock GDS plus gateway surfaces on one shared runtime, including the bounded non-interference claim that launcher reruns do not reap unrelated active EPS/ADCS simulator runs on different CSP hub ports; cite the dedicated S-band or UHF entries separately when command, file/downlink, beacon suppress, packet-quiet, or secure-auth semantics matter.
- Mission Console beacon viewer is separate from the lower-level beacon runtime
  ancestry. Cite entry `76` only when the claim is the Mission Console
  operator-facing dashboard/`/beacon` surface. For target evidence, entry `76`
  requires A-owned remote-sidecar readiness plus C-owned local mirroring; C
  does not restart, override, or stop shared target services. Cite entry `41`,
  `41A`, or adjacent UHF records separately when the claim is lower-level
  beacon capture/runtime/suppress semantics.
- The hosted dual-link orchestration owner path is separate from `43B`. Cite entry 43C only when the claim is the hosted layer-2 lifecycle/failure/cleanup contract above the maintained per-band baseline and when the oracle comes from orchestration-owned manifest/status/cleanup artifacts; do not cite entry 43C for per-band TT&C semantics, COMM runtime policy, or target-bearing simultaneous claims.
- `target-dual-link-claim-oracle-clarification-v1` does not register a new
  simultaneous target path. For that clarification boundary, cite entry `59`
  for default target node-`5` primary truth, entry `60C` for maintained
  autonomous-failover node-`6` truth, entry `60` for quiet/manual-switch
  command/file compatibility, entry `60A` for quiet suppress or runtime only,
  entry `60B` for quiet switched node-`6` reliable transfer, entry `66` for
  target TCP comparator use only, and entry `67` only for direct target
  command-path adjacency. Cite
  `target-nonquiet-background-tm-stability-v1` only for oracle rationale or
  degraded ground-observability context; do not restate it as a reusable
  simultaneous target proof, and do not let quiet rescue wording stand in for
  non-quiet observability closure.
- The hosted CCSDS spike entry is historical proof for the retired `OBC_CcsdsGroundLinkSpike` executable. Cite the default hosted CCSDS S-band adoption entry only when the evidence uses `OBC`, `OBCApp.*`, S-band node `5`, CCSDS GDS framing, and raw gateway capture/decode; cite entry 44 for current official `.fdp` byte-match over the default hosted CCSDS path.
- Cite the default hosted CCSDS S-band adoption entry plus `payload-ttc-mode-entry-v1` evidence when the claim is bounded `OBCApp.modeManager.MODE_SET` command/rejection behavior over the default hosted CCSDS S-band path.
- The default hosted CCSDS S-band official FDP parity path is distinct from the older direct Native GDS official FDP parity path. Cite entry 44 when the claim includes `OBC`, `OBCApp.*`, S-band node `5`, CCSDS GDS framing, `DpCatalog`, GDS-received official `.fdp` byte match, and V6 decode on the current baseline.
- The hosted legacy command ingress authority family remains reviewable only as historical compatibility evidence. Cite entry 45 and the legacy records only when a change explicitly needs retained command-envelope v1 behavior, legacy `SESSION_OPEN(seq0)` lifecycle, or reboot-safe legacy reopen-floor semantics as a compatibility or later cleanup claim.
- The hosted challenge-auth secure-command path is separate from the legacy v1 command-session lifecycle family and is the preferred current secure baseline. Cite entry 43E when the claim specifically depends on challenge/response bootstrap, auth-success session synthesis without wire `SESSION_OPEN`, secure-command-v2 gating, or the default hosted pre-switch node-`6` `uhf-backup` secure-auth boundary.
- The hosted EPS timeout FDIR path extends the hosted EPS internal libcsp service path and reuses the active mode-control baseline, but it is a distinct path. Cite entry 45 only when the claim includes repeated EPS poll failure detection, bounded retry, one-shot `SAFE` fallback through `EpsFdirController`, and explicit recovery clear behavior on the active hosted `TopCcsds` schedule.
- The hosted shared recovery-executor path in entry 50 is the earlier watchdog plus EPS shared-ownership closure only. Cite entry 51 instead when the claim includes `ADCS` or `COMM` as shared-recovery consumers, `AdcsFdirController`, executor-owned `COMM_LINK_FAILOVER`, or three-subsystem boot-truth evidence.
- The hosted persistent fault ring same-root relaunch path is distinct from the
  earlier shared recovery-executor and multi-subsystem FDIR paths. Cite entry
  54 only when the claim includes `PersistentFaultManager`, dual-copy
  `fault-ring-{a,b}.bin` fallback, or hosted shell persistent-history readback;
  cite entries 51 or 52 separately for broader recovery-policy behavior.
- The hosted boot trust-chain path is distinct from command ingress auth and older digest-only boot/update evidence. Cite entry 46 only when the claim includes signed manifest verification, runtime-config-backed boot signer/key-slot trust, anti-downgrade floor behavior, and `BootManager` lifecycle/status/metadata evidence; cite target-specific evidence separately for Raspberry Pi bootloader, partition, power-loss, or full secure-boot claims.
- The hosted COMM session/link-role command-policy and shared-arbitration path proves active-baseline runtime policy closure across the default CCSDS S-band node `5` path and the bounded serial UHF node `6` path. Cite entry 47 when the claim includes COMM-owned primary-link state, dynamic ingress roles, UHF-backup low-risk continuity, deterministic HK/DP arbitration, observe-only pass semantics, or session/downlink convergence on S-band loss.
- The hosted bounded UHF-primary HK fallback file/downlink path is historical and separate from both the older UHF backup ingress path and the COMM command-policy/arbitration path. Cite entry 49 only for archived HK fallback claims such as explicit primary switch to `UHF primary` plus hosted node-`6` byte match for `hk-index.csv`; do not cite it for the current official `.fdp` baseline.
