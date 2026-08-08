## ADDED Requirements

### Requirement: Persistent Fault Ring Uses Governed Recovery Root
The resource-storage baseline SHALL reserve
`<persistent-data-root>/recovery/` for the persistent fault ring and SHALL keep
`fault-ring-a.bin` and `fault-ring-b.bin` under that governed mutable root
outside immutable installed release payloads.

#### Scenario: Hosted and target profiles share the same logical recovery root
- **WHEN** the runtime launches under hosted or Raspberry Pi profiles with
  target-specific root overrides
- **THEN** the persistent fault ring SHALL resolve under the configured
  persistent-data root as `recovery/fault-ring-a.bin` and
  `recovery/fault-ring-b.bin`
- **AND** it SHALL NOT require a separate install-time release payload location

### Requirement: Persistent Fault Ring Stays Distinct From Other Stores
The governed storage model SHALL keep persistent fault history distinct from
boot metadata, staging data, logs, and official `.fdp` data-product storage.

#### Scenario: Recovery ring does not replace boot metadata or official data products
- **WHEN** the runtime persists recovery breadcrumbs in this change
- **THEN** the persistent fault ring SHALL use the governed recovery root under
  persistent-data
- **AND** it SHALL NOT rewrite boot metadata into the ring, write `.fdp` files,
  or reuse staging or logs roots as its primary store

