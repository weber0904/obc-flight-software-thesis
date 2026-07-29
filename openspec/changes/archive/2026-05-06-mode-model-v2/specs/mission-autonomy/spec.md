## ADDED Requirements

### Requirement: Mission Executive Redesign Is Deferred
The mission-autonomy capability SHALL treat the existing provisional `MissionExecutive` low-battery and detumble behavior as retired from the active mode-model-v2 runtime baseline until a later governed change rebuilds autonomy around the v2 mode model.

#### Scenario: Mode-model-v2 does not provide autonomy policy
- **WHEN** mode-model-v2 is complete
- **THEN** the active runtime SHALL NOT depend on the provisional `MissionExecutive` to command `HELL`, `SAFE`, `IDLE`, `PAYLOAD`, `TTC`, ADCS pointing, ADCS detumble, load shedding, or FDIR actions
- **AND** later autonomy changes SHALL define their own policy, thresholds, and verification evidence before claiming those behaviors

## REMOVED Requirements

### Requirement: First-Version Mission Executive
**Reason**: The provisional component was built around the old mode vocabulary and is intentionally retired from the active mode-model-v2 baseline.
**Migration**: Rebuild MissionExecutive behavior in a later governed change using the v2 mode model and explicit FDIR/autonomy requirements.

### Requirement: Low-Battery Entry To LOW_POWER
**Reason**: `LOW_POWER` is no longer a primary mission mode in mode-model-v2.
**Migration**: Define HELL/SAFE/IDLE SoC thresholds and hysteresis in `mode-autonomy-policy-v2`.

### Requirement: First-Version Sun-Safe Pointing Command
**Reason**: Sun-safe pointing belongs to the redesigned autonomy policy rather than the mode enum replacement.
**Migration**: Reintroduce sun-safe pointing requirements in the later MissionExecutive/FDIR policy change.

### Requirement: Low-Battery Policy Remains Narrow
**Reason**: The low-battery policy slice is retired as an active baseline behavior and cannot remain normative with `LOW_POWER` removed.
**Migration**: Replace this boundary with explicit HELL/SAFE/IDLE policy scope in `mode-autonomy-policy-v2`.

### Requirement: High-Rate Entry To DETUMBLE
**Reason**: Detumble autonomy belongs to the later redesigned MissionExecutive/FDIR policy and is not provided by mode-model-v2.
**Migration**: Reintroduce detumble autonomy requirements with updated mode-policy interactions in a later governed change.
