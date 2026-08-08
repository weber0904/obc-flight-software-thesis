## ADDED Requirements

### Requirement: Remote Pi-To-macOS CSP Evidence Is Recorded Separately From Ground Commands
The verification evidence tree SHALL record the commands, host/IP settings, node ids, observed Pi-side CSP reachability, and final verdict for the remote `Pi OBC -> macOS EPS/ADCS simulator` internal CSP path as a distinct result from any ground-driven command path.

#### Scenario: Remote internal CSP proof stays distinct from GDS command proof
- **WHEN** the repository validates the remote Pi-to-macOS simulator topology
- **THEN** the evidence SHALL identify the remote CSP host, CSP ports, node ids, and Pi-side observed `csp ping`, `eps get`, and `adcs get` results without treating that section as proof of `fprime-cli -> GDS` command dispatch

### Requirement: Target-Side GDS-Driven Subsystem Command Evidence Is Reviewable
The verification evidence tree SHALL record the commands, GDS ports, observed `fprime-cli` dispatch, Pi-side observed subsystem state changes, and final verdict for the target-side `fprime-cli -> GDS -> Pi OBC -> remote EPS/ADCS simulator` command path.

#### Scenario: Remote ground-driven subsystem command path is reviewable
- **WHEN** the remote Pi+macOS GDS-driven subsystem probe completes
- **THEN** reviewers SHALL be able to inspect the headless GDS launch command, the `fprime-cli` commands used for EPS and ADCS, and the Pi-side observed `eps get` / `adcs get` output that confirms the commanded state changes

#### Scenario: Adjacent paths remain out of scope
- **WHEN** the remote Pi+macOS GDS-driven subsystem probe passes
- **THEN** the evidence SHALL keep direct hosted-only GDS validation, external comm, GPS live UART, RF behavior, and future physical-bus validation separate
