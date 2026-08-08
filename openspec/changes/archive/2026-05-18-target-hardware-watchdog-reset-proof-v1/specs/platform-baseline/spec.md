## ADDED Requirements

### Requirement: Raspberry Pi Baseline Supports Governed Hardware Watchdog Access

The governed Raspberry Pi baseline SHALL support repo-owned access to
`/dev/watchdog0` for the active non-root OBC service without changing the OBC
service to run as root.

#### Scenario: Service gains watchdog access through governed install config
- **WHEN** the repository installs or refreshes the active Raspberry Pi COMM CSP
  OBC service for hardware watchdog mode
- **THEN** the install path SHALL create or reuse a governed `watchdog` access
  group, install a repo-owned udev rule for `/dev/watchdog0`, and configure
  `obc-comm-csp-stack.service` to run with `SupplementaryGroups=watchdog`
- **AND** the active OBC service SHALL remain non-root

### Requirement: Raspberry Pi Launch Surface Exposes Hardware Watchdog Config

The governed Raspberry Pi launch surface SHALL expose explicit hardware watchdog
configuration while keeping hosted baselines disabled by default.

#### Scenario: Target launch passes hardware watchdog settings
- **WHEN** the active Raspberry Pi COMM CSP service launches the installed OBC
  stack in hardware watchdog mode
- **THEN** the launch path SHALL pass explicit watchdog mode, device path, and
  timeout settings to the OBC runtime
- **AND** hosted launch paths SHALL remain disabled by default unless explicitly
  overridden

### Requirement: Quiet Diagnostic Path SHALL Stay Probe-Owned

The governed Raspberry Pi launch/runtime surface SHALL keep any
diagnostic-only quiet egress control probe-owned, default off, and excluded
from the normal operator-facing baseline. It MAY expose that control only for
repo-owned target watchdog proof execution.

#### Scenario: Temporary quiet override suppresses unsolicited egress only for the proof
- **WHEN** the repository runs the target hardware-watchdog reset proof on the
  active Raspberry Pi service baseline
- **THEN** the proof MAY temporarily enable a diagnostic quiet egress override
  that suppresses unsolicited packet/file downlink on the active ground path
- **AND** the probe SHALL restore the service to normal non-quiet mode before
  exit
- **AND** the default installed service behavior SHALL remain unchanged when the
  override is absent

### Requirement: Target Hardware Watchdog Proof Stays Distinct From Other Reset Claims

The governed Raspberry Pi baseline SHALL record hardware watchdog reset as a
distinct proof boundary rather than collapsing it into service-managed restart,
Linux reboot, or power-loss recovery.

#### Scenario: Target hardware watchdog evidence stays narrowly scoped
- **WHEN** the active Raspberry Pi baseline cites hardware watchdog reset proof
- **THEN** the evidence SHALL identify the path as board reset caused by the
  Raspberry Pi hardware watchdog under the active `TopCcsds` service baseline
- **AND** it SHALL remain distinct from service-managed `R2` process restart,
  generic Linux reboot, bootloader or partition handoff, and power-loss
  recovery claims
