## ADDED Requirements

### Requirement: Hosted Payload FDP Path Is Registered Separately
The verification-path registry SHALL include a distinct hosted payload `.fdp` path once the governed hosted node-`5` proof demonstrates canonical payload publication, downlink byte-match, decode, and JPEG extraction parity.

#### Scenario: Hosted payload artifact proof stays distinct from HK FDP and local payload evidence
- **WHEN** reviewers inspect whether payload end-to-end closure is formally proven on the hosted path
- **THEN** the registry SHALL identify a dedicated hosted node-`5` payload `.fdp` entry
- **AND** it SHALL keep that entry distinct from existing hosted HK `.fdp` proof paths, hosted payload local-capture evidence, and direct adapter target payload evidence

### Requirement: Target Payload FDP Boundary Remains Explicit Until Proven
The verification-path registry SHALL keep target payload official `.fdp` closure explicit and separate until a governed target COMM proof demonstrates the same payload `.fdp` oracle on the target path.

#### Scenario: Target payload official closure does not collapse into direct adapter capture proof
- **WHEN** reviewers inspect whether target payload delivery closure is proven
- **THEN** the registry SHALL either identify a dedicated target node-`5` COMM-backed payload `.fdp` entry or state explicitly that the path is not yet registered
- **AND** it SHALL keep target official payload claims distinct from the older Pi-local direct `OBC -> GDS` payload capture evidence
- **AND** it SHALL state explicitly that raw-register closure, physical switched camera rail closure, and UHF nonquiet runtime stability remain outside the current hosted payload `.fdp` entry
