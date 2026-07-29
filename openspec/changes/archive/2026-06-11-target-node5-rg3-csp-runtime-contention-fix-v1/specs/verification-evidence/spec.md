## ADDED Requirements

### Requirement: Target Node-5 RG3 Salvage Evidence

The verification evidence tree SHALL record the salvaged node-`5` RG3
contention classification, bounded product fix, and clean-branch verification
record under
`docs/test-records/target-node5-rg3-csp-runtime-contention-fix-v1/`.

#### Scenario: Evidence identifies both diagnosis provenance and clean rerun

- **WHEN** the repository salvages the service-managed node-`5` RG3 contention
  fix onto a clean branch
- **THEN** the evidence SHALL identify the source-branch diagnosis provenance
  used to classify the blocker
- **AND** it SHALL identify the clean-branch verification command and artifact
  root used to confirm the bounded fix without the dirty vendored timing patch

#### Scenario: Evidence keeps the claim bounded to RG3 closure

- **WHEN** reviewers inspect that evidence record
- **THEN** it SHALL summarize the reproduced RG3 slip behavior before and after
  the fix
- **AND** it SHALL explicitly keep any remaining RG1 or broader CSP client
  contention as separate follow-up work
