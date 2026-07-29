## ADDED Requirements

### Requirement: Authority Vocabulary Evidence Is Traceable
Verification evidence for command authority vocabulary SHALL connect each OpenSpec requirement to policy behavior and at least one focused test or probe expectation.

#### Scenario: Traceability is reviewable
- **WHEN** reviewers inspect `link-authority-vocabulary-v1` evidence
- **THEN** they SHALL be able to trace vocabulary requirements to policy source behavior, dictionary catalog generation, and focused tests.

### Requirement: Authority Catalog Coverage Is Verified
Verification evidence SHALL show that both active OBC topology dictionaries are covered by the command authority policy.

#### Scenario: Dictionary coverage is enforced
- **WHEN** focused authority catalog tests run
- **THEN** they SHALL verify that every command in the default CCSDS and legacy ComFprime topology dictionaries has a classification
- **AND** they SHALL verify that generated runtime opcode catalog output matches the checked-in policy source and current dictionary command names.
