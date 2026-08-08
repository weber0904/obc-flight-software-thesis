## MODIFIED Requirements

### Requirement: Hosted Legacy Command Ingress Authority Compatibility Path Is Registered

The verification-path registry SHALL keep hosted legacy command-ingress
authority families registered only as historical compatibility evidence and
SHALL NOT present them as current maintained closeout gates once secure-auth-
only retirement is complete.

#### Scenario: Registry identifies historical envelope sequence-enforcement boundary
- **WHEN** reviewers inspect hosted legacy command-ingress authority entries
- **THEN** the registry SHALL state that those entries prove historical legacy
  envelope metadata or sequence behavior only
- **AND** it SHALL state that those entries are not current maintained
  secure-baseline authority.

### Requirement: Hosted Legacy Command Ingress Authority Path Includes Session Lifecycle Boundary

The verification-path registry SHALL preserve hosted legacy session-lifecycle
records only as archived historical compatibility evidence after public
`SESSION_OPEN` retirement.

#### Scenario: Registry identifies historical lifecycle proof boundary
- **WHEN** reviewers inspect hosted legacy lifecycle records after this change
- **THEN** the registry SHALL state that the path proved explicit
  `SESSION_OPEN` behavior, source-epoch replacement, and reopened-session
  history on the old path
- **AND** it SHALL identify that boundary as archived historical compatibility
  evidence rather than current maintained authority.

#### Scenario: Registry keeps lifecycle path out of maintained gate inventory
- **WHEN** the current maintained secure baseline gate set is described
- **THEN** the registry SHALL exclude legacy hosted command-ingress lifecycle
  wrappers from that maintained closeout-gate set.

### Requirement: Hosted Command Ingress Authority Path Includes Authenticated Envelope Boundary

The verification-path registry SHALL preserve authenticated envelope v1 proof
families as historical compatibility citations only once the current runtime no
longer accepts legacy command-envelope v1 traffic.

#### Scenario: Registry identifies authenticated legacy scope as historical
- **WHEN** reviewers inspect the hosted authenticated legacy command-ingress
  family after retirement
- **THEN** the registry SHALL state that the path proved the old authenticated
  envelope ordering on the legacy path
- **AND** it SHALL NOT describe that path as a current maintained operator or
  closeout gate.

### Requirement: Hosted Command Ingress Authority Path Includes Persistent Freshness Boundary

The verification-path registry SHALL preserve hosted persistent-freshness
records only as historical compatibility evidence once current runtime restart
behavior is governed by fresh secure-auth re-bootstrap instead of persisted
legacy reopen-floor semantics.

#### Scenario: Registry identifies persistent freshness as historical
- **WHEN** reviewers inspect the hosted persistent-freshness entry after this
  change
- **THEN** the registry SHALL state that the path proved historical persisted
  legacy reopen-floor behavior only
- **AND** it SHALL NOT describe that entry as a current maintained gate.

### Requirement: COMM Policy Clarifications Cite Governing Evidence Or Stay Non-Claims

The verification-path registry SHALL require current COMM policy wording to
cite governing secure-auth evidence for maintained command-session truth, while
keeping retained legacy `SESSION_OPEN(seq0)` citations historical-only and
keeping retired timing wrappers out of the maintained gate set.

#### Scenario: Current secure policy cites maintained secure-auth evidence
- **WHEN** the repository cites current command-session, uplink-authority, or
  observability-governance behavior
- **THEN** the wording SHALL point to the governing maintained secure-auth
  registry entries or archived evidence for those exact paths
- **AND** it SHALL NOT require retained legacy `SESSION_OPEN(seq0)` proof as a
  current maintained dependency.

#### Scenario: Retired timing wrappers stay out of maintained closeout authority
- **WHEN** reviewers inspect current maintained gate wording after this change
- **THEN** the registry SHALL classify
  `target-timing-empirical-ceiling-freeze-v1` and
  `target-timing-wcet-profile-proof-v1` as retired historical timing paths
- **AND** it SHALL NOT present them as current maintained closeout gates for
  unrelated product changes.
