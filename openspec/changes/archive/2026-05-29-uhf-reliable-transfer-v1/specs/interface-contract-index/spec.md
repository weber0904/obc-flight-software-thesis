## MODIFIED Requirements

### Requirement: Interface index records the reliable-transfer owner split

The interface contract index SHALL describe the current bounded reliable slice
so reviewers can separate file-selection ownership, policy ownership, and
helper execution ownership across both current allowed paths.

#### Scenario: Reliable-transfer ownership stays explicit

- **WHEN** the interface index records the bounded reliable slice after
  `uhf-reliable-transfer-v1`
- **THEN** it SHALL state that `CommController` remains the policy owner
- **AND** it SHALL state that the bounded helper owns in-transfer send / ACK /
  retry execution
- **AND** it SHALL state that `DpCatalog` remains the file-selection owner

#### Scenario: Reliable-transfer segment size stays helper-path-local

- **WHEN** the interface index records the `160`-byte reliable-transfer
  segment ceiling
- **THEN** it SHALL identify that value as belonging only to the bounded
  reliable helper path on the exact allowed S-band node-`5` and explicit UHF
  node-`6` slices
- **AND** it SHALL distinguish that helper ceiling from the stock current UHF
  `243`-byte `Fw::FilePacket::DATA` ceiling
- **AND** it SHALL NOT present the helper segment size as a repo-wide
  transport MTU
