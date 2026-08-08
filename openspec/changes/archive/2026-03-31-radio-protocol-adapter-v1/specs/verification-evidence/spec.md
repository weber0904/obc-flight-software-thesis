## ADDED Requirements

### Requirement: Radio Protocol Adapter Evidence
The verification evidence tree SHALL record which radio protocol adapter was selected during validation, together with the commands and observed regression outcome proving the default adapter still preserves the current comm behavior.

#### Scenario: Default adapter regression is reviewable
- **WHEN** the radio protocol adapter change completes
- **THEN** reviewers SHALL be able to inspect the selected adapter name, the validation commands, and the observed comm result from the repository evidence tree

#### Scenario: Future adapter work stays distinct
- **WHEN** the repository still lacks a real KISS or vendor-specific adapter
- **THEN** the evidence SHALL describe only the default-adapter regression result and SHALL keep future real-radio protocol work explicitly out of scope
