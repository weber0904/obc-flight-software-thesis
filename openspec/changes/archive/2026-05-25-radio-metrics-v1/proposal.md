## Why

The active COMM baseline already exposes several link, transport, and radio
status surfaces, but the repository does not yet freeze one reusable current
truth for which owner supplies each field, what units it uses, when it becomes
stale, and how unavailable values must be interpreted. This change closes that
observability gap before any later reliable-transfer or broader COMM policy
work widens the surface area.

## What Changes

- Freeze a bounded current-baseline radio/link observability contract for the
  active hosted and target/lab COMM paths.
- Extend the OBC-owned ground-link observation runtime contract so raw
  byte/backend owners publish counters, connection state, and status-observation
  progress without taking on mission policy.
- Add a cached radio observation contract owned by `RadioController`, including
  freshness and unavailable-result semantics for raw RSSI and the other current
  radio status fields.
- Update hosted runtime readback and reviewable telemetry so operators can see
  raw observation, derived health, and policy-facing COMM state separately.
- Update current docs, OpenSpec main specs, and reviewable evidence to record
  the frozen field set, ownership boundaries, and explicit non-claims.

## Capabilities

### Modified Capabilities

- `comm-subsystem`: formalize the current radio/link observability contract,
  extend raw observation/runtime readback surfaces, and freeze raw RSSI as a
  `RadioController` observation rather than a link-health input
- `interface-contract-index`: require the checked-in interface index to record
  the frozen COMM observability contract with owner, units, freshness, and
  unavailable semantics

## Impact

- Affected code: `GroundLinkDriver`, `GroundLinkHealthProvider`,
  `CommController`, `RadioController`, hosted runtime service/readback code, and
  related tests and probes
- Affected docs/specs: `openspec/specs/comm-subsystem/spec.md`,
  `openspec/specs/interface-contract-index/spec.md`,
  `docs/interfaces.md`, `docs/architecture/current-development-architecture.md`,
  `docs/roadmap/current-baseline.md`, `docs/roadmap/next-work.md`, and
  `evidence/records/radio-metrics-v1/README.md`
- Affected systems: hosted node-`5` S-band path, hosted node-`6` UHF path,
  hosted direct-TCP fallback path, and the service-managed target/lab default
  node-`5` path
