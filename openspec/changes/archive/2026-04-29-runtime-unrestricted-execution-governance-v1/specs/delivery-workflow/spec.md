## ADDED Requirements

### Requirement: Execution Permission Classification
The delivery workflow SHALL classify agent-driven repository commands before execution, and runtime-bearing or network-bearing command classes SHALL default to unrestricted execution instead of relying on an initial sandboxed failure to discover required permissions.

#### Scenario: Repository-owned runtime verification commands start unrestricted
- **WHEN** an agent runs a repository-owned probe, stack script, or the shared verification gate and that command may start hosted runtimes, bind local ports, open PTYs or serial devices, touch SocketCAN-adjacent interfaces, or otherwise require local runtime resources outside pure file access
- **THEN** the workflow SHALL treat that command as unrestricted by default rather than first attempting the same command under the default sandbox

#### Scenario: Networked operational commands start unrestricted
- **WHEN** an agent runs a repository command that depends on external network access or remote sessions such as `gh`, `ssh`, remote Raspberry Pi scripts, or equivalent repository-operated tooling
- **THEN** the workflow SHALL treat that command as unrestricted by default rather than waiting for a sandbox-related network failure

#### Scenario: Static file-oriented commands may stay sandboxed
- **WHEN** an agent runs file inspection, file editing, OpenSpec validation, repo-local consistency checks, or other commands that operate entirely within the writable workspace without binding ports, opening device handles, or requiring external network access
- **THEN** the workflow MAY keep those commands in the default sandbox
