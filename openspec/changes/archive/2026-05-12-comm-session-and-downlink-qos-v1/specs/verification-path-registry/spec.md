## ADDED Requirements

### Requirement: Registry Tracks COMM Session-And-Downlink Policy Paths

The verification-path registry SHALL add a hosted COMM session-and-downlink QoS path that identifies the command-policy and shared downlink behavior proven on the active baseline without collapsing distinct S-band and UHF boundaries into a single generic path.

#### Scenario: Command-policy path is registered

- **WHEN** hosted evidence proves COMM-driven authenticated policy across S-band node `5` and UHF node `6`
- **THEN** the registry SHALL describe the bounded command-policy path, the affected roles, and the explicit scope limits that remain outside the proof

#### Scenario: UHF file/downlink path is registered separately

- **WHEN** hosted evidence proves bounded UHF node-`6` file/downlink after primary switch
- **THEN** the registry SHALL add a distinct UHF file/downlink entry rather than extending the existing S-band file/downlink entry by implication
