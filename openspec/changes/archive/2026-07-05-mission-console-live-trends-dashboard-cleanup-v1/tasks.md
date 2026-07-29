## 1. Gateway Trend History Foundation

- [x] 1.1 Add a bounded per-channel history store to the Mission Console gateway snapshots layer keyed by context, band, and channel name.
- [x] 1.2 Record curated live-trend channel samples into the bounded history store whenever listener snapshots advance.
- [x] 1.3 Add a repo-owned trend-history API route that returns bounded samples for requested curated channels on the active context and band.
- [x] 1.4 Add automated tests for bounded trend-history retention, context/band separation, and empty-history responses.

## 2. Live Trends Workspace

- [x] 2.1 Add a dedicated `/trends` page and navigation entry to the Mission Console.
- [x] 2.2 Implement curated subsystem channel groups for the first live-trend selector set: EPS, ADCS, and Health.
- [x] 2.3 Add a repo-owned multi-series chart implementation that renders bounded trend data without depending on stock GDS addon assets.
- [x] 2.4 Implement client-side trend selection, polling, and chart refresh behavior for multiple overlaid channels on one chart.
- [x] 2.5 Add automated tests for curated trend metadata and trend API serialization used by the new trends page.
- [x] 2.6 Apply the shared mission-control visual language to trend layout, selector rows, legend chips, and chart framing.
- [x] 2.7 Persist `context / band` selector state across primary Mission Console pages with per-context band memory and safe fallback when a band disappears.
- [x] 2.8 Rework `/trends` into a multi-panel workspace with panel-local legends, per-panel axis fitting, and a bounded maximum panel count instead of one chart plus a permanent side legend.

## 3. Ops And Readback Separation

- [x] 3.1 Move curated `GET_*` and status-command execution responsibility fully into the ops workspace flow.
- [x] 3.2 Rework `/readback` into a saved viewer with tabs for `OBC`, EPS, ADCS, Payload, Storage, Boot & Recovery, and Sequence instead of a button wall plus single-job result pane.
- [x] 3.3 Preserve proof/debug evidence for readbacks in a secondary panel or `Proof / Debug Details` layer instead of presenting it as the primary reader surface.
- [x] 3.4 Make saved readback results persist visibly across command switches by reusing the existing readback cache/history model in the UI.
- [x] 3.5 Add automated tests for readback viewer serialization, saved-result grouping, and proof/debug secondary rendering.
- [x] 3.6 Move `/ops` `Job Result` into a compact workspace panel near the primary action controls instead of leaving it deep in the page flow or as an oversized sticky sidebar.
- [x] 3.7 Persist readback debug-panel open state across polling so the viewer does not auto-collapse proof details after refresh.
- [x] 3.8 Persist secondary raw readback details across polling and separate viewer-display channel sets from minimum closing-evidence channel sets.
- [x] 3.9 Add card-level quick refresh to `/readback` by reusing the existing readback job path and saved cache, without reintroducing a button wall.
- [x] 3.10 Keep `/dashboard` dispatch-free while exposing quick refresh metadata only through the `/readback` viewer model.

## 4. Dashboard Cleanup

- [x] 4.1 Rework dashboard card layout so `Satellite Status`, comm posture, EPS, ADCS, secure session, and health summaries have clearer hierarchy and spacing.
- [x] 4.2 Remove `MODE_SAFETY_LAST_CURRENT_MODE` and `MODE_SAFETY_LAST_TARGET_MODE` from the dashboard primary posture summary and rehome them as secondary autonomy detail if still shown.
- [x] 4.3 Add dedicated EPS and ADCS summary sections that surface curated operator-facing latest values from the existing snapshot truth.
- [x] 4.4 Update dashboard field mappings, labels, and rendering helpers to match the new operator-first summary structure.
- [x] 4.5 Perform manual UI cleanup on dashboard and trends so empty space, card density, and chart placement are reviewable on desktop and mobile.
- [x] 4.6 Normalize shared visual tokens for card spacing, shadows, typography hierarchy, and compact status chips across dashboard/trends/readback viewer.
- [x] 4.7 Aggregate default dashboard posture cards by context-latest observation while keeping `Secure Session` band-scoped and surfacing `sourceBand` to the UI.
- [x] 4.8 Add a dashboard-side clear control for the selected context/band recent-events ring without introducing dashboard dispatch actions for readback commands.
- [x] 4.9 Add a dashboard-side `Clear context cache` action that resets the selected context's saved observability cache without clearing action history or changing flight state.

## 5. Residual Chatter Boundary And Evidence

- [x] 5.1 Inventory residual continuously emitted diagnostics-only channels that remain live at runtime but are not promoted into default operator views.
- [x] 5.2 Update observability and handoff docs so the curated trend/dashboard/readback boundary matches the implemented UI behavior.
- [x] 5.3 Extend `scripts/test_mission_console_phase1.py` with trend-history and dashboard-structure contract tests.
- [x] 5.4 Run hosted Mission Console verification focused on dashboard, trends, and readback-viewer behavior, then record branch-head evidence in the phase1 test record.
- [x] 5.5 Re-tune the shared Mission Console accent/button treatment toward a simpler deep blue-gray palette while preserving separate semantic status colors.
- [x] 5.6 Move verbose dashboard channel provenance (`Background update / Flight sampled / Gateway observed / Via ...`) into hover tooltip metadata so the posture cards stay compact.

## 6. Packet-Lab Demo Readability

- [x] 6.1 Extend packet-lab result serialization with explicit expected-versus-actual fault fields for sequence, session, MAC, and other supported case kinds.
- [x] 6.2 Add reject-reason decoding from numeric telemetry/event codes to readable enum-style names while preserving the raw numeric value.
- [x] 6.3 Rework packet-lab result rendering so the main result view highlights the faulty field, expected condition, actual injected condition, and observed reject path without requiring a separate event view.
- [x] 6.4 Add automated tests for packet-lab reason decoding and expected-versus-actual serialization.
- [x] 6.5 Add hosted manual evidence that the packet-lab page itself is sufficient to explain why each demo packet is wrong.
- [x] 6.6 Split packet-lab wording between `Injected Fault Model` and `Observed Flight Rejection` so ground-side fault intent is not conflated with flight-side reject telemetry.

## 7. Governed Sequence Authoring Workspace

- [x] 7.1 Add a dedicated `/sequences` workspace and navigation entry for governed sequence authoring instead of folding full authoring into the existing `/ops` action forms.
- [x] 7.2 Add a repo-owned Mission Console backend helper that compiles official `.seq` source into `.bin` by invoking official `fprime-seqgen` against the active context dictionary.
- [x] 7.3 Implement a structured sequence-step editor for the first tranche using relative-time steps, command selection, generated parameter fields, and step ordering controls.
- [x] 7.4 Render the generated official `.seq` source as a first-class preview so operators can inspect the exact text format that will be compiled.
- [x] 7.5 Make compile diagnostics explicit and keep compile success distinct from governed upload/admission success in both backend results and UI copy.
- [x] 7.6 Reuse the existing governed file-upload path and `SequenceAdmissionController` `SEQ_VALIDATE / SEQ_RUN / SEQ_PREPARE_MANUAL` surfaces from the new sequence workspace instead of introducing raw sequencer controls.
- [x] 7.7 Add automated tests for sequence-source serialization, compile helper result shaping, active-dictionary routing, and governed workflow handoff from the sequence workspace.
- [x] 7.8 Update Mission Console runbooks/handoff and add hosted branch-head evidence for sequence build, compile, governed upload, and `SEQ_VALIDATE` from the new workspace.
- [x] 7.9 Persist sequence drafts, generated `.seq`, and latest compile artifacts under `MISSION_CONSOLE_ROOT/sequence-drafts/` instead of the repo tree.

## 8. Session-Bound Operator Histories

- [x] 8.1 Tag action-history and packet-lab-history entries with a Mission Console session identifier so UI defaults can isolate the current operator session.
- [x] 8.2 Default `/surfaces` Action History and `/packet-lab` history to the current Mission Console session while still allowing a bounded `show all` mode.
- [x] 8.3 Add repo-owned clear APIs and UI controls for clearing current-session history, with clear-all remaining an explicit secondary action.
- [x] 8.4 Render action-history and packet-lab-history containers as fixed-height scroll regions rather than allowing unbounded page growth.
