## Why

Repository-owned probes now use a mix of modern managed cleanup and older ad hoc process teardown. Trying to batch-convert every remaining legacy probe at once risks breaking paths whose current architecture, evidence boundary, or runtime assumptions are no longer well understood.

## What Changes

- Add a formal delivery-workflow rule that probe cleanup migration follows a touch-on-use policy instead of a mandatory backlog-wide batch rewrite.
- Require a change that depends on a legacy-cleanup probe to migrate that probe before relying on it as governed evidence.
- Require rerun-safety and owned-helper hygiene checks after a probe cleanup migration.
- Allow bounded evidence and explicit blocker recording when a migrated probe still fails due to known architecture drift or unrelated instability, instead of forcing unrelated deep debug in the current change.
- Update the hosted-probe workflow skill so future agent work follows the same rule in practice.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `delivery-workflow`: add the touch-on-use hosted-probe cleanup migration rule and bounded-failure handling expectations for reused probes

## Impact

- `openspec/specs/delivery-workflow/spec.md`
- `.codex/skills/hosted-probe-workflow/SKILL.md`
- `openspec/changes/probe-cleanup-touch-on-use-policy/specs/delivery-workflow/spec.md`
