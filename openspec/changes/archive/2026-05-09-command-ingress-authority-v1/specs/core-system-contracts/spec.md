## ADDED Requirements

### Requirement: Command Ingress Authority Gates Routed Commands
The core system contract SHALL provide OBC-side command ingress authority enforcement for routed `Fw.Com` command packets before they reach `Svc::CommandDispatcher`.

#### Scenario: Gate is inserted before command dispatch
- **WHEN** the default CCSDS topology routes F Prime command packets
- **THEN** `ComCcsds.fprimeRouter.commandOut` SHALL connect to `CommandIngressAuthority`
- **AND** allowed commands SHALL be forwarded from `CommandIngressAuthority` to `CdhCore.cmdDisp.seqCmdBuff`.

#### Scenario: Legacy topology uses the same gate behavior
- **WHEN** the legacy ComFprime topology routes F Prime command packets
- **THEN** `ComFprime.fprimeRouter.commandOut` SHALL connect to `CommandIngressAuthority`
- **AND** allowed commands SHALL be forwarded from `CommandIngressAuthority` to `CdhCore.cmdDisp.seqCmdBuff`.

### Requirement: Command Status Semantics Are Preserved
The command ingress authority gate SHALL preserve F Prime command source/status semantics for both forwarded and denied commands.

#### Scenario: Forwarded commands preserve context
- **WHEN** an allowed command is forwarded to `Svc::CommandDispatcher`
- **THEN** the original `Fw.Com.context` SHALL be forwarded unchanged
- **AND** the status returned from `CmdDispatcher.seqCmdStatus` SHALL be returned to the same upstream status sink.

#### Scenario: Denied commands return synthetic status
- **WHEN** the gate denies a command before dispatch
- **THEN** it SHALL NOT forward the command to `Svc::CommandDispatcher`
- **AND** it SHALL return exactly one synthetic `Fw.CmdResponse` through the same upstream status path.

#### Scenario: Synthetic status response mapping is deterministic
- **WHEN** a command is denied by policy
- **THEN** the synthetic response SHALL be `VALIDATION_ERROR`
- **WHEN** a restricted ingress receives a malformed command packet
- **THEN** the synthetic response SHALL be `FORMAT_ERROR`
- **WHEN** authority configuration is missing or invalid
- **THEN** the synthetic response SHALL be `EXECUTION_ERROR`
- **WHEN** restricted ingress receives an unknown opcode
- **THEN** the synthetic response SHALL be `INVALID_OPCODE`.

### Requirement: Restricted Command Ingress Fails Closed
The command ingress authority gate SHALL fail closed for restricted or unknown configured ingress roles.

#### Scenario: UHF backup denied commands do not reach subsystem handlers
- **WHEN** a `UHF + BACKUP` configured ingress receives a command outside the v1 allowlist
- **THEN** the command SHALL be denied before `Svc::CommandDispatcher`
- **AND** the target subsystem command handler SHALL NOT run.

#### Scenario: Missing config does not default to S-band primary
- **WHEN** authority configuration is missing, invalid, or unknown
- **THEN** the gate SHALL deny command ingress by default
- **AND** it SHALL NOT silently assume `SBAND + PRIMARY`.

### Requirement: Authority Evidence Is Bounded And Observable
The command ingress authority gate SHALL emit bounded runtime evidence for denied commands without allowing rejection floods to dominate event output.

#### Scenario: Rejection event is throttled
- **WHEN** repeated command rejections occur
- **THEN** the gate SHALL throttle rejection events
- **AND** it SHALL continue to update rejection counters for every rejected command.

#### Scenario: File and unknown packet authority is deferred
- **WHEN** command ingress authority is documented or cited
- **THEN** it SHALL be described as applying only to routed `Fw.Com` command packets
- **AND** it SHALL NOT claim authority over `FprimeRouter.fileOut`, `FprimeRouter.unknownDataOut`, direct file packet uplink, unknown packet routing, full uplink authority, or full link authority.
