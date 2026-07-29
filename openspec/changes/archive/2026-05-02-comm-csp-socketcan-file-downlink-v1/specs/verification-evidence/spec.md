## ADDED Requirements

### Requirement: COMM SocketCAN File Downlink Evidence
The repository SHALL maintain an evidence record for COMM SocketCAN file/downlink that captures TT&C prerequisites, archive slot identifiers, received file paths, source snapshots, byte-comparison results, CAN health, and excluded adjacent paths.

#### Scenario: Evidence records SocketCAN TT&C prerequisite
- **WHEN** the formal COMM SocketCAN file/downlink probe passes
- **THEN** the evidence SHALL first record the command/event/channel TT&C prerequisite result over the same SocketCAN-backed COMM path
- **AND** it SHALL include OBC command readback, fprime-cli command events, `GROUND_LINK_TX_BYTES`, and active CAN capture evidence

#### Scenario: Evidence records file comparisons
- **WHEN** SocketCAN-backed file/downlink is claimed
- **THEN** the evidence SHALL identify the downlinked `hk-index.csv` and at least two downlinked `hk-slot-*.bin` files
- **AND** it SHALL identify the target OBC source snapshot and GDS received path for each file
- **AND** it SHALL state that each received file was compared byte-for-byte against the source snapshot

#### Scenario: Evidence records constrained-link mitigations
- **WHEN** COMM SocketCAN file/downlink evidence is recorded
- **THEN** it SHALL record the bounded F' file packet size used for the formal run
- **AND** it SHALL record whether bounded file/downlink command retries were needed before the final byte-match
- **AND** it SHALL distinguish bounded whole-command retries from missing-packet retransmission and SHALL NOT claim packet-loss recovery unless that behavior is separately implemented and proven

#### Scenario: Evidence records active transport health
- **WHEN** COMM SocketCAN file/downlink evidence is recorded
- **THEN** it SHALL record host serial device, subsystem serial device, baudrate, preamble settings, GDS ports, GDS file-storage directory, `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1`
- **AND** it SHALL include CAN health showing `ERROR-ACTIVE` and no `bus-off` on active CAN interfaces

#### Scenario: Evidence excludes adjacent paths
- **WHEN** COMM SocketCAN file/downlink evidence is recorded
- **THEN** it SHALL cite the prior physical COMM SocketCAN command/event/channel TT&C evidence as a reused prerequisite
- **AND** it SHALL keep arbitrary file downlink, RF, no-preamble serial behavior, archive wraparound, ScenarioBridge/pass automation, dual-bus redundancy, independent COMM hardware, and packet-loss-tolerant reliable file transfer outside the proven verdict
