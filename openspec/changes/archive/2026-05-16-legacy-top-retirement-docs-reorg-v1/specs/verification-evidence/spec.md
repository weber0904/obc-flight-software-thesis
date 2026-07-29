## ADDED Requirements

### Requirement: Legacy Top Retirement Evidence Is Reviewable
The verification evidence tree SHALL record reviewable local evidence for
`legacy-top-retirement-docs-reorg-v1`, including the source/build cleanup,
script and policy cleanup, documentation reorganization, thesis refresh,
OpenSpec validation, and explicit historical-evidence boundary.

#### Scenario: Cleanup evidence records local gates
- **WHEN** `legacy-top-retirement-docs-reorg-v1` records final evidence
- **THEN** the evidence SHALL identify commands that checked for forbidden
  active references, generated and built the active deployment, generated and
  built unit tests, ran the full verification CI gate, and validated the
  OpenSpec change and current specs.

#### Scenario: Historical evidence remains reviewable
- **WHEN** reviewers inspect old evidence records that mention
  `OBC_ComFprimeLegacy`, `OBCAppComFprimeLegacy`, or `OBC/Top`
- **THEN** current evidence and indexes SHALL make clear that those references
  are historical records rather than current build, script, or runtime
  requirements.

## MODIFIED Requirements

### Requirement: Authority Catalog Coverage Is Verified
Verification evidence SHALL show that maintained active OBC topology dictionary
commands are covered by the command authority policy.

#### Scenario: Dictionary coverage is enforced
- **WHEN** focused authority catalog tests run
- **THEN** they SHALL verify that every command in the maintained active CCSDS
  topology dictionary has a classification
- **AND** they SHALL verify that generated runtime opcode catalog output matches
  the checked-in policy source and current dictionary command names
- **AND** they SHALL NOT require retired `OBCAppComFprimeLegacy.*` policy
  entries.

### Requirement: Active OBC Raspberry Pi Alignment Evidence
The verification evidence tree SHALL record reviewable Raspberry Pi package,
install, and autostart evidence that the governed target-facing release path
ships the active `OBC` / `TopCcsds` payload and no longer preserves a maintained
legacy deployment in the active build or script surface.

#### Scenario: Package evidence proves active payload truth
- **WHEN** `active-topccsds-target-alignment-v1` records final package evidence
- **THEN** reviewers SHALL be able to inspect the package command, manifest, and
  bundle contents showing that `bin/OBC` and
  `dict/AppTopologyDictionary.json` were sourced from the active `OBC`
  deployment
- **AND** any legacy helper or evidence reference SHALL be labeled as historical
  rather than part of active package truth.

#### Scenario: Installed and rebooted target evidence stays on the active path
- **WHEN** `active-topccsds-target-alignment-v1` records installed-release and
  autostart evidence
- **THEN** reviewers SHALL be able to inspect the installed current-release
  launch result and the post-reboot service result on the corrected active
  package path
- **AND** the evidence SHALL describe that proof as target package/startup
  alignment rather than as full COMM lab-operational closure.

### Requirement: Command Ingress Source Index Evidence Is Reviewable
The verification evidence SHALL record the configured ingress source-index
behavior introduced by `command-ingress-source-index-v1`.

#### Scenario: Component tests prove multi-port behavior
- **WHEN** the change is closed out
- **THEN** evidence SHALL list component tests covering configured port `0`,
  configured port `1`, unconfigured port fail-closed behavior, legacy
  `configure(config)` clearing semantics, and context preservation.

#### Scenario: Hosted probe proof is scoped to port zero
- **WHEN** hosted probe evidence is recorded
- **THEN** it SHALL state that current hosted default CCSDS topology wires only
  authority ingress index `0`
- **AND** it SHALL avoid claiming hosted proof for ingress port `1` or
  simultaneous S-band/UHF routed command ingress.
