## ADDED Requirements

### Requirement: Recovery Boot Metadata Reuses The Shared File-Backed Store
The resource-storage baseline SHALL keep recovery-related boot metadata in the same governed persistent-data storage model as the rest of boot metadata instead of introducing a parallel recovery database or separate persistence root.

#### Scenario: Recovery boot fields stay under governed persistent-data root
- **WHEN** the runtime persists reset-cause, boot-count, consecutive-reset, or boot-safe-fallback truth
- **THEN** those fields SHALL live in the existing boot metadata file under the governed persistent-data runtime root
- **AND** they SHALL remain subject to the same hosted and Raspberry Pi root-override model already used for boot metadata
