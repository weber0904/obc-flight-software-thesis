## Why

`chapter_5_result.tex` defines three integrated validation routes that the
current repository still does not close end-to-end on the active baseline:

- Route 1 needs runtime EPS SoC stimulation so payload capture, low-SoC
  fallback, and official `.fdp` downlink can be exercised in one governed flow.
- Route 2 needs TTC entry to trigger the current ADCS mode surface
  automatically so the ground side can observe `POINTING` after pass-window
  entry.
- Route 3 needs a Chapter 5-owned probe layout that reuses the current
  recovery and target baseline instead of relying on obsolete probe families.

The old `mode/ttc/payload-ttc/comm-ttc` probe scripts no longer match the
current flight-software architecture closely enough to extend safely. The repo
also already proved that `mission-console-phase1` establishes the maintained
attach/baseline pattern, while its `packet-lab negative evidence` is not a
Chapter 5 blocker and should not become a prerequisite for the new route work.

## What Changes

This change creates one governed umbrella for Chapter 5 route closure:

- add an externally driven runtime SoC control surface to the EPS simulator
  without adding an OBC command path
- make TTC auto-entry best-effort trigger an internal ADCS `POINTING` mode
  switch and keep that hook non-blocking for TTC entry
- create a new `scripts/chapter5_routes/` tree for hosted and target route
  probes, staged wrappers, and command-playbook fallbacks
- align hosted and target launcher plumbing so the new Chapter 5 probes can
  drive the EPS simulator control socket on the maintained baseline
- update evidence and verification-path governance so multi-script or
  command-playbook closure remains formally reviewable

## Impact

Affected specs:

- `eps-subsystem`
- `adcs-subsystem`
- `mission-autonomy`
- `verification-evidence`
- `verification-path-registry`

Affected code:

- `simulators/eps/`
- `OBC/Components/TtcPassManager/`
- `OBC/Components/AdcsBridge/`
- `OBC/TopCcsds/`
- subsystem/hosted launch scripts under `scripts/`

Affected evidence and probe surfaces:

- new Chapter 5 staged hosted probes
- new Chapter 5 staged target probes under A/B/C governance
- future Chapter 5 evidence and registry updates that explicitly separate
  Route 1, Route 2, and Route 3 boundaries from adjacent Mission Console or
  historical probe surfaces
