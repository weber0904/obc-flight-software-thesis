## ADDED Requirements

### Requirement: Boot Trust Chain Validation Path Is Registered

The verification-path registry SHALL include a dedicated entry for any boot trust-chain validation path proven by `boot-trust-chain-v1`, with hosted and Raspberry Pi proof boundaries kept distinct.

#### Scenario: Hosted boot trust path can be cited without target overclaiming
- **WHEN** the hosted boot trust-chain probe passes
- **THEN** the registry SHALL identify the path as hosted `BootManager` staged-image manifest verification and activation lifecycle evidence
- **AND** it SHALL state that the path does not prove Raspberry Pi target reboot, bootloader handoff, hardware secure boot, physical SD-card switching, or power-loss behavior.

#### Scenario: Raspberry Pi boot trust path is registered only when proven
- **WHEN** a Raspberry Pi boot trust-chain probe passes on target hardware
- **THEN** the registry SHALL identify the target path separately from hosted evidence
- **AND** it SHALL state exactly whether the probe covered target process restart, installed/autostart runtime roots, or only the interactive target boot-update path.

#### Scenario: Failed or unrun target probe is not registered as proven
- **WHEN** Raspberry Pi target validation is not executed or does not pass
- **THEN** the registry SHALL NOT register a target boot trust-chain path as formally proven
- **AND** the evidence SHALL keep the remaining target scope explicit as deferred or constrained.
