## ADDED Requirements

### Requirement: Mission Console SHALL Provide Dictionary-Backed Command Discovery
The Mission Console SHALL expose a context-scoped command catalog derived from
the active `AppTopologyDictionary.json` plus a curated overlay so operators can
discover mission-facing and engineering commands without memorizing raw command
names.

#### Scenario: Command catalog merges dictionary truth with curated UX metadata
- **WHEN** an operator or page client requests the Mission Console command
  catalog for an active context
- **THEN** the Gateway SHALL return dictionary-backed command name, opcode,
  command kind, and formal parameter truth for that context
- **AND** it SHALL merge curated group, label, description, default visibility,
  and field-hint metadata without changing the underlying command authority
  semantics.

### Requirement: Mission Console SHALL Provide A Searchable Schema-Driven Ops Workspace
The Mission Console SHALL provide a searchable `/ops` command workspace that
renders one operator input per formal parameter by default and keeps a visible
raw fallback path for uncurated or weakly inferred commands.

#### Scenario: Mission-first command selection renders parameter fields
- **WHEN** an operator filters and selects a command from the Mission Console
  `/ops` workspace
- **THEN** the UI SHALL filter commands by name, label, description, and
  argument metadata
- **AND** it SHALL render parameter inputs from the command schema while still
  serializing the selected values into the existing `commandArgs` array for the
  current POST command route.

#### Scenario: Engineering commands remain available behind show-all
- **WHEN** the operator enables the Mission Console show-all command toggle
- **THEN** the UI SHALL reveal hidden-by-default dictionary-backed engineering
  commands without removing the default mission-facing grouping behavior.

### Requirement: Mission Console SHALL Render Structured Operator Views
The Mission Console SHALL render structured, human-readable views on the
dashboard, readback, packet-lab, and surfaces pages instead of defaulting to
raw JSON blocks as the primary presentation layer.

#### Scenario: Dashboard and readback pages present structured evidence
- **WHEN** an operator views dashboard cards, recent events, or readback
  results
- **THEN** the Mission Console SHALL present primary values, freshness, and
  evidence using readable rows, badges, timelines, or structured result panels
- **AND** raw identifiers and secondary raw detail SHALL remain available
  without making JSON dumps the primary operator view.

#### Scenario: Packet-lab and surfaces pages present readable diagnostics
- **WHEN** an operator views packet-lab results, packet-lab history, surface
  registry state, or action history
- **THEN** the Mission Console SHALL present case descriptions, expected
  failures, observed evidence, lifecycle state, secure-state summaries, and
  compact history rows in structured views
- **AND** raw packet hex or backend detail SHALL remain secondary and
  collapsible.
