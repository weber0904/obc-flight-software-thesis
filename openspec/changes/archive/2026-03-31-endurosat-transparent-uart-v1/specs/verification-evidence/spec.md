## ADDED Requirements

### Requirement: Legacy EnduroSat Transparent UART Evidence
The verification evidence tree SHALL record the commands, selected serial devices, UART settings, payload-exchange behavior, and final verdict for the governed legacy EnduroSat transparent-UART validation path.

#### Scenario: Transparent-UART validation is reviewable
- **WHEN** the legacy EnduroSat transparent-UART change completes
- **THEN** reviewers SHALL be able to inspect the host peer launch path, the Raspberry Pi launch path, the selected host and target serial devices, the UART settings used, and the observed payload exchange outcome from the repository evidence tree

### Requirement: Legacy Transparent Scope Boundaries Stay Explicit
The evidence for the legacy EnduroSat transparent-UART path SHALL explicitly distinguish validated transparent payload transport from any still-unimplemented legacy control/configuration behavior or newer `csp-es`-oriented hardware paths.

#### Scenario: Legacy transparent validation does not imply full radio integration
- **WHEN** the legacy transparent-UART path passes
- **THEN** the evidence SHALL mark only the validated transparent data-path behavior as passed and SHALL keep the remaining legacy control-plane and future-generation hardware work explicit as out of scope or `Blocked-HW`, whichever applies
