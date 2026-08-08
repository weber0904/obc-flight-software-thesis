## 1. Workflow Rule

- [x] 1.1 Add the formal `delivery-workflow` requirement for local rollback commits, first-push single-commit boundaries, and post-push focused fix commits.
- [x] 1.2 Align `obc-dev-spec/08_delivery_workflow.md` to the same three-phase rule and terminology.
- [x] 1.3 Align `AGENTS.md` and the closeout skill with the same rule without turning them into competing workflow specs.

## 2. Audit Trail

- [x] 2.1 Update the reconciliation matrix entry for `delivery-workflow-commit-guidance-followup-v1` to mark it as superseded/reverted.
- [x] 2.2 Add the reconciliation entry for this new accepted workflow-guidance change during closeout.

## 3. Verification And Closeout

- [x] 3.1 Run `python3 scripts/check_agent_entrypoint.py`.
- [x] 3.2 Run `python3 scripts/check_repo_consistency.py` and `openspec validate --specs`.
- [x] 3.3 Validate and archive the change.
