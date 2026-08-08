## Why

Route 1 has a reusable implementation for an official sequence-driven payload
flow, but its current evidence is embedded in older integrated-route material.
The repository needs a distinct, reviewable Route 1 path that proves the
current governed sequence contract on hosted and target surfaces without
overclaiming scheduler, payload-throughput, or RF closure.

## What Changes

- Define Route 1 as a bounded sequence-driven payload verification path:
  official sequence compilation, governed staging upload, admission validation,
  execution, SoC/mode admission, and fresh payload-completion evidence.
- Register the hosted and governed target variants as distinct evidence
  surfaces. The target variant uses the A -> B -> C ownership model, with the
  functional probe owning neither shared baseline repair nor shared-service
  shutdown.
- Add a Route 1-specific test record and update the registry/runbook so the
  manual example and automated probe use the same current command contract.
- State explicit non-claims: this change does not introduce a mission
  scheduler, persistent onboard schedule, generic payload throughput closure,
  RF closure, or a broader sequence-control plane.

## Capabilities

### New Capabilities

- `route1-sequence-verification`: The bounded hosted and governed-target Route
  1 sequence verification contract and its required evidence.

### Modified Capabilities

- `verification-path-registry`: Register Route 1 sequence verification as a
  reusable, distinct path rather than inferring it from integrated-route,
  payload-only, or generic sequence evidence.
- `verification-evidence`: Require Route 1 evidence to preserve the governed
  upload/admission/execution chain, fresh completion oracle, and hosted versus
  target provenance distinction.

## Impact

Affected surfaces are the Route 1 wrappers and target functional probe under
scripts/chapter5_routes/ and scripts/comm_verification/, the official Route 1
sequence example, target/manual operator guidance, the verification path
registry, and a dedicated test record. The active TopCcsds sequence and
payload owners are reused; no new runtime command authority or scheduler is
introduced.
