## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `command-auth-envelope-v1`.
- [x] 1.2 Validate the change artifacts with `openspec validate command-auth-envelope-v1` before runtime edits.

## 2. Runtime Authenticated Envelope

- [x] 2.1 Extend command envelope v1 with authenticated source, key-slot, and MAC fields while preserving the existing outer pseudo-opcode contract.
- [x] 2.2 Extend ingress/runtime config so each configured authority source carries deterministic auth config backed by repo-controlled runtime inputs.
- [x] 2.3 Add authenticated envelope parse and `HMAC-SHA256` verification owned by `CommandIngressAuthority` without creating a parallel runtime gate.
- [x] 2.4 Enforce runtime order `parse -> auth -> authority -> lifecycle -> sequence -> dispatch` for enveloped traffic.
- [x] 2.5 Keep rejection boundaries explicit so auth-fail, authority-denied, and lifecycle-denied paths do not mutate the wrong state layer.
- [x] 2.6 Preserve `SESSION_OPEN(seq0)` lifecycle semantics and strict-monotonic sequence enforcement after auth success.
- [x] 2.7 Keep legacy non-envelope compatibility explicit and outside authenticated privileges or session semantics.
- [x] 2.8 Add dedicated auth failure events, rejection reasons, and bounded telemetry.

## 3. Tests And Hosted Probe

- [x] 3.1 Extend helper tests for authenticated envelope parse/serialize, tamper detection, source mismatch, key-slot mismatch, and truncation.
- [x] 3.2 Extend classic `CommandIngressAuthority` component UTs for authenticated open, authenticated increasing sequence, bad MAC rejection, malformed/unknown-key rejection, authority-denied no-consume behavior, lifecycle-denied no-sequence-consume behavior, reboot/recreate reopen behavior, and `uhf-backup` continuity without authority expansion.
- [x] 3.3 Add `scripts/run_command_auth_envelope_probe.sh`.
- [x] 3.4 Prove hosted `sband-primary` acceptance for valid authenticated `SESSION_OPEN(seq0)` and increasing post-open command traffic.
- [x] 3.5 Prove hosted fail-closed auth rejection for invalid MAC, malformed auth envelope, and unknown-key/source-mismatch paths.
- [x] 3.6 Prove hosted `uhf-backup` authenticated read/status continuity while high-authority traffic remains authority denied and does not mutate lifecycle/sequence state.
- [x] 3.7 Prove hosted stale/mismatched session rejection and reboot-reopen behavior under the authenticated contract.

## 4. Evidence And Canonical Docs

- [x] 4.1 Add `docs/test-records/command-auth-envelope-v1/README.md`.
- [x] 4.2 Update the hosted command ingress authority / envelope / session verification-path entry in `docs/verification-path-registry.md`.
- [x] 4.3 Update `docs/roadmap/README.md` and the relevant roadmap note to mark this change complete and reassess the next recommended closure.
- [x] 4.4 Update `docs/architecture/current-development-architecture.md` if authenticated command ingress becomes active baseline truth.

## 5. Verification And Closeout

- [x] 5.1 Run fresh local generate/build and fresh UT generate/build with `fprime-venv` tools.
- [x] 5.2 Run focused command component tests and the repository-owned hosted auth probe after the fresh build.
- [x] 5.3 Run `bash scripts/run_verification_ci.sh <output-dir>`.
- [x] 5.4 Run `openspec validate command-auth-envelope-v1` and `openspec validate --specs`.
- [x] 5.5 Stop before mutating `openspec/specs/` if closeout proves main-spec sync is needed, and report the exact intended sync scope first.
