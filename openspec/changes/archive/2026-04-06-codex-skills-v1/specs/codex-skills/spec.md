## ADDED Requirements

### Requirement: Change Closeout Skill

The repository SHALL provide a repo-local Codex skill named `change-closeout` under `.codex/skills/` that guides an agent through the governed post-implementation closeout workflow for this repository.

#### Scenario: Close out a completed change

- **WHEN** implementation for a repository change is complete
- **THEN** the skill SHALL instruct the agent to read the repository workflow and verification-governance documents, run the fresh local verification gate, rerun focused repository-owned probes after the fresh build, run `openspec validate`, sync/archive the change, prepare a clean Conventional Commit boundary, and treat push/PR/CI green as the final completion gate

#### Scenario: Probe results conflict with a recent code change

- **WHEN** a focused probe result appears inconsistent with a recent implementation change
- **THEN** the skill SHALL direct the agent to rebuild first and only trust probe output gathered after the fresh build

### Requirement: Hosted Probe Workflow Skill

The repository SHALL provide a repo-local Codex skill named `hosted-probe-workflow` under `.codex/skills/` that guides an agent through the governed design and rerun workflow for repository-owned hosted probes.

#### Scenario: Create or update a hosted probe

- **WHEN** an agent adds or modifies a repository-owned hosted probe for a runtime path
- **THEN** the skill SHALL direct the agent to identify the exact path under test, check the verification-path registry and governing evidence first, prefer a repository-owned headless probe script over ad hoc piped REPL interaction, isolate runtime roots and ports, and keep hosted-probe assertions bounded and reviewable

#### Scenario: Probe uses GDS or local sockets

- **WHEN** a hosted probe depends on `fprime-gds` or other local socket listeners
- **THEN** the skill SHALL direct the agent to separate direct `OBC -> GDS` connectivity from `fprime-cli -> GDS` command/uplink connectivity and keep the local socket chain under consistent privilege conditions before interpreting feature behavior

#### Scenario: Probe formally proves a new path

- **WHEN** the hosted probe becomes formal evidence for a verification path not yet registered in this repository
- **THEN** the skill SHALL direct the agent to add or update the corresponding `evidence/records/...` evidence and `evidence/verification-path-registry.md` entry
