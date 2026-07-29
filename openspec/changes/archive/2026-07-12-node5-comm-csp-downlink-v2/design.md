## Context

The current target node-`5` S-band baseline uses stock F' file/downlink
ownership and then hands serialized file buffers to a repo-local COMM CSP
backend. That backend currently chunks outgoing bytes into synchronous
`DOWNLINK_WRITE` request/reply transactions and waits for node-local external
link writes before returning success. The repository already has async CSP
runtime wrappers, but those wrappers still serialize requests one at a time and
therefore do not solve the transport bottleneck by themselves.

## Goals / Non-Goals

**Goals:**
- Keep `DpCatalog -> CommController -> FileDownlink -> CommEgressMux`
  unchanged.
- Add a node-`5`-only parallel `v2` COMM CSP transport with bounded in-memory
  staging and committed drain queues.
- Make `GroundLinkDriver.send()` complete when a full buffer has been accepted
  into node-`5` memory, not when the external link finishes writing it.
- Preserve `v1` fallback and current behavior for node `6`, reliable transfer,
  and non-node-`5` paths.

**Non-Goals:**
- No `FileDownlink` rewrite.
- No payload-family reliable-transfer widening.
- No node-`6` or generic all-node COMM CSP migration in this slice.
- No end-to-end reliable delivery guarantee after bytes are committed into the
  node-`5` drain queue.

## Decisions

### Parallel wire protocol instead of in-place replacement

Keep `v1` services intact and add explicit `v2` service IDs for node `5`. This
lets hosted and target node-`5` stacks adopt the uplift without forcing node
`6`, generic node `4`, or stale installs to understand the new wire shape.

### Stage -> commit -> drain instead of enqueue-per-chunk

`GroundLinkDriver.send()` still represents one atomic upper-layer buffer send.
To preserve that contract, node `5` stages chunks for one stream privately and
commits them to the shared drain queue only when the `LAST` chunk is accepted.
This avoids half-committed file buffers and duplicate byte insertion on sender
retry.

### In-memory acceptance boundary

Accepted `v2` traffic stops at node-local memory. That provides the desired
throughput uplift while keeping RAM usage bounded and avoiding disk I/O in the
hot path. The tradeoff is that committed-but-not-yet-flushed bytes remain
best-effort if the external link fails.

### Capability probe only on node 5

The OBC-side COMM CSP backend probes `v2` support only when targeting node `5`.
If the status probe fails, the backend emits one bounded fallback diagnostic and
continues on `v1`.

### Do not build on the existing async CSP owner

The existing async CSP owner is a single-worker serialization helper, not a
multi-inflight transport. The transport uplift therefore lives in the
node-`5`/backend protocol and queue logic, not in another layer of async CSP
wrapping.

## Risks / Trade-offs

- [Committed queue loss on external-link failure] -> Purge queues, account for
  dropped bytes explicitly, and keep retry ownership at the existing stock
  upper-layer file/downlink path.
- [Sender retry could duplicate bytes] -> Cache duplicate `(streamId, seq)`
  replies and make only final-chunk commit visible to the shared drain queue.
- [Stale staged streams could leak memory/credit] -> Apply a fixed `2000 ms`
  staging timeout plus explicit sender abort on mid-stream failure.
- [v2 rollout could silently widen other paths] -> Gate probe/use on node `5`
  only and retain `v1` unchanged elsewhere.

## Migration Plan

1. Add protocol/service definitions, node-`5` staging/drain queues, and backend
   `v2` probe/send behavior behind node-`5` targeting only.
2. Add/refresh unit tests for protocol probe, staging, commit retry, duplicate
   replay, abort, and purge accounting.
3. Update hosted proof tooling and docs to observe accepted-versus-flushed byte
   separation on the maintained node-`5` path.
4. Run governed target node-`5` proof on the existing baseline and keep `v1`
   fallback available for rollback.

## Open Questions

- None. Scope, compatibility strategy, and acceptance boundary are fixed for
  this slice.
