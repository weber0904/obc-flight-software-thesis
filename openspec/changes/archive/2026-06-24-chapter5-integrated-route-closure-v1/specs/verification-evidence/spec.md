## ADDED Requirements

### Requirement: Integrated Route Evidence May Be Staged

The verification evidence tree SHALL allow one integrated route verdict to be
constructed from multiple repo-owned staged scripts or from a repo-owned
command playbook when one monolithic probe would reduce determinism or make the
observability surface ambiguous.

#### Scenario: Staged route evidence records aggregation explicitly
- **WHEN** one Chapter 5 route is proven by multiple staged scripts
- **THEN** the evidence SHALL identify each contributing script, its exact path
  boundary, the artifacts it produced, and the aggregation rule that yields the
  route verdict

#### Scenario: Command-playbook fallback remains reviewable
- **WHEN** a Chapter 5 segment is first proven through a repo-owned stepwise
  command playbook instead of a fully automated probe
- **THEN** the evidence SHALL record the exact commands, expected observations,
  artifact paths, and final verdict
- **AND** it SHALL NOT leave the proof surface only in chat history

### Requirement: Chapter 5 Route Evidence Keeps Mission Console Packet-Lab Separate

Chapter 5 evidence SHALL reuse the maintained Mission Console baseline-attach
pattern only as an adjacent prerequisite and SHALL keep Mission Console
`packet-lab negative evidence` separate from Chapter 5 route verdicts unless a
later change proves a direct dependency.

#### Scenario: Chapter 5 evidence does not treat packet-lab oracle as prerequisite
- **WHEN** a Chapter 5 hosted or target route probe is reviewed
- **THEN** the evidence SHALL state whether Mission Console baseline attach is a
  reused prerequisite
- **AND** it SHALL keep Mission Console packet-lab negative evidence out of the
  route verdict boundary unless that route explicitly tests that oracle
