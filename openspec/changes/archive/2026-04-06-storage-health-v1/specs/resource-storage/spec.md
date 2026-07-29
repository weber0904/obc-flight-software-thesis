## ADDED Requirements

### Requirement: Governed Runtime-Root Storage Monitoring
The resource-storage baseline SHALL provide a first-version observability path for the governed `hk`, `persistent-data`, `staging`, and `logs` runtime roots, and that path SHALL surface root-specific warning or degraded states instead of requiring direct filesystem inspection as the only review method.

#### Scenario: Runtime-root storage state becomes reviewable
- **WHEN** the first storage-health slice completes a scan on the hosted runtime
- **THEN** the project SHALL be able to review bounded root statistics and warning status for the governed runtime roots through repository-owned runtime contracts and evidence
