# Route 1 Sequence Verification v1

Status: current Route 1 sequence evidence record.
The 2026-07-12 proof remains a historical governed functional proof, not current target authority,
because its remote workspace revision and build/install provenance artifacts
were not retained. The repo-backed 2026-07-20 bundle is retained as a
thesis-backed functional observation with explicit governance limitations.
Route 1 target requalification remains pending.

## Scope

This record proves the current Route 1 verification boundary:

- compile the checked-in Route 1 payload sequence with the active dictionary;
- upload only through the governed .sequence-staging/<leaf> path;
- invoke SEQ_VALIDATE and SEQ_RUN(..., WAIT) through the repo-owned
  sequence-admission surface;
- observe fresh AUTO and DETERMINISTIC payload completion metadata;
- verify the required SoC/mode admission and the deterministic preview .fdp
  receipt; and
- keep the hosted proof separate from governed target provenance and target
  ground-observation evidence.

It does not prove a mission scheduler, persistent onboard schedule, generic
payload throughput, RF/OTA receipt, or a general sequence-control plane.

## Command Contract

The human-readable source is:

- scripts/manual_ops/examples/route1-demo.seq

The hosted and target probes use the same payload command signatures, capture
indices, governed staging convention, SEQ_VALIDATE, and SEQ_RUN(..., WAIT)
contract. Hosted uses shorter waits for its stub backend; target preserves the
longer camera-completion waits. This timing difference does not create a
second command interface.

Target mode admission remains: raise EPS simulator SoC, send EPS_GET_STATUS,
then MODE_SET IDLE and MODE_SET PAYLOAD. The mode oracle is SYS_MODE_CHANGE;
opcode completion alone is not enough.

## 2026-07-20 Repo-Backed Functional Observation

The run used the already-deployed target state and invoked A and B before and
after C for both Route 1 target stages, without a rebuild, package, or install.
Its complete uncompressed evidence tree,
including the full OBC journal and the `SEQ_VALIDATE` journal window, is in
the [frozen target observation root](../chapter5-integrated-route-closure-v1/ARTIFACTS.json).

The captured functional scenario passed S-band secure auth, EPS refresh,
IDLE/PAYLOAD mode entry, `SEQ_VALIDATE`, `SEQ_RUN(WAIT)`, AUTO index 48,
DETERMINISTIC index 49, deterministic preview ground receipt, and the 59%/39%
SoC fallbacks. It used a node-5 V3 profile of 240 data bytes.

This campaign is not accepted as current governed target closure. Its
checkpoints show that C installed and removed
`56-obc-groundlink-timeouts.conf`, restarting the shared OBC service, and the
campaign lacks complete contemporaneous local branch/head, remote workspace
head, and remote build metadata. Those facts violate the ownership and
provenance requirements above. The 7/20 values remain reviewable empirical
observations used by Chapter 5, not evidence that the run satisfied current
A/B/C governance. This is target-only evidence and does not renew a hosted
verdict.

The campaign is frozen through
[`dedup-manifest.json`](../chapter5-integrated-route-closure-v1/ARTIFACTS.json).
Only six SHA-256-identical decoded JSON derivatives were removed; the AUTO
and DETERMINISTIC source-side `decode-primary` JSON files are canonical.
Raw FDP/JPEG/BIN products, capture bytes, journals, channel/event logs, A/B
snapshots, checkpoints, summaries, provenance, and verdict oracles remain.
Every file selected by the retained-artifact globs is enumerated with its
SHA-256 in the manifest; glob counts are structural checks rather than a
substitute for content integrity. Because this observation claims the
DETERMINISTIC preview reached GDS, its manifest `receivedFdp` is mandatory and
hash-checked against the retained ground-received raw product; AUTO remains
source-only and records `null`. The directional gateway captures, the distinct
prepare-stage `native-cli/recv.bin` and `pipeline-logs/recv.bin` surfaces, and
the SoC-fallback CLI `events/recv.bin` surface are independently hash-locked.
The prepare-stage StandardPipeline `channel.log` and `event.log` observations
are independently hash-locked as well; filenames without their content
digests are not accepted.
The StandardPipeline `pipeline-store` FDP is also independently hash-locked
from the byte-identical GDS-runtime FDP because they are distinct ground
consumer observations.
The exact 7/20 sequence source and compiled sequence binary are also
independently hash-locked; the frozen source is not inferred from the current
working-tree example.
The repository checker protects the thesis-critical times, indices, 48,506
byte FDP, 48,187 byte JPEG, and 59/39 SoC outcomes:

    python3 scripts/check_route1_evidence_bundle.py

The removed 2026-07-19 campaign is no longer maintained in this tree and is
not a PASS authority. It can be recovered from review commit `ea020861`.

## Evidence-Freeze Verification Boundary

This evidence freeze and wrapper-ownership review follow-up do not claim a new
hosted or target execution. By explicit developer decision they do not run the
local full verification gate and do not rerun a hosted/target Route 1
campaign. Focused importer and evidence-checker tests, wrapper ownership and
syntax checks, sensitive-information, documentation, consistency, diff, and
OpenSpec checks are the local verification boundary.
The pull request must still pass the hosted `baseline-gate`. This exception
does not promote the dated 2026-07-20 observation into governed operational
evidence and does not retroactively promote the provenance-incomplete
2026-07-12 proof. Route 1 target requalification remains pending.

## Historical Retained Hosted Evidence

Command:

    ./fprime-venv/bin/cmake --build build-fprime-automatic-native --target OBC -j4
    PROBE_TMP_DIR=/tmp/route1-pr2-hosted-rerun-20260712 bash scripts/chapter5_routes/hosted/run_route1_hosted.sh

Verdict: PASS

Evidence root:

- /tmp/route1-pr2-hosted-rerun-20260712

The hosted aggregate and stage roots are deliberately separate:

- aggregate: /tmp/route1-pr2-hosted-rerun-20260712/summary.log
- sequence/payload stage:
  /tmp/route1-pr2-hosted-rerun-20260712/prepare-and-capture/
- SoC fallback stage:
  /tmp/route1-pr2-hosted-rerun-20260712/soc-fallback/

Observed hosted assertions:

- required hosted S-band node 5 and UHF node 6 baseline started;
- sequence compile, governed staging, SEQ_VALIDATE, and SEQ_RUN passed;
- sequence emitted fresh AUTO index 48 and DETERMINISTIC index 49 metadata;
- the deterministic preview .fdp received by GDS hash-matched the source
  family and extracted to a valid JPEG;
- live EPS control drove PAYLOAD -> IDLE -> SAFE as the SoC fell.

The first fresh attempt is retained only as diagnosis: it started node 5
without the current required node-6 hosted baseline. Node-6 owner timeouts
then overflowed the COM queue and left a zero-byte GDS output file. The
rerun starts the current dual-band baseline and is the authoritative hosted
result.

## Historical Retained Target Evidence

Target proof uses:

    PROBE_ROOT=/private/tmp/route1-pr2-target-20260712 bash scripts/chapter5_routes/target/run_route1_target.sh

The wrapper owns the required A -> B -> C -> A -> B sequence. A is
ensure_target_comm_lab_baseline.sh, B is
ensure_ground_dual_gds_baseline.sh, and C is the Route 1 functional
scenario. C may create only probe-owned local helpers, temporary overlays, and
evidence; it does not restart or stop shared target services. The required
manual-auth service profile is applied through A's
`TARGET_BASELINE_INCLUDE_MANUAL_AUTH_PREFLIGHT=1` option before B, rather than
as a wrapper-owned mutation between A/B and C. Both C stage entrypoints also
pin their service-profile expectations to the governed node-`5` S-band values,
and the externally managed helper verifies all six resulting service fields.
Any drift fails C without installing a drop-in or restarting the shared OBC
service; non-externally-managed legacy probes retain their existing
probe-owned override behavior.

### Fresh target result

Local source provenance was head `6aecc180e8f5e71635ebfe4b6ec9a89def63635f`.
The target workspace is deliberately a source sync without `.git`, so its
remote workspace head is explicitly unavailable rather than silently assumed.
Provenance therefore uses that local head plus the completed workspace sync, a
clean target native build, and the installed release pointer
`route1-pr2-6aecc180e`. The target build reports F' `v3.5.0` because the synced
workspace has no Git metadata; the package release identifier is the retained
revision linkage. A forced governed A restart confirmed that the live OBC
process executed that release's `bin/OBC`.

These narrative linkages are useful historical context but do not satisfy the
current evidence contract: no retained remote workspace identity marker,
remote build metadata, installed manifest, or installed OBC hash remains
available for independent checking. The result therefore cannot serve as
current authoritative target closure.

The sequence/payload stage passed at:

- `/private/tmp/route1-pr2-target-20260712/prepare-and-capture/`

It compiled and staged `route1-sequence-demo.bin` at
`.sequence-staging/route1-sequence-demo.bin`, then recorded successful
`SEQ_VALIDATE` and `SEQ_RUN(..., WAIT)`. Fresh real-camera AUTO index 48 and
DETERMINISTIC index 49 completion metadata were observed. The deterministic
preview's 61,780-byte catalog completed in 31.485 seconds; the final GDS
arrival followed 0.222 seconds later, decoded to a valid JPEG, and matched the
source artifact SHA-256.

The maintained wrapper now retains wrapper-owned `baseline/` and `summary.log`
at the stage root and gives the cleanup-owning C implementation a `scenario/`
child root. This preserves A/B preflight and postflight evidence without
altering the functional command or oracle contract above.

The 2026-07-12 review-follow-up rerun verified that ownership split at
`/private/tmp/route1-pr161-review-fix-20260712/`: all four wrapper-owned
`baseline/*-before.json` and `baseline/*-after.json` files remain at the stage
root, while the C summary remains at `scenario/route1-sequence-target-summary.json`.
The sequence C result was PASS with fresh deterministic catalog timing of
32.906 seconds to catalog completion and 0.207 seconds to final GDS arrival.
Its wrapper postflight A performed an A-owned repair for a stale diagnostics
override and a down availability marker; an immediate independent A -> B
postflight then returned READY/no-action with UHF required and enabled.

A second 2026-07-12 review follow-up corrected catalog pruning to retain every
member named by the deterministic preview's `familyFdpFiles`, rather than only
the metadata-reported first slice. The isolated hosted rerun passed at
`/tmp/r161h-e/` and the governed target sequence rerun passed at
`/private/tmp/route1-pr161-family-target-20260712/`, with the same full-family
source/received SHA oracle used for catalog receipt. The target wrapper's
postflight A performed its governed diagnostics cleanup; the final independent
A -> B retry returned READY/no-action with UHF required and enabled.

An earlier hosted retry using a deliberately long runtime root failed in
`FileDownlink` because its source filename exceeded the stock fixed filename
capacity. The deterministic family had one slice in that attempt, so this
failure preceded and was independent of the family-preservation change. The
short isolated runtime root above is the authoritative hosted result.

A final 2026-07-12 hosted review rerun at `/tmp/r161o-e/` added a bounded
`CatalogBuildComplete` wait before `START_XMIT_CATALOG` and required every
source-family SHA to arrive at GDS before extraction. The complete hosted Route
1 wrapper passed, including sequence completion, deterministic preview family
receipt, and the SoC fallback stage.

The initial aggregate fallback stage did not reach C because A repaired a
baseline transport condition and the subsequent manual-auth preflight hit a
transient target SSH banner timeout. It is retained as a baseline-readiness
diagnostic, not a functional verdict. Only the fallback C stage was then
retried with a fresh root:

- `/private/tmp/route1-pr2-target-20260712/soc-fallback-retry/`

That retry passed secure auth, target-journal EPS/mode evidence, 59% SoC
`PAYLOAD -> IDLE`, and 39% SoC `IDLE -> SAFE`. Its wrapper postflight A again
encountered a transient SSH banner timeout after C had passed; an immediate
independent governed A then B retry returned READY, with UHF required and
enabled. The proof is accepted only with this recorded final postflight. The
Route 1 wrappers now fail rather than hide a future postflight A/B failure.

## Evidence Interpretation

- A missing/mismatched local head, remote workspace head, build metadata, or
  installed release pointer is a provenance failure.
- Every hosted attempt that actually executes, including automatic retry and
  pending resume, first revalidates the campaign identity and runs a fresh
  native build of OBC and every hosted-wrapper executable under an
  attempt-specific log. `SKIP_DEPLOY=1` controls
  target synchronization, packaging, and installation only; it cannot make an
  existing native tree authoritative for a new hosted verdict. Resume skips a
  retained authoritative hosted PASS only after validating its
  attempt-recorded importer-manifest SHA-256, metadata, artifact roots, exact
  retained-file set, sizes, and SHA-256 values; it does not rebuild or relabel
  that historical observation.
- The maintained formal-rerun wrapper writes an explicit branch/head/version
  marker after its governed workspace sync and target build because the target
  workspace intentionally excludes `.git`. Marker schema v3 also retains the
  sorted path list and deterministic SHA-256 over path, Git-normalized mode,
  and content for every serialized Git-indexed superproject/submodule file,
  plus a package-path-keyed SHA-256 map of all eight remote-build inputs copied
  into the RPi bundle: six executables, the topology dictionary, and build
  version metadata. Before sync or marker creation, the checker verifies that
  every serialized byte and
  Git-normalized mode matches the committed superproject/submodule trees; this
  fails closed on assume-unchanged, skip-worktree, staged-index, and
  `core.filemode=false` drift even when Git porcelain reports clean. Before
  target C executes, the checker recomputes the live remote workspace digest
  and compares every current remote build input and installed-manifest package
  entry with its marker-time build hash. A changed, deleted, omitted,
  duplicated, or unsafe path, or any replaced executable, dictionary, or
  build metadata subsequently packaged and installed with mutually consistent
  downstream hashes, is therefore a provenance failure. A static regression
  also keeps this governed input map equal to the remote-build copies in
  `scripts/package_rpi_bundle.sh`.
  The complete marker is streamed over SSH stdin rather than placed in argv.
  The formal runner retains the ordinary A -> B readiness preflight for fresh
  campaigns. After the independently runnable hosted surface completes, every
  target C attempt,
  including an automatic retry and a pending `SKIP_DEPLOY=1` or resumed
  attempt, revalidates campaign identity, requests an A-owned forced OBC
  restart, and generates fresh target provenance. This prevents a
  runner-installed, externally installed, or between-attempt `current` release
  change from leaving the previous process executing or reusing the preceding
  attempt's provenance. Attempt-specific log labels preserve each identity,
  restart, and provenance observation; each target attempt also retains a
  distinct `deployment/target-revision-provenance-attempt-NN.json` whose path
  and SHA-256 are bound on that attempt record, so a retry cannot overwrite
  the failed attempt's provenance. The provenance check requires the
  marker, any remote Git head when present, remote build
  metadata, installed manifest/version metadata, the `current` release
  pointer, the installed-manifest SHA-256/content, and live size/SHA-256 values
  for every manifest-listed release file to agree with the intended local
  revision and a locally retained trusted package/install manifest. This
  includes launchers, helpers, configuration, dictionaries, and binaries
  rather than only `bin/OBC`; changing a file and the target-side manifest
  together still fails the local receipt. A fresh `SKIP_DEPLOY=1` invocation
  must provide `TRUSTED_INSTALLED_MANIFEST_PATH`, while resume keeps the copied
  campaign receipt. The gate also compares the active
  `obc-comm-csp-stack.service` fragment against the trusted rendered-unit
  receipt, requires exactly the A-owned `57-csp-socketcan-canfd.conf` and
  `58-target-beacon-baseline.conf` bytes, rejects unknown drop-ins, verifies
  the effective launch path, and audits the service process tree against the
  installed release. Fresh `SKIP_DEPLOY=1` therefore also requires
  `TRUSTED_SERVICE_UNIT_PATH`; resume preserves the campaign's unit receipt.
  The formal runner and both target wrappers require the canonical
  `obc-comm-csp-stack.service` name, so A/C cannot execute a caller-selected
  alternate unit while provenance audits the canonical unit.
  Both target wrappers clear caller-provided manual timeout values and
  timeout-drop-in naming before asking A for its manual-auth preflight.
  Pending `RESUME=1` target attempts do not
  bypass this gate, and an automatically retried attempt re-enters it before
  C. If resume already has an authoritative target PASS, the
  runner instead revalidates and preserves that attempt's original
  `deployment/target-revision-provenance-attempt-NN.json`, requiring its path
  and SHA-256 to match the retained target attempt; it does not restart target
  services or replace historical provenance with the current install. Before
  either hosted or target PASS is skipped, its importer
  manifest and every retained artifact byte are revalidated. A
  non-authoritative PASS or missing/corrupt/mismatched retained provenance,
  manifest, or artifact fails closed.
- The dedup manifest records explicit historical/non-authoritative status for
  both the 2026-07-12 proof and 2026-07-20 observation, plus pending target
  requalification, for every governing document. The evidence checker requires
  complete document coverage and rejects an authority promotion even when both
  dates and links remain; `not` embedded inside an authority-promoting word
  such as `notably` is not accepted as negation, and an approved
  non-authority sentence does not permit a contradictory positive authority
  statement elsewhere in the document. Double-negated wording such as
  `not non-authoritative` fails explicitly, and each semicolon/colon clause
  must carry its own valid non-authority meaning. Contrast clauses introduced
  by `while`, `although`, or `though` are also independent, and a positive
  `remains` predicate fails validation independently. Common numeric, English
  month-name, and Chinese spellings of 2026-07-12 and 2026-07-20 map to the
  same bounded checker subjects; unrelated current evidence remains
  outside this dated-observation check.
- Formal Route 1 provenance distinguishes generated evidence from source
  inputs and rejects undeclared untracked build inputs before workspace
  marking or target authority, so an unreviewed input cannot be attributed to
  `HEAD`.
- The formal Route 1 target sync additionally enables Git-index-only archive
  construction, including initialized submodule files, and extracts into a
  staged replacement workspace while retaining the prior workspace as a
  bounded sibling backup. A fresh deployment initializes and aligns recursive
  submodules before recording campaign identity, allowing a clean clone to
  reach the committed-tree gate; `SKIP_DEPLOY` and resume keep their prior
  behavior. Ignored, untracked, and stale prior working-tree files therefore
  cannot enter the active target build workspace. The marker's deterministic
  tracked-content binding is rechecked for `SKIP_DEPLOY=1`, pending resume, and
  every automatic target retry; ordinary developer sync behavior is unchanged.
- Before its first attempt, a formal campaign records
  `deployment/campaign-source-identity.json` with branch, head, and project
  version. `RESUME=1` validates that identity against both the current checkout
  and the branch/head/version already recorded in the retained campaign
  manifest before loading prior attempts. Every invocation validates it again
  after hosted execution and before target restart/provenance, so replacing an
  identity file cannot relabel hosted and target PASS records across revisions.
- Attempt labels are confined to one safe path component below
  `<route>/<surface>`; absolute, traversal-containing, or multi-component
  labels are rejected before the importer creates any artifact directory.
- A failed revision gate is retained in the attempt-specific provenance file
  and copied to the campaign-level
  `deployment/target-revision-provenance.json` latest view; it blocks target C
  and prevents both the target attempt and campaign manifest from becoming
  authoritative PASS evidence.
- A missing service, stale listener, or missing UHF readiness is a
  baseline-readiness failure.
- A stale output, wrong observation source, or timeout boundary is a
  probe-oracle failure.
- Product behavior is considered only after the three preceding categories
  have been excluded.

docs/verification.md is diagnostic context, not this
record's sole PASS authority.
