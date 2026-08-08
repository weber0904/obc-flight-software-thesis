# mission-console Specification

## Purpose
Define the repo-owned Mission Console Phase 1 operator surface that layers a
local Mission Gateway and server-rendered UI above the maintained manual
dual-GDS secure-operations baseline, preserving the existing command authority
while making dashboard, structured readback, and bounded negative-packet demo
workflows reviewable and repeatable.
## Requirements
### Requirement: Mission Console SHALL Provide A Repo-Owned Ground Operator Surface

The repository SHALL provide a repo-owned `Mission Console` Phase 1 surface
that layers a local Mission Gateway and server-rendered UI above the maintained
manual dual-GDS operator baseline without replacing stock `fprime-gds`.

#### Scenario: Hosted operator can use the Mission Console without replacing stock GDS
- **WHEN** the maintained hosted manual dual-GDS surface is active
- **THEN** operators SHALL be able to open the Mission Console locally and use
  dashboard, operator actions, detailed readback, and surface-status pages
- **AND** stock `fprime-gds` SHALL remain available as a separate engineering
  and fallback observation surface.

### Requirement: Mission Console SHALL Reuse The Existing Operator Authority

The Mission Console SHALL execute secure auth, secure-v2 command, governed
upload, and governed `SEQ_*` actions only through the repo-owned manual secure
operator surface and SHALL NOT introduce a second command authority.

#### Scenario: Mission Console uses the maintained secure helper path
- **WHEN** an operator triggers auth, command, upload, or sequence actions from
  the Mission Console
- **THEN** the Gateway SHALL route those actions through the maintained
  `manual_secure_ops` plus `secure_link_auth_lib.py` path
- **AND** it SHALL NOT claim stock GDS UI interception, direct raw command
  authority, or bypass of `SequenceAdmissionController`.

### Requirement: Mission Console SHALL Own Its Own Listener And Snapshot Layer

The Mission Console SHALL own gateway-managed `events` and `channels` listeners
 for each active operator surface so that dashboard and readback behavior do not
 depend on whether the maintained manual surface already started passive
 listeners.

#### Scenario: Gateway restarts listeners when surface identity drifts
- **WHEN** the active manifest root, `ownerPid`, or `gdsTtsPort` changes for an
  active context/band
- **THEN** the Mission Gateway SHALL restart its owned listener pair for that
  surface
- **AND** it SHALL mark any cached secure session or snapshot state derived
  from the old surface identity as stale.

### Requirement: Mission Console SHALL Distinguish Continuous, Change-Driven, Explicit Readback, And Diagnostics Surfaces

The Mission Console SHALL expose distinct surface/lifecycle truth, dashboard
continuous values, change-driven operator state, transition events, explicit
detailed readback, and diagnostics-only review surfaces. Dashboard state SHALL
retain latest known values and SHALL NOT depend solely on future state
transitions becoming visible before an explicit status request can return fresh
proof. For important operator state fields backed by shared `update on change`
telemetry, the paired explicit readback path SHALL produce a new
downlink-visible sample when the operator explicitly requests current status.

#### Scenario: Dashboard keeps continuous, change-driven, and explicit readback distinct
- **WHEN** operators inspect the Mission Console dashboard and readback pages
- **THEN** the dashboard SHALL present continuous values, change-driven
  operator state, and transition state without expanding into a raw full
  channel/event flood
- **AND** detailed `GET_*`, `PAYLOAD_*`, `SEQ_LOG_STATUS`, boot, recovery, and
  persistent-fault observations SHALL remain explicit bounded readback actions.

#### Scenario: Explicit status request does not silently return only cached truth

- **WHEN** a dashboard-facing operator-state field is backed only by
  change-driven or shared telemetry and the operator explicitly requests
  current status
- **THEN** the Mission Console SHALL use its defined explicit readback path to
  obtain a new downlink-visible sample when needed
- **AND** it SHALL NOT silently treat a cache-only rewrite as successful fresh
  proof.

### Requirement: Mission Console SHALL Support Structured Operator Jobs

The Mission Console SHALL model operator actions as structured requests and
results with persisted job and history records so later planner work can reuse
the same action contract.

#### Scenario: Operator actions are recorded as structured history
- **WHEN** an operator runs an auth, command, upload, sequence, readback, or
  packet-lab action
- **THEN** the Mission Gateway SHALL record a structured result with action
  kind, context, band, timestamps, success/failure, and bounded artifacts or
  readback payloads
- **AND** that history SHALL be available without scraping raw console output.

### Requirement: Mission Console SHALL Provide A Bounded Negative Packet Demo Surface

The Mission Console SHALL provide a lab-only negative packet demo surface that
can inject selected replay, stale-session, sequence, and MAC-error cases and
explain why those packets are wrong using bounded parsed field summaries.

#### Scenario: Demo surface explains why a malformed or replayed packet is wrong
- **WHEN** an operator runs a negative packet demo case from the Mission
  Console
- **THEN** the UI SHALL show the packet source, bounded key fields, the
  intentionally wrong field or condition, the expected failure class, and the
  observed evidence
- **AND** it SHALL NOT require a full raw packet dump to understand the demo.

### Requirement: Mission Console SHALL Provide Dictionary-Backed Command Discovery
The Mission Console SHALL expose a context-scoped command catalog derived from
the active `AppTopologyDictionary.json` plus a curated overlay so operators can
discover mission-facing and engineering commands without memorizing raw command
names.

#### Scenario: Command catalog merges dictionary truth with curated UX metadata
- **WHEN** an operator or page client requests the Mission Console command
  catalog for an active context
- **THEN** the Gateway SHALL return dictionary-backed command name, opcode,
  command kind, and formal parameter truth for that context
- **AND** it SHALL merge curated group, label, description, default visibility,
  and field-hint metadata without changing the underlying command authority
  semantics.

### Requirement: Mission Console SHALL Provide A Searchable Schema-Driven Ops Workspace
The Mission Console SHALL provide a searchable `/ops` command workspace that
renders one operator input per formal parameter by default and keeps a visible
raw fallback path for uncurated or weakly inferred commands.

#### Scenario: Mission-first command selection renders parameter fields
- **WHEN** an operator filters and selects a command from the Mission Console
  `/ops` workspace
- **THEN** the UI SHALL filter commands by name, label, description, and
  argument metadata
- **AND** it SHALL render parameter inputs from the command schema while still
  serializing the selected values into the existing `commandArgs` array for the
  current POST command route.

#### Scenario: Engineering commands remain available behind show-all
- **WHEN** the operator enables the Mission Console show-all command toggle
- **THEN** the UI SHALL reveal hidden-by-default dictionary-backed engineering
  commands without removing the default mission-facing grouping behavior.

### Requirement: Mission Console SHALL Render Structured Operator Views
The Mission Console SHALL render structured, human-readable views on the
dashboard, readback, packet-lab, and surfaces pages instead of defaulting to
raw JSON blocks as the primary presentation layer.

#### Scenario: Dashboard and readback pages present structured evidence
- **WHEN** an operator views dashboard cards, recent events, or readback
  results
- **THEN** the Mission Console SHALL present primary values, freshness, and
  evidence using readable rows, badges, timelines, or structured result panels
- **AND** raw identifiers and secondary raw detail SHALL remain available
  without making JSON dumps the primary operator view.

#### Scenario: Packet-lab and surfaces pages present readable diagnostics
- **WHEN** an operator views packet-lab results, packet-lab history, surface
  registry state, or action history
- **THEN** the Mission Console SHALL present case descriptions, expected
  failures, observed evidence, lifecycle state, secure-state summaries, and
  compact history rows in structured views
- **AND** raw packet hex or backend detail SHALL remain secondary and
  collapsible.

### Requirement: Mission Console SHALL Provide A Curated Live Trend Workspace

The Mission Console SHALL provide a dedicated `/trends` workspace that lets
operators observe curated continuous telemetry as multi-series line charts
without leaving the repo-owned operator surface.

#### Scenario: Operator overlays multiple curated telemetry series on one chart
- **WHEN** an operator opens the Mission Console `/trends` page for an active
  context and band
- **THEN** the UI SHALL present subsystem-grouped telemetry selectors for the
  curated continuous channel set
- **AND** the operator SHALL be able to enable multiple channels and see them
  plotted together on the same chart.

#### Scenario: Trend workspace stays bounded to curated operator-facing channels
- **WHEN** the Mission Console renders the default trend selector set
- **THEN** it SHALL expose only the curated operator-facing continuous
  telemetry channels approved for live trends
- **AND** it SHALL NOT default to exposing the full raw channel inventory as
  operator chart inputs.
- **AND** the first active selector tranche SHALL limit those default
  subsystem groups to `EPS`, `ADCS`, and `Health` rather than promoting
  `GPS` by default.

### Requirement: Mission Console SHALL Maintain Bounded Channel History For Live Trends

The Mission Console Gateway SHALL maintain bounded per-channel history for
curated live-trend channels so that charts are derived from repo-owned
snapshot truth rather than raw log tailing or unbounded client-side caching.

#### Scenario: Gateway returns bounded history for requested trend channels
- **WHEN** the Mission Console frontend requests trend history for one or more
  curated channels on an active context and band
- **THEN** the Gateway SHALL return bounded time-series samples for each
  requested channel from its own maintained history cache
- **AND** the response SHALL exclude unbounded historical accumulation.

#### Scenario: Context or band switch retargets trend history cleanly
- **WHEN** the operator changes the active context or band while viewing the
  Mission Console trend workspace
- **THEN** subsequent trend requests SHALL use the new context/band history
  source
- **AND** the UI SHALL NOT continue plotting samples from the previously
  selected surface as if they belonged to the new target.

### Requirement: Mission Console SHALL Preserve Context And Band Selection Across Primary Pages

Mission Console SHALL remember the operator's last selected context and
per-context band across the primary operator pages so cross-page workflows do
not reset the active band unexpectedly.

#### Scenario: Operator returns to the previously selected band on the same context
- **WHEN** the operator switches between `/dashboard`, `/ops`, `/trends`,
  `/readback`, `/sequences`, `/surfaces`, or `/packet-lab`
- **AND** the chosen context and band still exist
- **THEN** Mission Console SHALL restore that same context and band selection
  rather than resetting the operator back to `sband`.

#### Scenario: Missing stored band falls back safely
- **WHEN** the last stored band for a context is no longer available
- **THEN** Mission Console SHALL fall back to the first currently available
  band for that context
- **AND** it SHALL NOT keep a stale invalid band selection active.

### Requirement: Mission Console SHALL Present An Operator-First Dashboard

The Mission Console dashboard SHALL prioritize current operator truth and
subsystem summaries over autonomy breadcrumb fields, and SHALL surface EPS and
ADCS status in dedicated operator-facing sections.

#### Scenario: Dashboard demotes mode-safety breadcrumb fields
- **WHEN** the Mission Console renders the dashboard primary summary
- **THEN** `MODE_SAFETY_LAST_CURRENT_MODE` and
  `MODE_SAFETY_LAST_TARGET_MODE` SHALL NOT occupy the primary mission posture
  summary alongside `SYS_MODE`
- **AND** any continued display of those breadcrumb fields SHALL be presented
  as secondary autonomy detail rather than primary operator truth.

#### Scenario: Dashboard includes explicit EPS and ADCS operator summaries
- **WHEN** the Mission Console renders the operator dashboard
- **THEN** it SHALL include dedicated EPS and ADCS summary sections derived
  from the curated operator-facing snapshot set
- **AND** those sections SHALL remain readable without requiring the operator
  to inspect raw JSON or leave the dashboard for a basic status summary.

#### Scenario: Dashboard mixes context-latest posture with band-scoped secure session
- **WHEN** the Mission Console renders `Satellite Status`, `Comm State`,
  `EPS Snapshot`, `ADCS Snapshot`, or `Health Snapshot`
- **THEN** it SHALL use the newest available observation from the selected
  context across its maintained bands
- **AND** it SHALL preserve the originating `sourceBand` for UI display.
- **AND** `Secure Session` SHALL remain scoped to the operator's currently
  selected band rather than being promoted into context-wide summary truth.

### Requirement: Mission Console SHALL Use A Consistent Mission-Control Visual Language

The Mission Console SHALL present dashboard, trends, and readback views with a
consistent repo-owned visual language that is readable for operators and
reviewable in demo settings without collapsing into stock debug-panel styling.

#### Scenario: Primary operator pages share spacing, hierarchy, and restrained chrome
- **WHEN** the Mission Console renders dashboard, trends, or readback viewer
  pages
- **THEN** those pages SHALL use a consistent spacing-first layout, restrained
  borders/shadows, and stable typography hierarchy
- **AND** they SHALL NOT regress into dense form-wall or raw debug-panel
  styling as the primary operator presentation.

#### Scenario: Trend charts preserve subsystem distinction without uncontrolled color noise
- **WHEN** the Mission Console renders multi-series operator trend charts
- **THEN** chart colors SHALL remain controlled and semantically grouped by
  subsystem
- **AND** the interface SHALL preserve clear visual association between trend
  lines, selectors, and latest-value indicators.

#### Scenario: Trend workspace supports multiple independent panels
- **WHEN** an operator opens the Mission Console `/trends` page
- **THEN** the workspace SHALL provide multiple trend panels with independent
  subsystem and series selection
- **AND** each panel SHALL fit its Y-axis only against its own selected series
  rather than a global shared chart scale.

### Requirement: Mission Console SHALL Separate Readback Viewing From Readback Proof

The Mission Console SHALL present saved subsystem readback values as an
operator-facing viewer and SHALL keep proof/debug evidence as a secondary layer
rather than as the primary readback experience.

#### Scenario: Readback viewer centers saved subsystem values
- **WHEN** an operator opens the Mission Console readback view after one or
  more `GET_*` actions have succeeded
- **THEN** the UI SHALL present the latest saved readback values grouped by
  tabs for the supported subsystem or readback families on the same route
- **AND** it SHALL show the last refresh time, source command, and refresh
  status without requiring the operator to read proof-oriented evidence fields
  first.

#### Scenario: Proof evidence remains available without dominating the viewer
- **WHEN** the operator inspects readback diagnostics for a saved result
- **THEN** the Mission Console SHALL still make readback family, fresh
  evidence, fallback, incomplete-result, reject, and provenance details
  available
- **AND** those proof/debug details SHALL be visually secondary to the saved
  readback values.

#### Scenario: Readback proof details remain open across viewer refresh
- **WHEN** the operator expands the readback proof/debug details for a saved
  result
- **THEN** the Mission Console SHALL preserve that expanded state across
  viewer polling and rerender
- **AND** it SHALL NOT auto-collapse the details solely because the viewer
  refreshed.

### Requirement: Mission Console SHALL Keep GET-Style Readback Actions In The Ops Surface

The Mission Console SHALL keep `GET_*` and status-command execution in the
operator action surface and SHALL NOT require the readback viewer itself to be
the primary command-dispatch page for readback actions.

#### Scenario: Operator runs a GET command from ops and later reviews the saved result
- **WHEN** an operator dispatches a `GET_*` or status command from the Mission
  Console ops workspace
- **THEN** the action SHALL be recorded through the existing job/action path
- **AND** the resulting saved readback SHALL become available from the
  readback viewer without requiring the operator to re-run the command from
  the viewer page.

### Requirement: Mission Console SHALL Support Card-Level Readback Quick Refresh

The Mission Console readback viewer SHALL support bounded in-place re-runs of a
saved readback card's source command without turning the viewer back into a
primary command-dispatch wall.

#### Scenario: Saved readback card can refresh itself in place
- **WHEN** a saved readback card has a known source `GET_*` or status command
- **THEN** the readback viewer SHALL expose a card-level quick refresh action
- **AND** invoking that action SHALL reuse the existing readback job path and
  write back into the same saved readback entry for the current
  `contextId + band`.

#### Scenario: Quick refresh preserves viewer state while updating the saved result
- **WHEN** the operator refreshes a saved readback card from the viewer
- **THEN** the card SHALL expose its own refresh state independently of other
  cards
- **AND** successful refresh SHALL update the saved values and last refresh
  time without collapsing existing proof/debug or raw-payload expand state.

### Requirement: Mission Console SHALL Support Bounded Dashboard Cache Maintenance

The Mission Console dashboard SHALL remain a display-only page while still
giving operators bounded maintenance controls for clearing stale local
observability residue.

#### Scenario: Dashboard clears selected-context cache without dispatching flight commands
- **WHEN** the operator invokes `Clear context cache` from the dashboard
- **THEN** Mission Console SHALL clear the selected context's saved snapshot,
  saved readback, and trend cache layers
- **AND** it SHALL NOT clear action history, packet-lab history, secure-state,
  or mutate flight-side state.

#### Scenario: Dashboard clears only the selected recent-event ring
- **WHEN** the operator invokes `Clear current view` from dashboard recent
  events
- **THEN** Mission Console SHALL clear only the selected context/band recent
  event ring
- **AND** it SHALL NOT delete broader history outside that bounded view.

### Requirement: Mission Console SHALL Present Packet-Lab Faults As Human-Readable Packet Diagnostics

The Mission Console packet-lab surface SHALL explain the intentional packet
fault itself, not merely the fact that the packet was rejected.

#### Scenario: Packet-lab result shows the wrong field directly
- **WHEN** an operator injects a packet-lab negative case
- **THEN** the Mission Console SHALL show which packet field or condition was
  intentionally made wrong for that case
- **AND** it SHALL present that explanation in the main packet-lab result
  view without requiring the operator to inspect a separate raw-event page.

#### Scenario: Packet-lab result compares expected and actual key values
- **WHEN** a packet-lab case has a meaningful expected-versus-actual
  comparison for sequence, session, MAC, or similar key fields
- **THEN** the Mission Console SHALL show the expected value or condition and
  the actual injected value or condition side by side
- **AND** the result SHALL make the fault intelligible even when the operator
  does not inspect raw hex.

#### Scenario: Reject reason codes are decoded into readable names
- **WHEN** the packet-lab result uses reject telemetry or reject events that
  expose numeric reason codes
- **THEN** the Mission Console SHALL decode those reason codes into readable
  reject names in the packet-lab view
- **AND** it SHALL still preserve the original numeric value as secondary
  detail for debugging.

#### Scenario: Packet-lab distinguishes injected fault intent from observed rejection
- **WHEN** the Mission Console renders a packet-lab result
- **THEN** it SHALL present the ground-side injected fault model separately
- **AND** it SHALL present the observed flight rejection evidence separately
- **AND** it SHALL NOT imply that the decoded reject reason alone defines what
  the operator intentionally injected.

### Requirement: Mission Console SHALL Keep Operator Histories Session-Scoped By Default

Mission Console SHALL default action-history and packet-lab history views to
the current console session while preserving an explicit way to review or clear
broader history.

#### Scenario: History defaults to the current Mission Console session
- **WHEN** the operator opens `/surfaces` or `/packet-lab`
- **THEN** the default history list SHALL show only entries from the current
  Mission Console session
- **AND** the operator SHALL be able to opt into a broader `show all` view.

#### Scenario: Operator clears the current session without deleting all history
- **WHEN** the operator requests history cleanup from `/surfaces` or
  `/packet-lab`
- **THEN** Mission Console SHALL support clearing only the current session by
  default
- **AND** full history clearing SHALL remain an explicit secondary action.

### Requirement: Mission Console SHALL Provide A Governed Sequence Authoring Workspace

The Mission Console SHALL provide a dedicated sequence-authoring workspace that
helps operators create governed official sequence artifacts without bypassing
the existing repo-owned sequence authority path.

#### Scenario: Operator builds official sequence source from structured steps
- **WHEN** an operator opens the Mission Console sequence-authoring workspace
- **THEN** the UI SHALL let the operator build a sequence as ordered timed
  steps using command selection and generated parameter fields
- **AND** it SHALL render the resulting official `.seq` source text as an
  inspectable first-class output rather than hiding the final sequence format.

#### Scenario: Sequence compilation uses the active context dictionary and official compiler
- **WHEN** an operator requests sequence compilation from the Mission Console
  sequence-authoring workspace
- **THEN** the backend SHALL compile the generated or edited `.seq` source by
  invoking official `fprime-seqgen` against the active context
  `AppTopologyDictionary.json`
- **AND** the result SHALL distinguish compile success or failure from later
  upload, admission, or execution outcomes.

#### Scenario: Sequence drafts live under the Mission Console runtime root
- **WHEN** an operator saves or compiles sequence work from the Mission
  Console sequence-authoring workspace
- **THEN** draft metadata, generated `.seq`, and latest compile artifacts
  SHALL persist under `MISSION_CONSOLE_ROOT/sequence-drafts/`
- **AND** the workspace SHALL NOT write draft authoring state into the repo
  tree by default.

#### Scenario: Governed upload and execution remain on the repo-owned sequence path
- **WHEN** an operator uploads, validates, or runs a sequence produced through
  the Mission Console sequence-authoring workspace
- **THEN** the Mission Console SHALL reuse the existing governed
  `.sequence-staging/<leaf>.bin` upload path and `SequenceAdmissionController`
  `SEQ_*` actions
- **AND** it SHALL NOT expose raw `SeqDispatcher.RUN` or raw `CmdSequencer`
  controls as the primary operator execution surface.

### Requirement: Mission Console SHALL Provide A Dedicated Beacon Viewer Surface

Mission Console SHALL provide a dedicated beacon viewer surface for hosted and
target manual operator contexts without reclassifying beacon as stock GDS
event/channel traffic or explicit readback.

#### Scenario: Dashboard shows only the latest beacon summary
- **WHEN** a selected Mission Console context exposes a supported beacon
  capability
- **THEN** the dashboard SHALL show only the latest beacon time and sequence in
  its Beacon card
- **AND** it SHALL keep detailed provenance, decode status, and field payload
  off the dashboard summary surface
- **AND** unavailable beacon state SHALL remain a bounded summary condition,
  not a raw diagnostics dump.

#### Scenario: Beacon page shows decode detail and provenance
- **WHEN** an operator opens the Mission Console `/beacon` page for a context
  with beacon capability
- **THEN** Mission Console SHALL show latest decoded beacon detail, bounded
  beacon history, source kind, source band, capture-path metadata, and decode
  status
- **AND** it SHALL distinguish hosted local PTY capture from target remote
  sidecar capture
- **AND** it SHALL NOT present that surface as a stock GDS readback or RF/OTA
  receipt claim.

### Requirement: Mission Console SHALL Discover Beacon Support From Manual Surface Capability Metadata

Mission Console SHALL detect beacon availability from manual-surface manifest
capability metadata rather than inferring it from band names, stock GDS
traffic, or readback caches.

#### Scenario: Hosted and target contexts advertise beacon capability explicitly
- **WHEN** the hosted manual surface starts with beacon support or a target
  manual surface consumes ready A-published Beacon sidecar metadata
- **THEN** the surface manifest SHALL expose beacon capability metadata for the
  supported UHF operator surface
- **AND** the Mission Console gateway SHALL consume that capability to locate
  capture artifacts, frame size, and decode ancestry
- **AND** unsupported contexts SHALL remain explicit `supported=false`
  responses rather than implicit missing behavior.

#### Scenario: Target missing baseline sidecar safely degrades
- **WHEN** the target manual surface has no ready A-published Beacon sidecar
- **THEN** Mission Console SHALL not attempt a target service repair
- **AND** it SHALL expose the existing unsupported Beacon response.

### Requirement: Mission Console SHALL Normalize Operator-Facing Decimal Precision

Mission Console SHALL render finite decimal measurements with no more than two
fractional digits and SHALL normalize a rounded negative zero to `0` without
mutating the underlying API value or raw evidence.

#### Scenario: Structured fields and arrays use bounded decimal precision
- **WHEN** Mission Console renders decimal number primitives, explicit decimal
  strings, or decimal elements within a structured array
- **THEN** each decimal display value SHALL be rounded to at most two
  fractional digits
- **AND** redundant trailing zeroes SHALL not be displayed.

#### Scenario: Trend labels use the same bounded decimal precision
- **WHEN** Mission Console renders a trend latest-value label or trend-axis
  tick
- **THEN** it SHALL use the same at-most-two-decimal formatting rule as other
  structured operator fields.

#### Scenario: Negative zero is normalized for display
- **WHEN** a positive or negative decimal rounds to zero at two fractional
  digits
- **THEN** Mission Console SHALL display `0`
- **AND** it SHALL NOT display `-0`.

#### Scenario: Non-decimal and identity-bearing values remain unchanged
- **WHEN** Mission Console renders an integer number, integer string, enum,
  identifier, hexadecimal text, scientific-notation text without a decimal
  point, or other non-decimal string
- **THEN** it SHALL preserve the original displayed value
- **AND** it SHALL NOT coerce that value through the decimal formatter.

### Requirement: Mission Console SHALL Replay Valid Secure Packets From Maintained Capture Forms

The existing Packet Lab `replay-captured-raw` case SHALL accept both the
repository synthetic command-descriptor envelope and stock-GDS native capture
bytes while preserving the maintained manual secure-command authority.

#### Scenario: Existing synthetic capture remains authoritative
- **WHEN** a capture contains one or more complete synthetic
  command-descriptor envelopes
- **THEN** Packet Lab SHALL use the existing synthetic packet parsing behavior
- **AND** it SHALL NOT additionally select native-looking byte sequences from
  that capture.

#### Scenario: Valid secure packet is recovered from native capture noise
- **WHEN** no synthetic packet is present and a stock-GDS native capture
  contains unrelated prefix or suffix bytes around a complete secure-command-v2
  packet
- **THEN** Packet Lab SHALL recover the complete secure packet and its absolute
  capture offset for the existing replay workflow.

#### Scenario: Malformed native candidates are ignored
- **WHEN** a native capture contains candidates with an invalid secure magic,
  version, reserved field, header length, MAC length, inner command kind,
  trailer, or computed packet boundary
- **THEN** Packet Lab SHALL ignore those candidates and continue scanning for a
  later valid secure-command-v2 packet.

#### Scenario: No valid packet preserves the bounded failure
- **WHEN** neither a complete synthetic packet nor a valid native secure packet
  exists in the capture
- **THEN** `replay-captured-raw` SHALL fail with the existing no-reusable-packet
  error behavior
- **AND** it SHALL NOT send arbitrary capture bytes.

#### Scenario: Replay extension does not create a second command surface
- **WHEN** native capture replay is used
- **THEN** Mission Console SHALL continue to send the selected packet through
  the existing Packet Lab and manual secure-operations path
- **AND** it SHALL NOT add an HTTP endpoint, F' command, secure-command format,
  or target/RF verification claim.
