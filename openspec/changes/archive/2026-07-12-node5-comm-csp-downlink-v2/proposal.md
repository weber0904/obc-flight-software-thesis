## Why

The current target node-`5` COMM CSP downlink path keeps stock
`DpCatalog -> CommController -> FileDownlink` ownership, but it serializes each
downlink write through small synchronous request/reply chunks that wait for the
external link write to finish before acknowledging progress. That keeps
official `.fdp` delivery correct but makes large target file downlink
unnecessarily slow.

## What Changes

- Add a parallel node-`5`-only COMM CSP downlink `v2` protocol below stock
  `FileDownlink` without changing the official data-product owner chain.
- Keep `v1` `DOWNLINK_WRITE` available as the fallback path and preserve current
  behavior for node `6` and non-node-`5` COMM CSP targets.
- Change node-`5` downlink acceptance semantics from "external link write
  completed" to "bytes staged/committed in node-local memory".
- Add node-local staging, atomic commit-to-drain behavior, bounded timeout
  cleanup, and queued drain-to-external-link workers on node `5`.
- Extend hosted and governed target node-`5` proof surfaces plus unit tests to
  cover transport `v2`, fallback, byte accounting, and unchanged official
  payload/HK `.fdp` closure.

## Capabilities

### New Capabilities
- `node5-comm-csp-downlink-v2`: Parallel node-`5` COMM CSP transport services
  that stage and drain official file/downlink traffic without changing stock
  upper-layer ownership.

### Modified Capabilities
- `comm-subsystem`: node-`5` S-band COMM service exposure and official
  file/downlink transport behavior gain a bounded `v2` path and fallback rules.
- `verification-path-registry`: the maintained node-`5` proof family must keep
  the same path identity while recording the transport uplift and its bounded
  non-claims explicitly.

## Impact

- Affected code: `simulators/comm/CommCspProtocol`, `GroundLinkBackend`,
  `CommNodeServer`, hosted/target proof wrappers, and COMM unit tests.
- Affected systems: service-managed target node-`5` S-band path, hosted node-`5`
  proof path, COMM transport diagnostics, and file/downlink timing behavior.
- Public behavior: stock `FileDownlink` ownership remains unchanged; only the
  node-`5` transport layer gains earlier acceptance and queue-backed draining.
