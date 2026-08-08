## ADDED Requirements

### Requirement: Chapter 5 Integrated Routes Register Staged Closure Boundaries

The verification-path registry SHALL treat Chapter 5 Route 1, Route 2, and
Route 3 as distinct governed validation-path families, and each family MAY cite
multiple staged scripts or a repo-owned playbook as its governing closure
surface when the evidence record states that split explicitly.

#### Scenario: Registry distinguishes Route 1 from adjacent payload or file-only paths
- **WHEN** the repository later registers Chapter 5 Route 1 closure
- **THEN** the registry SHALL identify the combined payload-capture,
  low-SoC-fallback, and official payload `.fdp` downlink boundary
- **AND** it SHALL keep adjacent payload-capture-only, SoC-policy-only, or
  file/downlink-only paths distinct

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
