## ADDED Requirements

### Requirement: Internal CSP Carrier Selection Is Configuration-Driven
The platform baseline SHALL keep `libcsp` as the shared internal subsystem network contract while making the carrier or interface binding layer selectable through governed runtime configuration instead of embedding one hosted carrier directly into the runtime core.

#### Scenario: ZMQHUB remains the default development carrier
- **WHEN** the repository launches the existing hosted-local or Raspberry Pi local baselines without overriding the carrier selection
- **THEN** the internal CSP runtime SHALL continue to bind through the governed `zmqhub` carrier and SHALL preserve compatibility with `CSP_HUB_HOST`, `CSP_HUB_SUB_PORT`, and `CSP_HUB_PUB_PORT`

#### Scenario: Carrier abstraction does not change business-layer contracts
- **WHEN** the runtime carrier selection is refactored or extended
- **THEN** EPS and ADCS business traffic SHALL continue to use their subsystem-owned CSP contracts without requiring changes to the F' public command, telemetry, or event surface
