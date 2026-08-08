## ADDED Requirements

### Requirement: Target Payload Capture Sanity Evidence Distinguishes Onboard Source Artifacts From Downlink Closure

The verification evidence tree SHALL record a distinct target real-camera
payload sanity proof whenever the repository closes a target source-image
validity defect without rerunning full official downlink closure.

#### Scenario: Target capture sanity evidence stays source-side and reviewable

- **WHEN** `payload-target-black-image-fix-v1` records final evidence
- **THEN** reviewers SHALL be able to inspect the governed A/B/C target proof
  command path, the onboard source artifacts fetched from `obc.local`, the
  recorded payload metadata, and the bounded image-content sanity result
- **AND** the record SHALL state explicitly that those source artifacts were
  recovered from target storage rather than received over the ground/downlink
  path

#### Scenario: Route 1 historical closure remains transport-scoped

- **WHEN** adjacent payload or Chapter 5 records reference the older Route 1
  target formal rerun
- **THEN** they SHALL describe that rerun as transport/hash/downlink closure
  only
- **AND** they SHALL point current source-image validity claims to the
  dedicated target capture sanity record instead of implying that Route 1
  already proved non-black source imagery
