## MODIFIED Requirements

### Requirement: Target Payload FDP Path Is Registered Separately

The verification-path registry SHALL include a distinct governed target
payload `.fdp` path once the governed target node-`5` proof demonstrates the
same payload `.fdp` oracle on the target path.

#### Scenario: Target payload official closure becomes a registered node-5 path

- **WHEN** reviewers inspect whether target payload delivery closure is proven
- **THEN** the registry SHALL identify a dedicated target node-`5`
  COMM-backed payload `.fdp` entry once the repo-owned target wrapper passes
- **AND** it SHALL keep that entry distinct from the older Pi-local direct
  `OBC -> GDS` payload capture evidence
