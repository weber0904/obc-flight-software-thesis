## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `core-system-contracts`, `comm-subsystem`, `interface-contract-index`, `live-beacon-broadcast`, `verification-path-registry`, and `verification-evidence`.
- [x] 1.2 Validate the change artifacts with `openspec validate legacy-command-envelope-retirement-v1`.

## 2. Dependency Audit

- [x] 2.1 Inventory legacy command-envelope dependence across current docs, specs, scripts, registry entries, and maintained proof wrappers.
- [x] 2.2 Record a binary audit outcome for `source_id` / `key_slot` and remove them from the tracked current keystore/runtime path because the active secure baseline no longer depends on them.
- [x] 2.3 Classify legacy-touched surfaces as `delete`, `rewrite`, `migrate-if-shallow`, `remove-config`, or later follow-up surface.

## 3. Current Baseline Cleanup

- [x] 3.1 Rewrite current docs so auth-success secure baseline wording replaces legacy `SESSION_OPEN` as the preferred operator-facing command boundary.
- [x] 3.2 Demote legacy command proof families from current-baseline authority into historical compatibility evidence.
- [x] 3.3 Update current roadmap/operator handoff docs so they call out the remaining non-core follow-up surfaces instead of implying full retirement is already done.

## 4. Runtime / Config Scope Control

- [x] 4.1 Preserve repo-internal opened-session/activity/revoke behavior that current secure auth still uses.
- [x] 4.2 Remove `source_id` / `key_slot` from the tracked current keystore/runtime provisioning path while preserving repo-internal secure-session synthesis and leaving historical compatibility helpers explicitly non-current.

## 5. Verification

- [x] 5.1 Run focused repo searches proving current-baseline docs/specs no longer present legacy `SESSION_OPEN` as the preferred active model.
- [x] 5.2 Rebuild the affected native and focused UT targets after the secure-baseline keystore/helper retirement changes.
- [x] 5.3 Re-run focused component regression coverage:
  `OBC_Components_CommandIngressAuthority_ut_exe`,
  `OBC_Components_SecureLinkAuthorizer_ut_exe`,
  `OBC_Components_FileIngressAuthority_ut_exe`, and
  `OBC_Components_CommController_ut_exe`.
- [x] 5.4 Re-qualify and rerun the canonical hosted secure-auth proof wrapper
  `bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh`.
- [x] 5.5 Re-qualify and rerun the canonical hosted uplink-authority proof wrapper
  `bash scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh`.
- [x] 5.6 Refresh the target installed release after the tracked keystore
  contract change, then rerun the canonical target secure-auth preflight
  `bash scripts/run_target_secure_auth_command_path_probe.sh`.
- [x] 5.7 Rerun the canonical target secure-auth proof
  `bash scripts/run_target_secure_auth_proof.sh`.
- [x] 5.8 Run the fresh local verification gate
  `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`.
- [x] 5.9 Record the dependency audit and fresh verification evidence in
  `evidence/records/legacy-command-envelope-retirement-v1/README.md`.
- [x] 5.10 Re-run `openspec validate legacy-command-envelope-retirement-v1`
  and `openspec validate --specs` after the final evidence/doc updates.
