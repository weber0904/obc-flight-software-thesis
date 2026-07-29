## ADDED Requirements

### Requirement: Mode Model V2 Evidence
The verification evidence SHALL record reviewable local evidence for the primary mode enum replacement, runtime mode command/status behavior, live beacon mode decode behavior, HK trend mode decode behavior, and explicit exclusions for deferred autonomy and communication work.

#### Scenario: Mode model evidence is reviewable
- **WHEN** mode-model-v2 completes local verification
- **THEN** the evidence SHALL identify the generated/build/test commands, focused mode command coverage, invalid mode rejection, beacon mode encode/decode coverage, beacon schema version, HK trend payload version, and HK trend mode decode coverage

#### Scenario: Deferred scope is explicit
- **WHEN** mode-model-v2 evidence is recorded
- **THEN** it SHALL state that the evidence does not prove COMM split-link behavior, CCSDS behavior, storage-policy behavior, FDIR, MissionExecutive autonomy, target hardware behavior, RF behavior, reliable transfer, or pass scheduling

## REMOVED Requirements

### Requirement: Low-Battery Autonomy Evidence
**Reason**: The low-battery `LOW_POWER` autonomy behavior is retired from the active mode-model-v2 baseline.
**Migration**: Record new HELL/SAFE/IDLE autonomy evidence in `mode-autonomy-policy-v2`.

### Requirement: Detumbling Autonomy Evidence
**Reason**: Detumble autonomy is deferred with the MissionExecutive/FDIR redesign.
**Migration**: Record new detumble-policy evidence when the redesigned autonomy behavior is implemented.
