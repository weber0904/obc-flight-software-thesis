## 1. Change Artifacts

- [x] 1.1 Write the `interfaces-contract-v1` proposal, design, and
  `interface-contract-index` spec artifact.
- [x] 1.2 Define implementation tasks for the non-normative interface contract
  index and stale-doc cleanup scope.

## 2. Interface Contract Index

- [x] 2.1 Add `docs/interfaces.md` with source-priority, status-legend, and
  `TBD` summary sections.
- [x] 2.2 Populate the command envelope, ingress role, command admission, CCSDS
  framing, MTU, and rate-group timing sections using current repo truth only.
- [x] 2.3 Populate the node/service identity, state/history boundary,
  recovery/boot, and sequencing/SystemResources sections without reviving HK
  fallback as current baseline.

## 3. Narrative Cleanup

- [x] 3.1 Update `docs/architecture-review/current/` companion docs so 00/04
  are completed via `interfaces-contract-v1` and 06 is completed instead of
  active.
- [x] 3.2 Update `docs/roadmap/architecture-review-followups/` plan notes so
  00/04 become historical completed plans and 06 no longer claims an active
  formal change.
- [x] 3.3 Update current architecture and roadmap docs so they no longer use
  branch-local 01/03/06 wording or describe HK archive file/downlink as current
  baseline proof.

## 4. Validation And Closeout

- [x] 4.1 Run `python3 scripts/check_repo_consistency.py`.
- [x] 4.2 Run `openspec validate interfaces-contract-v1`.
- [x] 4.3 Run `openspec validate --specs`.
- [x] 4.4 Prepare the docs-only change for governed closeout on the dedicated
  branch.
