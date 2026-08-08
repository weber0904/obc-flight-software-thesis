## ADDED Requirements

### Requirement: Payload Readback Includes Canonical Product Identity
The active payload contract SHALL expose canonical payload `.fdp` identity and publication status through existing payload readback surfaces.

#### Scenario: Last-capture metadata correlates local and canonical artifacts
- **WHEN** an operator requests payload status or last-capture metadata after a capture attempt
- **THEN** the runtime SHALL expose the local JPEG path, local metadata path, canonical payload `.fdp` relative path, and canonical publication result through existing payload readback surfaces
- **AND** the payload slice SHALL NOT require a new public list, select, or download command family to correlate those artifacts

### Requirement: Local Payload Files Remain Diagnostic After Canonical Promotion
The active payload contract SHALL keep local `.jpg + .json` artifacts reviewable after canonical payload `.fdp` publication without describing them as the formal delivered artifact.

#### Scenario: Local payload files remain reviewable but non-canonical
- **WHEN** a payload capture succeeds and canonical publication also succeeds
- **THEN** the runtime SHALL preserve the governed local `.jpg + .json` artifacts for bounded review and debugging
- **AND** active baseline wording SHALL describe the canonical payload `.fdp` as the formal stored/downlink artifact instead of the local files
