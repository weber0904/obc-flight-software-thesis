# document-realignment-governance-v1 Evidence

## Scope

This record covers a documentation/governance change that:

- reduces `README.md` and `AGENTS.md` to lower-churn repo-root onboarding
  entrypoints
- aligns current-facing documentation metadata and snapshot boundaries with the
  current repository baseline
- adds a dedicated documentation-governance checker and wires it into the
  repository consistency and verification gate flow

This change does not alter runtime code, topology, commands, telemetry, events,
transport behavior, or validation-path claims.

## Truth Sources Consulted

- `README.md`
- `AGENTS.md`
- `docs/README.md`
- `docs/architecture.md`
- `docs/architecture.md`
- `docs/interfaces.md`
- `docs/roadmap/README.md`
- `docs/roadmap/current-baseline.md`
- `docs/roadmap/next-work.md`
- `docs/operator/hosted-official-sequencing-system-resources-runbook.md`
- `docs/operator/target-lab.md`
- `docs/reporting/README.md`
- `docs/architecture-review/current/README.md`
- `docs/architecture-review/current/follow-up-disposition.md`
- `docs/thesis.md`
- `evidence/verification-path-registry.md`
- `docs/verification.md`
- `docs/architecture.md`
- `docs/interfaces.md`
- `docs/verification.md`
- `openspec/specs/delivery-workflow/spec.md`
- `openspec/specs/planning-docs/spec.md`
- `openspec/specs/architecture-review/spec.md`
- `openspec/specs/project-reporting/spec.md`
- `openspec/specs/interface-contract-index/spec.md`
- `evidence/records/comm-doc-reconciliation-and-ci-wait-governance-v1/README.md`
- `evidence/records/planning-roadmap-and-snapshot-docs-v1/README.md`
- `evidence/records/maintenance-baseline-reconciliation-v1/README.md`
- `evidence/records/ci-docs-governance-fast-path-v1/README.md`

## Review Surfaces Updated

- `README.md`
- `AGENTS.md`
- `docs/README.md`
- `docs/architecture.md`
- `docs/architecture.md`
- `docs/architecture.md`
- `docs/operator/hosted-official-sequencing-system-resources-runbook.md`
- `docs/operator/target-lab.md`
- `docs/reporting/README.md`
- `docs/architecture-review/current/README.md`
- `docs/architecture-review/current/follow-up-disposition.md`
- `docs/thesis.md`
- `docs/thesis/CHANGELOG.md`
- `evidence/verification-path-registry.md`
- `docs/verification.md`
- `docs/architecture.md`
- `docs/interfaces.md`
- `docs/verification.md`
- `scripts/check_documentation_governance.py`
- `scripts/check_repo_consistency.py`
- `scripts/run_verification_ci.sh`
- `openspec/specs/documentation-governance/spec.md`
- `openspec/specs/delivery-workflow/spec.md`
- `docs/baseline-reconciliation-matrix.json`
- `docs/baseline-reconciliation-matrix.md`

## Verdict Boundary

Verdict: documentation/governance PASS.

This record proves documentation alignment and governance-checker integration
only. It does not prove:

- any new runtime behavior
- any new hosted or target validation path
- RF, reliable transfer, payload scheduling, or target timing closure
- any new secure-boot, bootloader, or hardware-integration claim

## Validation

| Step | Command | Result |
|---|---|---|
| change validation | `openspec validate document-realignment-governance-v1` | PASS before and after archive |
| main specs | `openspec validate --specs` | PASS |
| documentation governance | `python3 scripts/check_documentation_governance.py` | PASS |
| repository consistency | `python3 scripts/check_repo_consistency.py` | PASS |
| diff whitespace | `git diff --check` | PASS |
| full verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-document-realignment-governance-v1` | PASS |

## Notes

- `scripts/check_documentation_governance.py` intentionally checks only the
  current/snapshot entrypoints formalized by this change. It is not a generic
  Markdown linter for every historical file in the repository.
- The full verification gate now runs the documentation governance checker both
  directly and through repository consistency so failures remain visible in both
  focused and aggregate governance logs.
