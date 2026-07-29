## ADDED Requirements

### Requirement: Raspberry Pi CSP And Comm Baseline Evidence
The verification evidence tree SHALL record the commands, target device paths, CSP node ids, UART settings, observed CSP reachability, observed EPS/ADCS state, observed comm/radio response, and final verdict for the combined Raspberry Pi CSP + external comm baseline validation.

#### Scenario: Target CSP and comm evidence is reviewable
- **WHEN** the combined target probe completes
- **THEN** the evidence SHALL identify the host serial device, target serial device, target runtime root, CSP hub ports, EPS/ADCS node ids, and observed command output that proves the scoped paths

#### Scenario: Adjacent paths remain out of scope
- **WHEN** the combined target probe passes
- **THEN** the evidence SHALL keep GDS, GPS live UART, real EPS/ADCS hardware, RF behavior, and vendor radio control semantics separate
