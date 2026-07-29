## ADDED Requirements

### Requirement: GPS Hosted Replay Validation Path Is Registered
The repository verification-path registry SHALL include a dedicated entry for the first hosted GPS fake/replay validation path and SHALL describe that path as distinct from any still-unimplemented Raspberry Pi GPIO UART wiring or live-fix hardware path.

#### Scenario: GPS hosted path can be reused without implying hardware bring-up
- **WHEN** a later change wants to reuse the first GPS validation baseline
- **THEN** reviewers SHALL be able to cite one registry entry that names the hosted GPS fake/replay path, its governing evidence, and the still-out-of-scope Raspberry Pi UART hardware path
