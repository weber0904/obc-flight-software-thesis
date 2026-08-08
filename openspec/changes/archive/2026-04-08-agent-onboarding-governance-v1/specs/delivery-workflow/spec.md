## ADDED Requirements

### Requirement: Repository Agent Onboarding Entrypoint
The repository SHALL provide a repo-root `AGENTS.md` file that acts as the first checked-in onboarding surface for future agents, and that file SHALL identify the canonical workflow, validation, and skills sources instead of forcing agents to reconstruct them from chat history alone.

#### Scenario: New agent finds the governed workflow from the repo root
- **WHEN** a new agent starts from a fresh checkout of the repository
- **THEN** `AGENTS.md` SHALL tell that agent which checked-in files define the repository purpose, formal delivery workflow, validation-path rules, and repo-local skills

#### Scenario: AGENTS entrypoint does not replace canonical rules
- **WHEN** `AGENTS.md` summarizes repository workflow expectations
- **THEN** it SHALL point to the canonical detailed sources in the repository instead of acting as a competing second workflow specification

### Requirement: Agent Entrypoint Check
The delivery workflow SHALL provide a repo-local onboarding check that verifies `AGENTS.md` exists and still references the required checked-in workflow and validation sources.

#### Scenario: Missing required onboarding reference fails the check
- **WHEN** `AGENTS.md` no longer points to one of the required canonical sources
- **THEN** the repo-local onboarding check SHALL fail and report the missing reference
