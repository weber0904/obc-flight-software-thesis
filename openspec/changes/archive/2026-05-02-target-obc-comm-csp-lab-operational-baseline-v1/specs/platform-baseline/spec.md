## ADDED Requirements

### Requirement: Lab Target COMM CSP Operational Baseline
The platform baseline SHALL provide a lab target operational baseline where `obc.local` can boot into a service-managed installed OBC release that uses the proven COMM SocketCAN path as the ground link, while explicitly excluding final flight deployment claims.

#### Scenario: OBC lab service starts the installed release
- **WHEN** the lab target COMM CSP service is installed and enabled on `obc.local`
- **THEN** the service SHALL start OBC from the installed `current` release or configured install-root equivalent
- **AND** it SHALL run the OBC runtime as a non-root runtime process
- **AND** it SHALL configure `GROUND_LINK_MODE=comm-csp`, COMM node `4`, `CSP_TRANSPORT=socketcan`, `obc.local:can0`, and live GPS UART defaults

#### Scenario: Existing installed service is not left racing the lab service
- **WHEN** the helper enables the lab target COMM CSP OBC service
- **THEN** it SHALL disable and stop the older `obc-installed-stack.service`
- **AND** rollback SHALL be documented as disabling/stopping the lab service and re-enabling the older installed service

#### Scenario: Lab baseline is not final flight deployment
- **WHEN** the repository describes this baseline
- **THEN** it SHALL call it a lab target operational baseline
- **AND** it SHALL state that it is not a final flight deployment baseline

### Requirement: Transitional Runtime Identity Boundary
The lab target operational baseline SHALL keep long-running OBC and subsystem runtime processes non-root, while allowing the current workspace-service stage to run those processes as `operator` because the workspace and native build outputs are owned by that user.

#### Scenario: Workspace service stage uses current workspace owner
- **WHEN** OBC or subsystem runtime processes run under the lab service-managed workflow
- **THEN** they MAY run as `operator` for this workspace-based stage
- **AND** root SHALL be reserved for bounded provisioning actions such as CAN interface bring-up

#### Scenario: Future host-install runtime identity remains separate
- **WHEN** a future change introduces host-install service management beyond the workspace stage
- **THEN** it SHALL prefer dedicated non-login runtime users such as `obc-runtime` and `subsystem-runtime` rather than human workspace owners

### Requirement: Lab CAN Provisioning Is Transitional
The lab target operational baseline SHALL provide CAN bring-up as root-owned oneshot service templates for development and lab repeatability, while keeping final OS-level CAN provisioning out of scope.

#### Scenario: CAN oneshot configures the proven timing by default
- **WHEN** the lab CAN provisioning oneshot runs
- **THEN** it SHALL bring the configured CAN interface up with default timing equivalent to `bitrate 500000 dbitrate 2000000 fd on`
- **AND** the timing SHALL remain configurable through the repo-owned helper or service environment

#### Scenario: CAN oneshot is not described as flight provisioning
- **WHEN** specs, runbooks, or evidence describe the oneshot services
- **THEN** they SHALL describe them as lab/development provisioning helpers
- **AND** they SHALL NOT describe them as the final flight OS networking model

### Requirement: Release And Deployment Responsibility Split
The lab target operational baseline SHALL distinguish the OBC installable bundle from the broader repo-governed lab deployment artifacts.

#### Scenario: OBC package owns only OBC-installed artifacts
- **WHEN** the Raspberry Pi OBC package is created for this baseline
- **THEN** it SHALL include the OBC binary, dictionary, metadata, OBC comm-csp launch profile, and OBC service template
- **AND** it SHALL NOT claim that subsystem service templates, CAN oneshot templates, ground launchers, or operator runbooks are contents of the OBC tarball

#### Scenario: Repo-governed lab artifacts complete the path
- **WHEN** an operator follows the lab target operational runbook
- **THEN** subsystem service templates, CAN oneshot templates, ground launcher, and runbook SHALL be treated as repo-governed lab deployment artifacts outside the OBC installable tarball boundary
