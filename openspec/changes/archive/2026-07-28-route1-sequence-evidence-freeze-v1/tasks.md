## 1. Governed Evidence Cleanup

- [x] 1.1 Remove the superseded 2026-07-19 Route 1 campaign tree.
- [x] 1.2 Retain the complete 2026-07-20 raw/provenance/verdict evidence and remove only enumerated SHA-256-identical decoded JSON copies.
- [x] 1.3 Add a deduplication manifest with canonical paths, hashes, removed equivalents, raw FDP hashes, and reconstruction commands.

## 2. Evidence Tooling

- [x] 2.1 Preserve and test attempt-aware importer metadata for attempt label, retry lineage, and failure class.
- [x] 2.2 Add a repository-owned Route 1 evidence checker for manifest integrity, retained artifacts, thesis-critical values, and governed links.
- [x] 2.3 Add focused positive and corruption-path tests for the evidence checker.

## 3. Documentation Reconciliation

- [x] 3.1 Update the Route 1 and payload evidence records to retain the 2026-07-20 thesis observations, classify the provenance-incomplete 2026-07-12 proof as historical functional evidence, and leave target requalification pending.
- [x] 3.2 Reconcile architecture status and registry entry 75 evidence guidance without changing its validation boundary.
- [x] 3.3 Remove maintained-document references to the deleted 2026-07-19 campaign and record the local verification exception.

## 4. Focused Verification And Archive

- [x] 4.1 Run importer/checker tests, shell syntax checks, sensitive-information scanning, `git diff --check`, documentation governance, and repository consistency.
- [x] 4.2 Validate the OpenSpec change and main specs, then verify implementation against proposal, design, specs, and tasks.
- [x] 4.3 Sync and archive the OpenSpec change, regenerate reconciliation JSON/Markdown, and rerun consistency checks.
- [x] 4.4 Confirm the final diff excludes Mission Console, `output/`, and `.codex_thesis_work/`, and prepare a clean local-ready commit.

## 5. Review Follow-up

- [x] 5.1 Classify the 2026-07-20 bundle as a functional observation because C restarted the shared OBC service and contemporaneous revision provenance is incomplete.
- [x] 5.2 Prevent the Route 1 C wrapper from forwarding the historical 240-byte shared-service override.
- [x] 5.3 Hash-lock both secure-auth provenance artifacts and the shared-service override snapshot, with focused deletion and corruption tests.
- [x] 5.4 Reconcile the main spec, registry, architecture, evidence records, and generated reconciliation entry without rerunning target or hosted campaigns.
- [x] 5.5 Sanitize inherited shared-service override inputs at both Route 1 C-stage entrypoints and test direct-stage coverage.
- [x] 5.6 Gate formal target execution and authoritative PASS promotion on matching local, remote-workspace, build, installed-release, and binary provenance.
- [x] 5.7 Move the required manual-auth service profile into A-layer preflight ownership for both Route 1 target stages.
- [x] 5.8 Reject untracked target-sync inputs while allowing only explicitly sync-excluded local evidence/output roots.
- [x] 5.9 Enumerate and SHA-256-check every frozen artifact selected by required globs, with baseline and FDP corruption tests.
- [x] 5.10 Pin both Route 1 C-stage service-profile controls to the governed node-5 S-band baseline.
- [x] 5.11 Restrict formal target synchronization to Git-indexed superproject/submodule files.
- [x] 5.12 Bind campaign resume to the branch/head/project-version identity recorded before the first attempt.
- [x] 5.13 Require and hash the DETERMINISTIC ground-received FDP, with a null-plus-deletion corruption test.
- [x] 5.14 Hash-lock both distinct prepare-stage raw ground receive captures with corruption tests.
- [x] 5.15 Reject unsafe attempt labels before any importer destination write.
- [x] 5.16 Retain fresh A -> B readiness, then request one A-owned forced OBC restart after hosted completion and immediately before target provenance for every fresh, `SKIP_DEPLOY`, or resumed invocation that may execute target C.
- [x] 5.17 Make externally managed C profile handling verification-only and retain probe-owned legacy behavior.
- [x] 5.18 Hash-lock the SoC-fallback CLI raw receive capture with mutation and deletion tests.
- [x] 5.19 Hash-lock the retained sequence source and compiled execution binary with mutation and deletion tests.
- [x] 5.20 Require explicit non-authoritative 2026-07-12/2026-07-20 classifications and pending target requalification in every governing document, with authority-promotion corruption tests.
- [x] 5.21 Bind each target attempt to its provenance-file SHA-256, preserve and revalidate that provenance for an authoritative PASS on resume, and reject invalid authority instead of rebinding the old observation to the current binary.
- [x] 5.22 Remove unsupported current-authority claims from the 2026-07-12 historical proof after confirming its remote workspace/build/install provenance artifacts are not retained.
- [x] 5.23 Revalidate the recorded campaign branch/head/version identity after hosted execution and before target restart/provenance on every invocation.
- [x] 5.24 Independently hash-lock the StandardPipeline `pipeline-store` received FDP and reject mutation, deletion, or manifest omission.
- [x] 5.25 Restore the SoC-fallback scenario's testcase-owned EPS simulator state to the Route 1 80% baseline on success and failure without taking over shared service lifecycle.
- [x] 5.26 Hash-lock the frozen campaign README and reject authority-promoting README edits even when required non-authoritative substrings remain.
- [x] 5.27 Route every automatic target retry through campaign identity validation, an A-owned forced OBC restart, and regenerated target revision provenance before C.
- [x] 5.28 Revalidate campaign identity and run a fresh native OBC build before every executed hosted attempt, independent of target deploy, skip, and resume controls.
- [x] 5.29 Bind the marker to a deterministic manifest/digest of every synchronized Git-indexed path and the marker-time remote-build input hashes, rejecting remote source or build drift under skip and resume.
- [x] 5.30 Compare every serialized tracked byte and Git-normalized mode with the committed superproject/submodule trees, rejecting hidden index-flag, staged-index, and file-mode drift.
- [x] 5.31 Require explicit approved non-authority phrases and reject coordinated governing-document/manifest rewrites that only contain `not` inside an authority-promoting word.
- [x] 5.32 Initialize recursive submodules before first identity capture on fresh deployment runs while preserving `SKIP_DEPLOY` and resume behavior.
- [x] 5.33 Reject contradictory governing-document authority statements even when the approved non-authority assertion remains present.
- [x] 5.34 Bind target provenance to the installed manifest and live size/SHA-256 audit of every manifest-listed release file before C.
- [x] 5.35 Bind each formal attempt to its importer-manifest SHA-256 and exact retained artifact file set, then fail resume and campaign authority on missing or corrupt bytes.
- [x] 5.36 Compare the live installed manifest with a local trusted package/install receipt so coordinated release-file and manifest edits fail closed.
- [x] 5.37 Evaluate authority language per clause and reject double-negated non-authority promotions.
- [x] 5.38 Rebuild OBC and every independently declared executable consumed by the hosted Route 1 wrapper before each executed attempt.
- [x] 5.39 Evaluate coordinated authority clauses independently so a positive clause cannot borrow a preceding negation.
- [x] 5.40 Retain and validate a distinct target revision-provenance path and SHA-256 for every failed or successful target attempt.
- [x] 5.41 Bind the active OBC systemd unit, exact A-owned drop-ins, and service process tree to a trusted rendered-unit receipt before target C.
- [x] 5.42 Clear caller-provided manual timeout controls before Route 1 requests A's manual-auth preflight.
- [x] 5.43 Hash-lock the retained StandardPipeline channel and event observations.
- [x] 5.44 Require the formal runner, A, C, and provenance checker to use the same canonical OBC systemd service identity.
- [x] 5.45 Reject `while`/`although` authority contradictions and positive `remains authoritative` predicates.
- [x] 5.46 Bind every remote-build input copied into the target bundle from marker time through live remote capture and the installed manifest, including the service-launched radio helper.
- [x] 5.47 Require a resumed campaign identity file to match the identity already recorded in the retained manifest before loading attempts.
- [x] 5.48 Apply dated authority checks to common numeric, English month-name, and Chinese spellings of the governed 2026-07-12 and 2026-07-20 observations.
