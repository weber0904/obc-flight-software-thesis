# Project Documentation

Status: canonical public documentation index.  
Last reconciled: 2026-07-29 for `thesis-submission-v1`.

## Current Truth

Read in this order:

1. [Current Architecture](architecture/current-development-architecture.md)
2. [Interface Contract Index](interfaces.md)
3. [Verification Matrix](verification-matrix.md)
4. [Verification Path Registry](verification-path-registry.md)
5. [Operator Runbooks](operator/)

Source priority is:

1. code, FPP topology, runtime wiring, and package/launch behavior
2. current test evidence and verification-path registry
3. `openspec/specs/`
4. current narrative documents
5. archived OpenSpec and historical test records for development context

## Document Families

- `architecture/`: current system, contribution boundary, and planned end state
- `operator/`: maintained hosted, target/lab, Mission Console, and thesis demo
  procedures
- `roadmap/`: release status and bounded future work
- `thesis/`: claim/evidence and source map, not thesis body text
- `test-records/`: preserved reviewable evidence summaries
- `evidence/`: public evidence catalog and release-asset policy

Point-in-time reporting packages, stale architecture-review packages,
implementation handoffs, agent-specific instructions, and thesis prose drafts
are intentionally absent from this public distribution. Their formal decisions
remain traceable through OpenSpec and test records.
