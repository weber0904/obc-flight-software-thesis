# Simulators Workspace

Hosted simulators, subsystem CSP protocol definitions, and host-side verification helpers live here.

## Current Contents

- `eps/`: EPS simulator model, libcsp service node, transport client, and integration test
- `adcs/`: ADCS simulator model, libcsp service node, transport client, and integration test
- `comm/`: shared byte-stream comm transport, radio protocol adapter layer, and TCP/serial-device integration test
- `scenario/`: repository-owned offline replay timeline and scenario bridge for hosted simulator validation

## Default EPS CSP Node

- `eps_simulator` runs as libcsp node `2` by default and accepts `--node-id <id>`
- hosted EPS uses the repository CSP ZMQHUB path managed by `csp_zmqproxy`
- EPS application services use ports `10..13`; libcsp ports `0..3` remain reserved for built-in management services
- EPS runtime DTOs live in `eps/EpsTypes.hpp`; active on-wire request and reply envelopes live in `eps/EpsCspProtocol.hpp`
- hosted EPS keeps the OBC-facing `pdu_status` bitmask unchanged, but the
  simulator now maps `0=OBC`, `1=ADCS`, `3=Payload`, `5=S-band`, and `6=UHF`
  to weighted demo loads while `2/4/7` remain zero-load spare channels
- hosted EPS status fields now evolve from monotonic time with fixed-seed
  pseudo-noise instead of returning fixed flat values
- the simulator-owned control socket keeps `set-soc` / `drop-status` and adds
  `set-load-mode normal|high-draw` for demo load switching

## Default ADCS CSP Node

- `adcs_simulator` runs as libcsp node `3` by default and accepts `--node-id <id>`
- hosted ADCS uses the repository CSP ZMQHUB path managed by `csp_zmqproxy`
- ADCS application services use ports `20..23`; libcsp ports `0..3` remain reserved for built-in management services
- ADCS runtime DTOs live in `adcs/AdcsTypes.hpp`; active on-wire request and reply envelopes live in `adcs/AdcsCspProtocol.hpp`
- EPS and ADCS do not retain a direct-ZMQ request/reply fallback in active source
- hosted ADCS `IDLE`, `DETUMBLE`, and `POINTING` are now time-continuous,
  seeded demo dynamics rather than fixed per-request snapshots
- hosted `POINTING` follows a repeating synthetic `60 s` pass profile; entering
  `POINTING` from another mode rewinds the pass, while the simulator-owned
  control socket can force a replay with `restart-pointing-pass`

## Comm Validation Paths

- `comm_transport_integration_test` validates the named default `mock-text` radio protocol adapter over loopback TCP mock and PTY-backed serial-device paths
- The comm transport layer does not declare a fixed always-on port; tests allocate loopback resources locally

## COMM CSP Simulator Identities

- `comm_csp_node` remains the generic compatibility COMM simulator executable:
  - default CSP node `4`
  - link identity `generic`
  - default CSP interface name `COMMCSP`
- `sband_comm_csp_node` is the hosted S-band COMM simulator foundation executable:
  - default CSP node `5`
  - link identity `sband`
  - default CSP interface name `SBANDCSP`
  - supports the shared serial-device endpoint and, for the governed hosted S-band TCP ground-link path, `--tcp-listen-host <host> --tcp-listen-port <port>`
- `uhf_comm_csp_node` is the hosted UHF COMM simulator foundation executable:
  - default CSP node `6`
  - link identity `uhf`
  - default CSP interface name `UHFCSP`
  - supports the shared serial-device endpoint
  - for the governed hosted UHF UART backup/beacon path, supports `--beacon-serial-device <path> --beacon-baudrate <rate>` as a bounded BeaconV1 side-channel output
- All three executable identities share the existing COMM CSP services:
  - `30 UPLINK_POLL`
  - `31 DOWNLINK_WRITE`
  - `32 LINK_STATUS`
- The hosted dual-link foundation proves explicit simulator identity and coexistence only. It does not prove a complete S-band GDS path, UHF UART backup path, CCSDS, RF behavior, reliable transfer, file/downlink behavior, or Pi hardware deployment.

## S-band TCP Ground-Link Simulator

- `ground_ttc_gateway` can connect its southbound side to the S-band simulator with `--rf-tcp-host <host> --rf-tcp-port <port> --link-identity sband`
- In this hosted path, `fprime-gds` remains the stock GDS-facing endpoint while the simulated RF segment is the separate TCP connection between `ground_ttc_gateway` and `sband_comm_csp_node`
- The first governed S-band TCP path is bounded to node `5`, command/event/channel TT&C, and housekeeping archive file/downlink byte matches through existing `HK_DOWNLINK_*` commands
- This does not prove UHF UART backup, CCSDS, RF behavior, reliable transfer, target hardware, Raspberry Pi deployment, pass scheduling, or arbitrary onboard file downlink

## UHF UART Backup / Beacon Simulator

- `ground_ttc_gateway` can connect its southbound side to the UHF simulator with `--serial-device <path> --baudrate <rate> --link-identity uhf`
- In the hosted v1 path, PTY serial pairs stand in for the macOS UART/USB/RS485 lab segment; this does not claim physical USB serial hardware or RS485 electrical behavior
- `uhf_comm_csp_node` node `6` keeps command ingress on existing services `30-32` and can emit BeaconV1 frames on a separate hosted beacon serial endpoint
- The governed UHF v1 path is bounded to command/event/channel ingress and BeaconV1 capture/decode; it does not prove full command authority, failover policy, file/downlink, CCSDS, RF behavior, reliable transfer, target hardware, Raspberry Pi deployment, or arbitrary onboard file downlink
- The current promoted-UHF runtime baseline is narrower than the historical hosted v1 ingress proof: active `uhf-primary-after-failover` command-session windows suppress live `event/tlm` packet chatter and beacon chatter on the formal UHF path, while official file/downlink routing remains a separate formal capability
- Near-term simultaneous S-band and UHF operator workflows are expected to use separate per-band stock GDS plus `ground_ttc_gateway` stacks rather than one stock GDS instance multiplexing both southbound links

## Scenario Replay Contract

- The first repository-owned offline replay contract lives at `scenario/examples/offline_replay_v1.csv`
- The first deployment-style detumbling autonomy replay example lives at `scenario/examples/deploy_detumble_replay_v1.csv`
- `ScenarioBridge` replays EPS sunlight and battery SoC over time, while ADCS angular rates are seeded only during replay initialization so later control-response behavior remains observable
- `ground_pass_open` and `link_available` are retained in bridge state now for future comm/autonomy work, but they do not yet drive another simulator in this first slice
