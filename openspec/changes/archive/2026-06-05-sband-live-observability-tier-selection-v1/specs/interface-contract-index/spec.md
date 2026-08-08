## ADDED Requirements

### Requirement: Interface Index Records Current Content Tiers In Addition To Access Gating

`docs/interfaces.md` SHALL describe the current node-`5` observability tiers
so reviewers can distinguish curated live summary, fresh GET-driven bounded
readback, onboard cached truth, and remaining `non-baseline live` surfaces.

#### Scenario: Reviewers can audit baseline versus non-baseline live surfaces
- **WHEN** a reviewer inspects the observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL identify which selected family surfaces remain in
  current live summary
- **AND** it SHALL identify the remaining transport, queue, driver, and other
  residual live surfaces as `non-baseline live` rather than current operator
  baseline truth.

#### Scenario: External status commands distinguish cached truth from fresh readback
- **WHEN** the same index describes `GET_*` or read/status command surfaces
- **THEN** it SHALL distinguish cached onboard truth from fresh external
  observation/readback
- **AND** it SHALL make the retirement of `STORAGE_SCAN_NOW` and the fresh
  semantics of `STORAGE_GET_STATUS` explicit.
