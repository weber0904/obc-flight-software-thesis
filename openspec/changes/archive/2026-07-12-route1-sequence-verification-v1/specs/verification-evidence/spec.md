## ADDED Requirements

### Requirement: Route 1 Sequence Evidence SHALL Be Independently Reviewable

Route 1 sequence verification evidence SHALL be stored in a dedicated test
record. It SHALL record the exact checked-in sequence source, wrapper/probe
commands, build and revision provenance, hosted or target execution surface,
command and completion observations, final verdict, and explicit deferred
boundaries. Debugging lessons may be cited as diagnostic history but SHALL not
be the sole PASS authority.

#### Scenario: Hosted evidence records the full functional chain
- **WHEN** hosted Route 1 evidence is recorded
- **THEN** it SHALL record isolated runtime inputs, compile/upload/validate/run
  results, SoC and mode observations, fresh payload-completion evidence, and
  rejected stale or failure conditions
- **AND** it SHALL distinguish the hosted path from target filesystem and
  ground-observation claims.

#### Scenario: Target evidence records A/B/C and provenance
- **WHEN** target Route 1 evidence is recorded
- **THEN** it SHALL record A and B preflight results, C-owned functional
  evidence, local and remote provenance, target and ground observation
  sources, and A/B postflight results
- **AND** it SHALL state that C did not restart or stop shared baseline
  services.
