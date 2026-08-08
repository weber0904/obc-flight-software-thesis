## ADDED Requirements

### Requirement: Internal CSP Runtime Metrics Reads Stay Bounded Under Blocking Traffic

The active internal CSP runtime baseline SHALL let observability-only metrics
readers obtain a bounded runtime snapshot without waiting behind blocking
`ping` or request/reply traffic on the shared runtime instance.

#### Scenario: Metrics reads reuse cached state during blocking CSP traffic

- **WHEN** the maintained runtime is already inside a blocking CSP `ping` or
  `requestReply` operation on the active target node-`5` path
- **THEN** observability-only metrics readers SHALL return the latest cached
  runtime metrics snapshot instead of waiting for that blocking transaction
  critical section to complete
- **AND** the runtime SHALL preserve the existing serialization for the actual
  business-traffic operation that currently owns the runtime mutex

#### Scenario: Cached metrics refresh after runtime activity

- **WHEN** the runtime completes `init`, `ping`, `sendRaw`, `requestReply`, or
  `shutdown`
- **THEN** it SHALL refresh the cached metrics snapshot from the current runtime
  state before later observability-only readers reuse that snapshot
