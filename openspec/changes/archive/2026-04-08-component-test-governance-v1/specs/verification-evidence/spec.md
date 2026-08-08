## ADDED Requirements

### Requirement: Component And Helper Coverage Stay Distinct
The verification model SHALL distinguish classic component L2 coverage for real F' components from plain L1 helper coverage for support logic, and the repository SHALL keep those layers reviewable as separate categories.

#### Scenario: Helper tests do not satisfy missing component coverage
- **WHEN** a real repository component lacks classic F' L2 coverage but helper tests exist for its supporting logic
- **THEN** the repository SHALL still report the component L2 gap explicitly instead of treating the helper tests as a complete substitute

#### Scenario: Helper modules keep direct logic tests
- **WHEN** a non-component helper such as a parser, store, framing helper, or topology provider contains nontrivial behavior
- **THEN** the repository SHALL keep direct L1 tests for that helper even when the owning component also has classic F' L2 coverage
