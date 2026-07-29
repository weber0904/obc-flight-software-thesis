## Why

The Route 1 formal evidence history retained a superseded 2026-07-19 campaign
alongside the thesis-backed 2026-07-20 functional results. The 2026-07-20
bundle also contained several byte-identical decoded JSON copies, which
obscured the canonical artifacts and made review unnecessarily large without
adding provenance or verdict value. Review follow-up established that the
7/20 run cannot be authoritative A/B/C evidence because C restarted the shared
OBC service and contemporaneous revision provenance is incomplete.

## What Changes

- Remove the superseded 2026-07-19 Route 1 formal-rerun artifact tree while
  preserving its recoverability from the existing review remote.
- Freeze the 2026-07-20 target run as a thesis-backed functional observation,
  classify the provenance-incomplete 2026-07-12 proof as historical functional
  evidence, and leave current target requalification pending without changing
  the 7/20 timestamps, product sizes, sequence indices, state-transition
  results, raw transport bytes, or functional verdict oracles.
- Canonicalize only SHA-256-identical decoded JSON derivatives through a
  checked deduplication manifest that records canonical paths, removed
  equivalents, raw FDP hashes, and reconstruction commands.
- Add repository-owned evidence checks for the manifest, retained artifacts,
  both secure-auth provenance files, governance classification,
  thesis-critical values, documentation links, and SHA-256 integrity for every
  artifact selected by the required retained-file globs.
- Require every governing document to retain explicit non-authoritative
  classifications for the 2026-07-12 historical proof and 2026-07-20
  functional observation, plus pending target requalification, rather than
  accepting date, link, or ambiguous `not` substring presence alone; reject a
  contradictory positive authority statement even when the approved sentence
  remains, including when the same governed dates use common numeric,
  month-name, or Chinese spellings.
- Treat `while`/`although` contrast clauses and `remains authoritative`
  predicates as independent positive authority assertions rather than letting
  them borrow a prior negation.
- Require the DETERMINISTIC canonicalization to retain and hash its
  ground-received FDP while preserving AUTO's source-only `null` boundary.
- Independently hash-lock the StandardPipeline `pipeline-store` received FDP
  even when it is byte-identical to the GDS-runtime product, because the files
  represent distinct ground consumers.
- Hash-lock the distinct prepare-stage native-CLI and pipeline raw ground
  receive captures plus the SoC-fallback CLI raw receive capture.
- Hash-lock the prepare-stage StandardPipeline channel and event observations
  so their retained presence is part of the frozen evidence oracle.
- Hash-lock the exact retained sequence source and compiled execution binary
  used by the frozen 2026-07-20 observation.
- Prevent either Route 1 C stage from forwarding the historical 240-byte or
  sibling shared-service overrides; A remains the only shared baseline owner.
- Delegate the required manual-auth shared-service profile to the existing
  A-layer baseline option rather than mutating it between A/B and C.
- Clear caller-provided manual timeout values and timeout-drop-in naming before
  requesting that A-layer preflight, preventing external inputs from changing
  the formal service profile.
- Pin both C-stage service-profile expectations to the governed node-`5`
  S-band values, and make externally managed profile handling
  verification-only, so inherited controls or baseline drift cannot create a
  C-owned restart.
- Retain the ordinary fresh A -> B readiness preflight, then request the
  existing A-layer forced OBC restart after hosted completion and immediately
  before target provenance for every fresh, `SKIP_DEPLOY`, or resumed formal
  invocation that may execute target C, so the running service cannot remain
  on the release that preceded a runner-owned, external, or between-attempt
  `current` symlink swap.
- Preserve and revalidate the original target revision provenance when resume
  retains an authoritative target PASS, including its attempt-recorded
  provenance path and file SHA-256; do not overwrite that historical binding
  with a same-head rebuild or current-install hash, and fail closed when
  retained authority cannot be validated.
- Make the formal-rerun wrapper verify matching local, synchronized-workspace,
  remote-build, installed-release, and OBC-binary provenance before target C
  can execute or a target/campaign PASS can become authoritative, while
  rejecting untracked inputs unless their local-only roots are also excluded
  from target synchronization.
- Install and retain the rendered OBC systemd unit as a trusted receipt, then
  require pre-C provenance to match the active unit, exact A-owned CAN-FD and
  Beacon drop-ins, effective launch path, and service process tree.
- Reject a caller-selected alternate OBC systemd service so the formal runner,
  A, C, and provenance checker cannot bind different process identities.
- Bind the no-`.git` workspace marker to a deterministic manifest/digest of
  every synchronized Git-indexed path and to marker-time hashes for every
  remote-build input copied into the target bundle; recompute the live
  workspace and build-input hashes before C so `SKIP_DEPLOY`, resume, or a
  mutually consistent later package/install cannot hide remote source or build
  drift. Before sync, compare the serialized bytes and modes with committed
  superproject/submodule trees so index flags and file-mode configuration
  cannot hide uncommitted inputs.
- Revalidate campaign identity and run a fresh native OBC build before every
  hosted attempt that actually executes, independent of target deployment
  controls, while leaving retained authoritative hosted PASS evidence skipped.
- Make formal target synchronization serialize Git-indexed files only, and
  bind campaign resume to the branch/head/version recorded before the first
  attempt in both the identity file and retained campaign manifest; revalidate
  that same identity after hosted execution and before target
  restart/provenance on every invocation. Initialize recursive submodules
  before first identity capture on fresh deployment runs so a clean clone
  reaches the committed-tree gate without changing skip/resume behavior.
- Govern the existing attempt-aware importer metadata (`--attempt-label`,
  `--retry-of`, and `--failure-class`) as part of reviewable failed-attempt
  provenance, while confining attempt labels to one safe route/surface child
  and retaining a distinct path/SHA-256-bound revision-provenance artifact for
  every target attempt.
- Reconcile Route 1 records, payload evidence guidance, architecture status,
  and verification-registry evidence routing with the frozen campaign.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `verification-evidence`: Permit manifest-backed canonicalization of
  byte-identical derived decode outputs while requiring retention of raw
  transport bytes, provenance, reconstruction data, and verdict oracles.
- `route1-sequence-verification`: Classify the 2026-07-12 proof and
  thesis-backed 2026-07-20 values as historical/non-authoritative functional
  observations, leave target requalification pending, and require their
  critical values, limitations, provenance artifacts, and evidence links to
  remain mechanically checkable.

## Impact

- Evidence and documentation under `evidence/records/`,
  `docs/architecture/`, and `evidence/verification-path-registry.md`.
- Route 1 artifact import/checking scripts and their focused tests.
- OpenSpec main specifications and reconciliation records.
- No flight-software, topology, protocol, HTTP, F' command, Mission Console,
  deployed target runtime, or RF behavior changes. Neither Route 1 C stage
  permits a shared-service transport override, and the formal-rerun
  provenance gate only governs evidence eligibility.
- Local verification intentionally excludes the repository full gate and new
  hosted/target campaign execution for this evidence-only change; the existing
  2026-07-20 functional observation is not promoted into governed operational
  evidence, the 2026-07-12 narrative is not promoted without retained
  provenance, and the PR must still pass the hosted `baseline-gate`.
