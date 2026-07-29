## ADDED Requirements

### Requirement: In-Place Fprime Bootstrap
The platform baseline SHALL support populating the existing OpenSpec-governed repository in place using `fprime-bootstrap project --populate --path . --tag v4.1.0`, and the resulting workspace SHALL preserve the `openspec/`, `obc-dev-spec/`, and project-local `.codex/skills/` structures.

#### Scenario: Bootstrap populates an existing governed workspace
- **WHEN** the project performs the initial F' bootstrap in this repository
- **THEN** it SHALL populate the current root directory and SHALL preserve the existing governance and narrative-document structures

### Requirement: Minimum Bootstrap Output
After the initial bootstrap, the workspace SHALL contain the generated F' baseline files needed for future work, including the project virtual environment, the root build/config files, and the near-term project directories required by the platform baseline.

#### Scenario: Baseline tree exists after bootstrap
- **WHEN** the bootstrap phase completes successfully
- **THEN** the workspace SHALL contain the generated virtual environment, root project files, and the baseline directories needed for OBC, simulator, script, documentation, and CI-oriented work

### Requirement: Bootstrap Interpreter Compatibility
The bootstrap virtual environment SHALL use a Python interpreter supported by the F' v4.1.0 dependency set. If the host default `python3` resolves to an unsupported version, the project SHALL create `fprime-venv/` with a compatible interpreter such as Python 3.13.

#### Scenario: Host default Python is too new
- **WHEN** the bootstrap host resolves `python3` to a version that cannot install the `v4.1.0` requirements successfully
- **THEN** the project SHALL recreate `fprime-venv/` with a compatible interpreter before running `fprime-util generate` and `fprime-util build`
