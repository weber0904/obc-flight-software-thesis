## Context

The Route 1 record contained two formal campaign trees. The 2026-07-20 target
run is the evidence cited by the thesis; the earlier 2026-07-19 rerun is
superseded and not cited. Review follow-up found that the 7/20 run is not
authoritative: C installed and removed a shared OBC service drop-in, restarting
that service, and contemporaneous revision provenance is incomplete. Review
also confirmed that the historical 7/12 proof cannot supply current authority:
its remote workspace identity and build/install provenance artifacts were not
retained. Within the 2026-07-20 tree, capture-family expansion produced
byte-identical decoded JSON at several paths even though only one copy is
needed to reconstruct and review each decoded product.

The evidence freeze crosses stored artifacts, importer metadata, repository
checks, verification records, architecture status, and OpenSpec governance.
It must reduce redundant review surface without weakening transport
provenance, target verdicts, or the numeric facts used by Chapter 5.

## Goals / Non-Goals

**Goals:**

- Preserve the 2026-07-20 Chapter 5 functional observations without
  misclassifying them as governed A/B/C authority.
- Keep the 2026-07-12 proof as historical functional evidence, not current
  target authority, and leave target requalification pending.
- Remove the superseded 2026-07-19 campaign from the maintained tree.
- Replace only SHA-256-identical decoded JSON copies with a deterministic
  manifest pointing to one canonical `decode-primary` JSON per policy.
- Mechanically protect retained raw products, provenance, verdicts,
  thesis-critical values, and documentation links.
- Preserve and test attempt-aware importer metadata for failed/retried runs.

**Non-Goals:**

- Rerun hosted or target Route 1 campaigns.
- Change flight software, topology, sequence behavior, payload behavior, or
  target/lab A/B/C ownership.
- Deduplicate raw FDP/JPEG/BIN/capture/journal/channel/event artifacts or
  time-distinct snapshots.
- Change Chapter 5 text, expand RF/OTA claims, or include Mission Console work.

## Decisions

1. **Keep the 2026-07-20 campaign as a maintained functional observation, not
   Route 1 authority.** It is the campaign cited by Chapter 5 and retains the
   relevant functional outputs, but its own checkpoints prove that C restarted
   a shared service and its revision provenance is incomplete. The 2026-07-12
   record also lacks retained remote workspace identity and build/install
   provenance artifacts, so it remains historical functional evidence rather
   than current target authority. Target requalification stays pending. The
   removed 7/19 tree remains recoverable from the preserved review branch.

2. **Canonicalize by content identity, not filename or semantic similarity.**
   A duplicate is removable only when its SHA-256 equals the selected
   canonical JSON. This avoids interpreting arrays, metadata, or timestamps
   while reducing exact copies. Raw transport products and time-distinct
   snapshots are excluded even when their content happens to match.

3. **Keep one source-side `decode-primary` JSON per maintained policy.**
   AUTO and DETERMINISTIC canonical paths are stable products of the importer.
   Received-decode and `family-members` copies are recorded as removed
   equivalents. The manifest also records source and received FDP hashes and
   the importer reconstruction command so the deleted derivatives remain
   reproducible.

4. **Enforce the freeze with a repository-owned checker.** The checker treats
   the dedup manifest as data, validates canonical hashes and removed-path
   absence, checks retained artifacts including both secure-auth provenance
   files and the ownership-violation snapshot, requires the functional-only
   classification, protects thesis-critical facts, and resolves governed
   documentation links. Every file selected by the required artifact globs is
   also enumerated and hash-checked, so count checks cannot mask in-place
   changes to A/B snapshots, raw FDPs, journals, or summaries. Focused tests
   exercise both valid and intentionally corrupted bundles. The
   DETERMINISTIC entry must retain and hash its ground-received FDP because the
   observation claims GDS receipt; AUTO remains source-only and may record
   `null`. The byte-identical GDS-runtime and StandardPipeline `pipeline-store`
   received FDP files remain separate observations; the pipeline-stored copy is
   independently enumerated and hash-checked. The distinct prepare-stage
   native-CLI and pipeline raw receive captures plus the SoC-fallback CLI raw
   receive capture are also retained and hash-checked rather than being
   inferred from directional gateway capture coverage. The exact retained
   sequence source and compiled execution binary are likewise enumerated and
   hash-checked because they are direct inputs to the frozen observation and
   cannot be reconstructed from the current working-tree example. The manifest
   also enumerates exact
   non-authoritative historical status for 2026-07-12, non-authoritative
   2026-07-20 classification, and pending target-requalification assertions for
   every governing document; the checker rejects an authority promotion even
   when dates and links remain present. These assertions use explicit approved
   non-authority phrases; a character sequence such as `not` inside `notably`
   is not treated as negation, and retaining an approved sentence does not
   permit a contradictory positive authority statement elsewhere in the same
   governing document.

5. **Prevent recurrence without widening the shared helper change.** Both
   Route 1 C-stage wrappers explicitly remove the historical max-data and
   sibling ground-link timeout override inputs before launching their
   scenarios. This preserves A ownership for aggregate and direct stage
   invocation without altering other target probe helpers or deployed runtime
   behavior. The existing manual-auth profile remains required, but both
   stages request it through A's baseline option before B rather than invoking
   a shared-service restart between A/B and C. Both C entrypoints pin the four
   service-profile controls to the governed node-`5` S-band values so caller
   environment cannot reintroduce a profile drop-in or restart. The shared
   helper additionally treats all six requested profile fields as
   verification-only when `TARGET_BASELINE_MANAGED_EXTERNALLY=1`: a mismatch
   fails before any override or restart, while legacy probe-owned baseline
   behavior remains unchanged.

6. **Record the local verification exception explicitly.** This evidence
   cleanup uses focused importer/checker, syntax, governance, consistency,
   sensitive-data, and OpenSpec checks. It does not claim a fresh operational
   result. Neither historical target observation is promoted to operational
   authority, target requalification remains pending, and the hosted PR
   `baseline-gate` remains mandatory.

7. **Gate future target authority on revision identity.** The formal-rerun
   wrapper records an explicit workspace marker after its governed sync/build
   because the target workspace intentionally excludes `.git`. Before target
   C, a repository-owned checker compares local branch/head/version, that
   marker, any remote Git head when present, remote build metadata, installed
   manifest/version metadata, the resolved release pointer, and installed OBC
   hash. The checker also rejects untracked paths unless they are below
   explicitly local-only roots that the same formal runner excludes from
   target sync. SSH/read failures, mismatches, or unreviewed sync inputs block
   C and authoritative PASS promotion, including under `SKIP_DEPLOY` and
   resume. Hosted evidence remains independently runnable and is not demoted
   by missing target provenance.

   The ordinary fresh A -> B readiness preflight remains before functional
   work. After hosted completion, every target C attempt, including an
   automatic retry and a pending `SKIP_DEPLOY` or resumed attempt, revalidates
   campaign identity, requests the existing A-layer forced OBC restart, and
   generates fresh target provenance. This closes the gap between the resolved
   `current/bin/OBC` and an already-running process without giving service
   lifecycle ownership to C or letting an external release change between
   attempts reuse the preceding attempt's provenance. Attempt-specific log
   labels preserve each gate observation, and each target attempt writes a
   distinct provenance JSON whose path and SHA-256 are bound on that attempt
   record. The campaign-level singleton remains only the latest-provenance
   compatibility view; it does not replace earlier attempt artifacts.

   A resume that already retains an authoritative target PASS does not execute
   target C and therefore does not requalify the current install. Its recorded
   campaign and target-provenance objects are revalidated as a pair, including
   the attempt-specific provenance path and SHA-256 recorded directly on the
   target attempt, the stored OBC hashes, and release/build metadata, then
   preserved unchanged. A missing, corrupt, mismatched, or non-authoritative
   retained binding fails closed instead of allowing current provenance to
   relabel the historical attempt. Target provenance is checked, retained, and
   hashed before importer writes so a missing binding cannot leave a partially
   imported attempt.

8. **Keep ignored bytes and cross-revision attempts out of formal authority.**
   The normal developer sync retains its existing behavior, while the Route 1
   formal runner requests a Git-index-only archive including initialized
   submodules. This is stronger than applying `.gitignore` patterns to tar and
   avoids accidentally excluding tracked files. Formal extraction stages a
   replacement workspace and keeps the previous workspace as a bounded sibling
   backup, preventing stale non-indexed files from surviving an overlay. On a
   fresh deployment, the existing recursive submodule initialization runs
   before the first campaign identity record so a clean clone can reach this
   committed-tree check; `SKIP_DEPLOY` and resume retain their prior behavior. A
   version-3 workspace marker retains the exact sorted Git-indexed path list
   and a deterministic SHA-256 over each path, Git-normalized mode, and content.
   Before that content can be synchronized or marked, the checker compares its
   bytes and normalized modes with committed superproject and recursive
   submodule index entries, verifies those indexes and submodule revisions
   against their committed trees, and therefore fails hidden
   assume-unchanged, skip-worktree, staged-index, or `core.filemode=false`
   drift even when porcelain status is clean.
   The target gate recomputes that digest from the live remote workspace, so a
   marker's self-reported head/version cannot hide a changed, deleted, omitted,
   duplicated, or unsafe path. The marker also records a package-path-keyed
   SHA-256 map for all eight remote-build inputs copied into the target bundle:
   six executables, the topology dictionary, and build version metadata.
   Changing any governed build output before a later package/install therefore
   fails even when the downstream manifest and installed hashes agree with
   each other. The marker JSON is streamed over SSH stdin rather than placed in
   argv because the complete indexed path list is intentionally large.

   A campaign source identity records branch/head/project version before the
   first attempt; resume checks it before loading retained attempts, every
   invocation checks it again after hosted execution, and every target attempt
   checks it before target restart/provenance. The manifest exposes the same
   identity. This source-identity check does not substitute for the retained
   binary-provenance binding described above.

9. **Keep attempt layout inside the selected evidence surface.** The importer
   accepts an empty label or one alphanumeric-led component containing only
   letters, digits, dot, underscore, or hyphen. Absolute, traversal, and
   multi-component labels fail before destination directories are created;
   retry metadata remains otherwise unchanged.

10. **Keep hosted build freshness independent of target deployment.**
    `SKIP_DEPLOY` and resume control target synchronization, packaging, and
    installation; they do not authorize reuse of an arbitrary native build for
    a new hosted verdict. Every hosted attempt that actually executes,
    including automatic retry and pending resume, first revalidates campaign
    identity and builds OBC plus every executable used by the hosted wrapper
    with an attempt-specific log. A retained authoritative hosted PASS still
    skips execution, and target A/B/C ownership remains unchanged.

11. **Bind every installed release input before target C.** A fresh formal
    install copies the locally packaged manifest into campaign evidence;
    `SKIP_DEPLOY=1` requires an explicit local package/install manifest and
    resume retains the copied receipt. The remote provenance capture retains
    the live installed-manifest SHA-256 and recomputes size plus SHA-256 for
    every manifest-listed release file from the resolved `current` release.
    The evaluator compares the live manifest hash and content with the local
    receipt before accepting the live file audit, and rejects unsafe paths,
    non-regular or unreadable entries, file-set differences, and size/hash
    drift. This closes coordinated launcher-plus-manifest mutation without
    changing A/B/C ownership or the functional C scenario.

12. **Treat retained-attempt bytes as part of resume authority.** Importer
    schema 2 enumerates every copied file with its size and SHA-256, while the
    campaign attempt records the importer-manifest SHA-256. Both the resume
    skip path and manifest authority calculation revalidate manifest metadata,
    artifact roots, the exact retained-file set, and every recorded byte.
    Legacy or damaged attempts without that binding fail closed instead of
    being promoted from a copied `PASS` string.

13. **Evaluate dated authority language clause by clause.** Semicolon, colon,
    and coordinated conjunction clauses inherit the dated subject for
    relevance but must carry their own valid negation. Explicit double
    negation is rejected before the normal non-authority allowlist, preventing
    a positive clause from borrowing `non-authoritative` text elsewhere in the
    sentence.

14. **Bind the active systemd launch chain before C.** Fresh deployment
    retains the exact rendered OBC service unit that the autostart installer
    writes. `SKIP_DEPLOY=1` requires the corresponding trusted receipt and
    resume preserves the campaign copy. After A's forced restart, provenance
    compares the active fragment to that receipt, requires only the exact
    A-owned CAN-FD and Beacon drop-ins, validates the effective launch path,
    and audits the service process tree against the installed release. Route 1
    clears caller manual timeout knobs before invoking A so an external
    timeout drop-in cannot be introduced between deployment and this gate.

15. **Treat pipeline observations as frozen evidence.** The retained
    StandardPipeline channel and event logs are distinct observations, so the
    dedup manifest enumerates and SHA-256-checks both alongside the pipeline
    raw receive bytes.

16. **Use one OBC service identity end to end.** The formal runner exports the
    canonical `obc-comm-csp-stack.service` name and rejects a conflicting
    caller value before A. Both Route 1 target entrypoints independently
    reject a noncanonical value, keeping A, C, the rendered-unit installer,
    and provenance capture on the same process identity.

17. **Split remaining contrast clauses.** Authority checking treats `while`,
    `although`, and `though` as clause boundaries and recognizes
    `remain(s/ed)` or `continue(s/d)` as positive authority predicates. A later
    positive assertion therefore cannot borrow an earlier non-authority
    phrase.

18. **Bind every packaged remote-build input.** The workspace marker records
    the complete package-path-keyed hash set copied from remote build trees.
    Before target C, provenance recomputes those remote hashes and requires
    each installed-manifest package entry to match its marker-time value.
    A focused static test keeps that governed set synchronized with the
    remote-build copies in `scripts/package_rpi_bundle.sh`; the active service
    process audit also binds both OBC and its managed radio helper to the
    installed manifest.

19. **Bind resume to retained campaign history.** Before loading attempts,
    resume compares the identity file with both the current checkout and the
    branch/head/version already stored in the existing campaign manifest.
    This prevents a replacement identity file from relabeling retained
    attempts without adding a new campaign-identity subsystem.

20. **Normalize only the governed Route 1 dates.** Authority checking maps
    common numeric, English month-name, and Chinese spellings of 2026-07-12
    and 2026-07-20 to their canonical subjects. It does not scan unrelated
    current evidence authority claims or attempt general natural-language
    interpretation.

## Risks / Trade-offs

- **A removed path is still referenced** → scan the repository and have the
  checker validate governed record links before closeout.
- **A duplicate is removed without exact identity** → require recorded SHA-256
  equality and test every canonical/removed mapping.
- **Reviewers need the historical 2026-07-19 tree** → retain the existing
  review remote and document its recovery point.
- **The evidence freeze is mistaken for a new target qualification** → state
  the no-rerun exception and classify the dated 2026-07-20 result as a
  functional observation with explicit limitations.
- **Generated reconciliation drifts after archive** → regenerate JSON/Markdown
  after sync/archive and rerun repository consistency.

## Migration Plan

1. Remove the exact 2026-07-19 campaign directory.
2. Hash and remove only the enumerated duplicate 2026-07-20 JSON files.
3. Add the manifest, checker, tests, and documentation reconciliation.
4. Run focused validation, sync main specs, archive the OpenSpec change, and
   regenerate reconciliation records.
5. Apply review follow-up: protect provenance files, record the 7/20
   limitations, restore 7/12 authority, prevent both Route 1 C stages from
   forwarding shared-service overrides, move manual-auth profile ownership to
   A, pin C's service-profile expectations, hash every required retained
   artifact, require the claimed DETERMINISTIC ground-received FDP, exclude all
   non-indexed formal target sync inputs, hash all three distinct ground
   receive captures, confine attempt labels, bind resume to one campaign
   revision, and
   enforce revision/sync-input provenance, A-owned post-install reload, and
   externally managed profile verification before future target
   execution/PASS promotion.
6. Commit a local-ready branch. Push and PR creation require separate approval.

Rollback is available by restoring the removed paths from review commit
`ea020861`; no external service or deployed target state is changed.

## Open Questions

None.
