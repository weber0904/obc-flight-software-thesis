# Proposal: hosted-probe-startup-order

## Why

Extend the repo-local `hosted-probe-workflow` skill with a standard startup order for hosted probes that combine headless GDS, `fprime-cli` event/channel listeners, gateways, COMM nodes, and hosted OBC runtimes.

Recent hosted probe work showed a recurring failure mode: `fprime-cli channels` may miss early telemetry such as `GROUND_LINK_TX_BYTES` when listeners are started after the OBC or gateway path is already producing traffic. Prior evidence for UHF backup and COMM TT&C probes already converged on a more reliable pattern: start GDS, wait for readiness, start CLI listeners, allow a short settle interval, start gateway and simulator infrastructure, and only then start the OBC/runtime process that produces the traffic under observation.

Capturing that order in the skill should keep future agents from weakening probe assertions or adding runtime-log fallbacks when the real issue is listener timing.

## What Changes

- Add a codex-skills requirement for GDS / CLI listener startup ordering.
- Extend `.codex/skills/hosted-probe-workflow/SKILL.md` with the concrete startup sequence, port/readiness rules, and non-parallel execution cautions.
- Validate the skill text can be read and that a simple static check confirms the required ordered concepts are present.

## Non-Goals

- No product code changes.
- No hosted probe behavior changes in this PR.
- No new validation path evidence.
