## ADDED Requirements

### Requirement: Command Authority Uses Configured Ingress Source Indexes
The command ingress authority gate SHALL support topology-configured authority source mapping by input port index.

#### Scenario: Each ingress port has independent configured source
- **WHEN** a command enters `CommandIngressAuthority.seqCmdBuffIn[N]`
- **THEN** the gate SHALL evaluate that command using only the authority config assigned to ingress port `N`
- **AND** it SHALL NOT infer source identity from `Fw.Com.context`, gateway metadata, opcode, or packet body.

#### Scenario: Legacy component configuration targets port zero only
- **WHEN** `CommandIngressAuthority.configure(config)` is called
- **THEN** it SHALL clear all ingress source configs
- **AND** it SHALL configure only ingress port `0` with `config`.

#### Scenario: Unconfigured ingress ports fail closed
- **WHEN** a command enters an unconfigured or invalid ingress port
- **THEN** the gate SHALL NOT forward the command to `Svc::CommandDispatcher`
- **AND** it SHALL return exactly one synthetic `Fw.CmdResponse` through the same port-indexed status path
- **AND** the synthetic response SHALL be `EXECUTION_ERROR`.

### Requirement: Source Index Evidence Is Observable Without Claiming Trusted Provenance
The command ingress authority gate SHALL expose bounded rejection evidence that identifies the configured ingress source index and configured link identity.

#### Scenario: Rejection event includes configured source index
- **WHEN** the gate rejects a command
- **THEN** `COMMAND_AUTHORITY_REJECTED` SHALL include the ingress port index and configured link identity
- **AND** it SHALL include the configured link role, command class, rejection reason, and command response.

#### Scenario: Context remains command status correlation only
- **WHEN** a command is forwarded or rejected
- **THEN** `Fw.Com.context` SHALL be preserved unchanged for status correlation
- **AND** it SHALL NOT be treated as mission source identity, session identity, sequence number, or authority evidence.

#### Scenario: Hosted proof remains single ingress
- **WHEN** `command-ingress-source-index-v1` evidence cites the hosted default CCSDS path
- **THEN** the evidence SHALL describe that hosted topology as proving only configured ingress port `0`
- **AND** it SHALL NOT claim trusted source, per-packet provenance, physical UHF provenance, or simultaneous dual-link runtime proof.
