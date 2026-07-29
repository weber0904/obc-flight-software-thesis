# codex-skills Specification

## Purpose

Define the repo-local Codex skills that capture stable development workflows for this repository without collapsing unrelated work into a single oversized skill.
## Requirements
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
- **THEN** the skill SHALL direct the agent to add or update the corresponding `docs/test-records/...` evidence and `docs/verification-path-registry.md` entry

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

### Requirement: Hosted Probe GDS CLI Startup Order

The `hosted-probe-workflow` skill SHALL define the standard startup order for repository-owned hosted probes that depend on headless GDS, `fprime-cli` listeners, gateway processes, and hosted OBC runtimes.

#### Scenario: Probe observes GDS CLI events or channels

- **WHEN** a hosted probe expects `fprime-cli events` or `fprime-cli channels` output as part of its verdict
- **THEN** the skill SHALL direct the agent to start headless GDS, wait for configured GDS readiness, start the relevant `fprime-cli` listeners, allow a short listener settle interval, start gateways, COMM nodes, and target simulators before the OBC/runtime path, and only then start the OBC/runtime process that produces the traffic under observation
- **AND** the skill SHALL warn the agent not to replace missed listener output with OBC runtime logs when the formal verdict requires ground-side CLI observation

#### Scenario: Probe allocates local GDS or TTS ports

- **WHEN** a hosted probe chooses local GDS, TTS, or adjacent runtime ports
- **THEN** the skill SHALL prefer a real socket bind check over process-list checks alone and SHALL tell the agent to avoid parallel runs that share GDS, TTS, ZMQ, serial, or runtime-root resources
