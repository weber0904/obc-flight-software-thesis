## ADDED Requirements

### Requirement: Persistent Fault Ring Evidence Is Reviewable
The verification evidence tree SHALL record reviewable local and hosted proof
for `persistent-fault-ring-v1`, including helper coverage, classic F'
component coverage, boot/recovery writer coverage, dual-copy fallback, same-
runtime-root relaunch proof, OpenSpec validation, and explicit deferred
boundaries.

#### Scenario: Focused tests cover store and owner contracts
- **WHEN** `persistent-fault-ring-v1` completes focused local verification
- **THEN** the evidence SHALL identify tests covering empty load, append,
  wraparound, newest-first readback, newer-copy corruption fallback, both-
  invalid empty load, and the `PersistentFaultManager` command or event or
  hosted-shell contract

#### Scenario: Hosted probe proves reboot-equivalent persistence only
- **WHEN** `persistent-fault-ring-v1` records hosted evidence
- **THEN** the evidence SHALL show a repository-owned probe that triggers shared
  recovery, relaunches the same runtime root, and reads back both pre-reboot
  recovery breadcrumbs and later boot breadcrumbs from the persistent ring
- **AND** it SHALL state that the verdict proves same-runtime-root relaunch
  persistence rather than target power-loss robustness

#### Scenario: Closeout gate stays explicit
- **WHEN** `persistent-fault-ring-v1` is ready for closeout
- **THEN** the evidence SHALL name the exact build, focused test, hosted probe,
  `openspec validate persistent-fault-ring-v1`, and
  `openspec validate --specs` commands used for the final verdict

