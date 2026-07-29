# Design: hosted-probe-startup-order

## Startup Order Guidance

The skill should teach a concrete sequence for probes that depend on `fprime-gds`, `fprime-cli events`, `fprime-cli channels`, gateway processes, and an OBC runtime:

1. Allocate GDS/TTS and adjacent runtime ports with a real bind check where possible.
2. Start headless `fprime-gds`.
3. Wait for GDS readiness using configured host/port values rather than hard-coded bind strings.
4. Start `fprime-cli events` and `fprime-cli channels` listeners before starting the OBC or other traffic-producing runtime.
5. Give the listeners a short settle interval.
6. Start gateways, COMM nodes, and target simulators.
7. Start the OBC/runtime process.
8. Inject bounded commands and assert the expected readback, events, and channels.
9. Avoid running probes that share GDS/TTS/ZMQ/runtime-root resources in parallel.

## Why This Belongs In `hosted-probe-workflow`

The existing skill already covers hosted probe design, evidence scope, fresh builds, sockets, and registry discipline. The missing piece is operational sequencing. Adding it to the existing skill is more discoverable than creating a second skill because agents already invoke `hosted-probe-workflow` when maintaining repository-owned hosted probes.

## Verification

Because this is a governance/skill change, verification is static:

- `openspec validate hosted-probe-startup-order`
- `openspec validate --specs`
- `python3 scripts/check_repo_consistency.py`
- a small script that reads `.codex/skills/hosted-probe-workflow/SKILL.md` and confirms the startup-order section contains the expected ordered concepts.
