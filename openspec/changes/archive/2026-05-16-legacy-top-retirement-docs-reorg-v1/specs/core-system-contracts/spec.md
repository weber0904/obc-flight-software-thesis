## MODIFIED Requirements

### Requirement: Command Authority Policy Uses Fully Qualified Command Names
The command authority policy source SHALL key command classification by fully
qualified FPP JSON dictionary command names and SHALL derive runtime opcode
lookup from dictionary data for maintained active topology dictionaries.

#### Scenario: Short command names are not policy keys
- **WHEN** policy classifies commands from the active topology dictionaries
- **THEN** it SHALL use `commands[].name` values such as
  `OBCApp.modeManager.MODE_GET`
- **AND** it SHALL NOT rely on suffix-only names such as `GET_STATUS`
- **AND** it SHALL NOT hand-maintain global opcode numbers as the policy source
  of truth.

#### Scenario: Every active command is classified
- **WHEN** repository tests inspect the maintained active CCSDS topology
  dictionary
- **THEN** every active command entry SHALL have a command authority
  classification
- **AND** missing classifications SHALL fail the focused policy/catalog tests
- **AND** retired `OBCAppComFprimeLegacy.*` command names SHALL NOT be required
  or retained as current policy entries.

### Requirement: Command Ingress Authority Gates Routed Commands
The core system contract SHALL provide OBC-side command ingress authority
enforcement for routed `Fw.Com` command packets before they reach
`Svc::CommandDispatcher` on the maintained active topology.

#### Scenario: Gate is inserted before command dispatch
- **WHEN** the default CCSDS topology routes F Prime command packets
- **THEN** `ComCcsds.fprimeRouter.commandOut` SHALL connect to
  `CommandIngressAuthority`
- **AND** allowed commands SHALL be forwarded from `CommandIngressAuthority` to
  `CdhCore.cmdDisp.seqCmdBuff`.

#### Scenario: Retired legacy topology is not a current gate obligation
- **WHEN** archived legacy `OBC/Top` or `OBC_ComFprimeLegacy` evidence is
  reviewed
- **THEN** it SHALL NOT create a current command-ingress authority wiring or
  policy obligation for the retired topology.
