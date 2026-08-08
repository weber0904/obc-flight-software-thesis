## ADDED Requirements

### Requirement: Hosted FDP Parity Evidence
The verification evidence SHALL distinguish hosted official `.fdp` received-file parity from generated-file or queue-only evidence.

#### Scenario: Hosted FDP parity evidence is reviewable
- **WHEN** the FDP parity change completes
- **THEN** the evidence record SHALL include the hosted probe command, generated source `.fdp`, GDS-received `.fdp`, byte-match verdict, decode verdict, and final scope boundary
- **AND** the verification-path registry SHALL identify hosted official `.fdp` byte-match/decode as proven only for the direct hosted GDS path

#### Scenario: Parity exclusions remain explicit
- **WHEN** the evidence discusses official `.fdp` downlink parity
- **THEN** it SHALL explicitly exclude RF behavior, physical COMM `.fdp` parity, CFDP, ARQ/NACK, packet-loss recovery, arbitrary onboard file downlink, and deletion of `HousekeepingArchive`
