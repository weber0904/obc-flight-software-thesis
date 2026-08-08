## ADDED Requirements

### Requirement: Target Real-Camera Payload Sanity Path Is Registered Separately From Route 1 Official Delivery

The verification-path registry SHALL keep target real-camera source-image
sanity distinct from the governed target node-`5` official payload `.fdp`
delivery path.

#### Scenario: Registry distinguishes target source-image sanity from official payload delivery

- **WHEN** reviewers inspect whether target payload source-image validity is
  formally proven
- **THEN** the registry SHALL identify a distinct target real-camera capture
  sanity path for onboard source artifacts
- **AND** it SHALL keep that entry separate from the Route 1 official
  payload `.fdp` downlink closure path
- **AND** it SHALL state explicitly that the source-artifact sanity proof makes
  no ground/downlink claim
