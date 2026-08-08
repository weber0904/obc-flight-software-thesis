## 1. OpenSpec Artifacts

- [x] 1.1 Create proposal, design, core-system-contracts delta spec, verification-evidence delta spec, and tasks for `command-envelope-metadata-v1`.
- [x] 1.2 Validate with `openspec validate command-envelope-metadata-v1`.

## 2. Envelope Contract And Runtime Behavior

- [x] 2.1 Add command envelope v1 parser/serializer helper and direct tests.
- [x] 2.2 Add envelope constants, observed/rejected events, and envelope telemetry to `CommandIngressAuthority`.
- [x] 2.3 Preserve legacy command authority behavior for non-envelope commands.
- [x] 2.4 Parse valid envelopes before authority evaluation and forward only the inner command while preserving context.
- [x] 2.5 Reject malformed envelope candidates before `CmdDispatcher` with exactly one synthetic `FORMAT_ERROR` response.
- [x] 2.6 Ensure sequence metadata is observed but not enforced in v1.

## 3. Tests And Hosted Probe

- [x] 3.1 Extend classic component UTs for valid envelope unwrap, metadata observation, malformed rejection, UHF-backup denial, and context preservation.
- [x] 3.2 Add a repo-owned envelope injector for hosted probe use.
- [x] 3.3 Add `scripts/run_command_envelope_metadata_probe.sh`.
- [x] 3.4 Keep the existing command ingress authority probe passing.

## 4. Evidence And Verification

- [x] 4.1 Run affected helper/component tests and catalog check.
- [x] 4.2 Run focused hosted command authority and envelope probes after a fresh build.
- [x] 4.3 Add `evidence/records/command-envelope-metadata-v1/README.md`.
- [x] 4.4 Update `evidence/verification-path-registry.md` for the envelope metadata evidence boundary.
- [x] 4.5 Run `openspec validate command-envelope-metadata-v1`, `openspec validate --specs`, and `python3 scripts/check_repo_consistency.py`.
