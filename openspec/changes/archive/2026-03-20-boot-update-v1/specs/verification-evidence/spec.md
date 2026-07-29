## ADDED Requirements

### Requirement: Boot Implementation Evidence

The first boot/update implementation slice SHALL record its build and `BootManager` unit-test results under `evidence/records/boot-update-v1/`.

#### Scenario: Boot evidence is reviewable after implementation
- **WHEN** the boot/update change completes
- **THEN** reviewers SHALL be able to inspect the recorded commands, outcomes, and key verification notes from the repository documentation tree

### Requirement: Boot Hardware Gaps Are Explicit

Any boot/update behavior that still depends on Raspberry Pi boot-chain integration, real SD-card slot switching, or power-loss testing SHALL be called out explicitly in the boot evidence record using `Deferred-RPi` or `Blocked-HW`, whichever applies.

#### Scenario: Hosted verification stops short of real Raspberry Pi reboot flow
- **WHEN** the boot/update change completes with host-only verification
- **THEN** the remaining boot-chain and physical-media validation gaps SHALL be labeled with the correct constrained status term
