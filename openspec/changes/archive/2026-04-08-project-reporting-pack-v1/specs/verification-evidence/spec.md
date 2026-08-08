## MODIFIED Requirements

### Requirement: Evidence Capture
The project SHALL preserve reviewable evidence for both automated and manual testing, including test identity, execution time, result summary, commands or steps used, expected outcomes, observed outcomes, and the final verdict. Governance or reporting-package changes that do not alter runtime behavior SHALL still capture the checked-in truth sources consulted, the review surfaces updated, and the local verification commands used to keep that reporting package auditable.

#### Scenario: Reporting package evidence is reviewable
- **WHEN** a change adds or refreshes a checked-in project-reporting package
- **THEN** the evidence record SHALL name the repo-truth sources used, the reporting artifacts updated, and the validation commands that kept the package aligned with the formal baseline
