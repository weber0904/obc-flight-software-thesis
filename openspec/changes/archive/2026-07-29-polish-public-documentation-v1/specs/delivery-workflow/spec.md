## MODIFIED Requirements

### Requirement: Baseline Reconciliation Matrix

The repository SHALL keep
`openspec/reconciliation/baseline-reconciliation-matrix.json` as the manually
maintained reconciliation source and
`openspec/reconciliation/baseline-reconciliation-matrix.md` as its generated
review surface.

#### Scenario: Generated review surface stays aligned with the JSON source
- **WHEN** the reconciliation JSON is edited
- **THEN** the Markdown review surface SHALL be regenerated before review
- **AND** repository consistency checks SHALL validate both files

### Requirement: Mainline Narrative Documentation Reconciliation

The delivery workflow SHALL update branch-verifiable documentation on the
originating branch before review, including formal specs, product behavior,
architecture, verification, evidence, and operator procedures.

#### Scenario: Branch changes a documented contract
- **WHEN** a change alters formal workflow, product behavior, architecture,
  verification, evidence, or operator procedures
- **THEN** the relevant documentation SHALL be updated on the originating
  branch
- **AND** the update SHALL use the normal review workflow
