## ADDED Requirements

### Requirement: BootManager Records Persistent Boot Breadcrumbs
The boot/update subsystem SHALL append bounded persistent fault ring
breadcrumbs for boot-observed startup and recovery-boot-ack stabilization while
keeping boot metadata as the authoritative boot and reset truth.

#### Scenario: Boot-observed breadcrumb reuses persisted boot truth
- **WHEN** the runtime starts and `BootManager` reloads persisted boot metadata
- **THEN** `BootManager` SHALL append a boot-observed breadcrumb that reflects
  the loaded reset-cause, boot-count, consecutive-reset-count,
  last-recovery-source, and last-recovery-level truth
- **AND** the breadcrumb SHALL remain derived from rather than replace the owned
  boot metadata file

#### Scenario: Recovery boot acknowledgement stays additive
- **WHEN** `BootManager` later clears the bounded repeated-recovery restart
  clamp after a stable runtime window
- **THEN** it SHALL append a recovery-boot-ack breadcrumb to the persistent
  fault ring
- **AND** it SHALL preserve the existing `BOOT_STATUS`, `GET_RESET_CAUSE`, and
  `GET_BOOT_COUNT` truth surfaces instead of moving them into the ring

