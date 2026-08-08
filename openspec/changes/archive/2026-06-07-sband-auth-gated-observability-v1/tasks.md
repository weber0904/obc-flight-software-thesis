## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `comm-subsystem`, `interface-contract-index`, `onboard-data-products-and-live-beacon`, and `verification-path-registry`.
- [x] 1.2 Validate the change artifacts before implementation with `openspec validate sband-auth-gated-observability-v1`.

## 2. COMM Observability Policy

- [x] 2.1 Extend `CommController` runtime state and policy handling so S-band live packet observability stays quiet until accepted S-band secure auth and closes on revoke, role invalidation, failover reconfiguration, or restart.
- [x] 2.2 Extend `CommEgressMux` with a reusable live-packet observability gate and S-band packet suppression/routing counters while preserving existing UHF quiet and file/downlink behavior.
- [x] 2.3 Update focused `CommController` and `CommEgressMux` unit coverage for pre-auth quiet, post-auth enable, session close, and UHF non-regression.

## 3. Current Docs And Specs

- [x] 3.1 Update `docs/interfaces.md` and `docs/architecture/current-development-architecture.md` to describe the current three-tier observability model and auth-gated node-`5` S-band live packet behavior.
- [x] 3.2 Update maintained hosted/target node-`5` operator runbooks so startup truth is "quiet until accepted secure auth, then live observability during the authenticated session."
- [x] 3.3 Keep explicit non-goals and UHF follow-up boundaries clear in the changed docs.

## 4. Probes And Evidence

- [x] 4.1 Add a repository-owned hosted node-`5` observability-governance probe that proves pre-auth quiet, post-auth live visibility, bounded `GET_*` summary readback, and session-close suppression.
- [x] 4.2 Add a service-managed target node-`5` observability-governance probe with the same governance boundary on the maintained target S-band path.
- [x] 4.3 Add a new `evidence/records/` evidence record and update verification-path documentation for the hosted and target node-`5` observability-governance proofs.

## 5. Validation And Closeout

- [x] 5.1 Run fresh focused UTs plus the new hosted and target probes.
- [x] 5.2 Rerun the target secure-auth control baseline and record the current hosted official sequencing/system-resources historical-wrapper status after `legacy-command-envelope-retirement-v1`.
- [x] 5.3 Run `openspec validate sband-auth-gated-observability-v1` and `openspec validate --specs`, then update this task list with the completed results.
