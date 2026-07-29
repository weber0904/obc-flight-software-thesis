## ADDED Requirements

### Requirement: Raspberry Pi CSP Plus Comm Baseline Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the Raspberry Pi CSP plus external comm baseline path once the target profile proves internal CSP reachability and `/dev/serial0` comm exchange in one governed run.

#### Scenario: Later target work can cite the combined baseline
- **WHEN** a later target-side change needs to rely on both Raspberry Pi CSP bring-up and comm UART availability
- **THEN** reviewers SHALL be able to cite one registry entry that names the combined target baseline and its still-out-of-scope neighboring paths
