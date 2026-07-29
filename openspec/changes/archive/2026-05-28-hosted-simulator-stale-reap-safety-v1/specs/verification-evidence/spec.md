## MODIFIED Requirements

### Requirement: Hosted Per-Band Stock-Stack Operator Baseline Evidence Is Reviewable

The verification evidence tree SHALL record reviewable hosted-first evidence
for the maintained per-band stock-stack operator baseline as distinct S-band
and UHF stock ground surfaces with launcher-owned hosted runtime roots, where
combined mode exposes one shared hosted runtime for both surfaces.

#### Scenario: Evidence records distinct operator surfaces and launcher-owned runtime
- **WHEN** the repository records the maintained hosted operator-baseline proof
- **THEN** the evidence SHALL identify the exact launcher or proof commands,
  the launcher-owned hosted runtime root, per-stack GDS and TTS ports,
  southbound endpoints, file-storage directories, process logs, startup order,
  and final verdict
- **AND** it SHALL state when combined mode uses one shared hosted runtime for
  both exposed stock surfaces
- **AND** it SHALL show that both stock stacks start successfully while
  remaining reviewably distinct

#### Scenario: Evidence records bounded non-interference with unrelated active simulators
- **WHEN** the repository updates or reruns the maintained hosted
  operator-baseline proof after cleanup hardening
- **THEN** the evidence SHALL show that launcher startup and teardown do not
  terminate unrelated active EPS/ADCS simulator runs that are using different
  CSP hub ports
- **AND** it SHALL keep that claim bounded to hosted launcher cleanup ownership
  rather than broad system-wide orphan-process prevention

#### Scenario: Evidence states what is reused versus newly proven
- **WHEN** the same hosted operator-baseline proof depends on existing S-band
  or UHF transport and policy paths
- **THEN** the evidence SHALL identify which adjacent governed paths are reused
  prerequisites and which maintained operator-baseline behavior is newly proven
- **AND** it SHALL avoid restating the proof as new simultaneous runtime
  arbitration, new UHF policy semantics, or new target-bearing transport proof

#### Scenario: Evidence preserves hosted-only scope and explicit non-claims
- **WHEN** the hosted operator-baseline evidence is finalized
- **THEN** it SHALL state that the proof is hosted-only
- **AND** it SHALL keep explicit non-claims for simultaneous dual-link runtime
  arbitration, one-GDS multi-upstream operator behavior, one-gateway
  multiplexer behavior, target/lab simultaneous closure, and RF closure
