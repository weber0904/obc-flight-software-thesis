## ADDED Requirements

### Requirement: Target Hardware Watchdog Feed Uses Existing WatchdogSupervisor Truth

The mission-autonomy capability SHALL keep `WatchdogSupervisor` as the single
owner of watchdog feed eligibility, and the Raspberry Pi hardware watchdog path
SHALL consume that existing truth instead of introducing a second target-only
watchdog policy owner.

#### Scenario: Target hardware stroking follows supervisor feed eligibility
- **WHEN** the active Raspberry Pi baseline enables hardware watchdog mode
- **THEN** only `WatchdogSupervisor` feed-eligible cycles SHALL stroke the
  hardware watchdog device
- **AND** target integration SHALL NOT bypass `WatchdogSupervisor` with an
  independent timer or separate feed policy

### Requirement: Watchdog-Source R6 Uses Hardware Reset On Enabled Target Mode

The mission-autonomy capability SHALL allow watchdog-source `R6_OBC_REBOOT` to
use Raspberry Pi hardware watchdog timeout on the governed enabled target mode
while preserving existing process-restart and non-watchdog reboot semantics.

#### Scenario: Watchdog-source R6 does not exit 32 in hardware-watchdog mode
- **WHEN** hardware watchdog mode is enabled on the active Raspberry Pi target
- **AND** a watchdog-source incident reaches `R6_OBC_REBOOT`
- **THEN** `RecoveryExecutor` SHALL persist reboot intent and recovery metadata
- **AND** it SHALL stop further watchdog stroking
- **AND** it SHALL NOT request runtime exit code `32` for that watchdog-source
  incident

#### Scenario: Existing R2 and non-watchdog R6 semantics remain intact
- **WHEN** an `R2_RESTART_SOFTWARE_COMPONENT` incident is raised
- **THEN** the runtime SHALL still request exit `31`
- **AND** watchdog hardware mode SHALL NOT change that behavior
- **WHEN** a non-watchdog incident reaches `R6_OBC_REBOOT`
- **THEN** the runtime SHALL still request exit `32`

### Requirement: Proof Trigger For Target Hardware Reset Remains Bounded

The mission-autonomy capability SHALL allow a bounded proof-only watchdog
suppression trigger for repository-owned target evidence when the active target
topology lacks a clean natural stale-source trigger.

#### Scenario: Proof trigger is not generalized into operator recovery control
- **WHEN** the repository records target hardware watchdog reset evidence
- **THEN** it MAY use a bounded proof-only trigger to suppress a watchdog source
  heartbeat path
- **AND** that trigger SHALL remain scoped to repository-owned proof workflows
- **AND** it SHALL NOT become a generic operator-driven restart or reboot
  command surface in this change
