## MODIFIED Requirements

### Requirement: Classic Component UT Pattern Skill

The repository SHALL provide a repo-local Codex skill named `classic-component-ut-pattern` under `.codex/skills/` that teaches how to build a complete classic F' component unit-test harness for a real repository component.

#### Scenario: Add or modify a real F' component

- **WHEN** an agent adds or modifies a class under `OBC/Components/` that derives from `*ComponentBase`
- **THEN** the skill SHALL direct the agent to confirm the target is a real component, add or update the classic harness files and `register_fprime_ut()` wiring, and cover the component interface through command, event, telemetry, scheduler, and output-port assertions as appropriate for that component

#### Scenario: Slice also contains helper or support logic

- **WHEN** the same implementation slice also contains helper/support logic such as parsers, stores, providers, or framing helpers
- **THEN** the skill SHALL direct the agent to keep helper logic under plain L1 tests and SHALL explicitly say those helper tests do not replace the required classic component harness

#### Scenario: Agent needs concrete repository examples

- **WHEN** the agent needs a starting pattern for how a repository component harness should look
- **THEN** the skill SHALL point to checked-in repository examples that use `TesterBase` / `GTestBase`, `UT_AUTO_HELPERS ON`, `register_fprime_ut()`, and contract-focused assertions
