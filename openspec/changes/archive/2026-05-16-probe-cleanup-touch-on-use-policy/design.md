## Context

The repository already has a governed managed-cleanup pattern for hosted probes, but a backlog of older probes still uses hand-rolled `subprocess.Popen` plus ad hoc termination paths. Some of those probes also sit on top of old topology assumptions or known unstable runtime boundaries, so bulk cleanup work can easily turn into unrelated feature debugging.

## Goals / Non-Goals

**Goals:**

- Make the formal workflow prefer cleanup migration only when a current change actually needs a given probe.
- Require a minimal post-migration hygiene bar: rerun safety plus no owned helper or high-CPU orphan leftovers from the migrated probe path.
- Let current changes record bounded probe blockers instead of forcing unrelated deep debug when the migrated path is already known to be unstable or stale.
- Align the repo-owned hosted probe skill with the same rule so future agent behavior matches the formal spec.

**Non-Goals:**

- Do not batch-convert the remaining legacy probe backlog in this change.
- Do not redefine any product verification boundary or claim that previously failing probes are now fixed.
- Do not add a second governance surface to `AGENTS.md`.

## Decisions

### Keep the policy in `delivery-workflow` instead of `AGENTS.md`

`AGENTS.md` is an onboarding index, not the normative workflow source. The actual rule belongs in `openspec/specs/delivery-workflow/spec.md`, where future formal changes already look for push/CI/verification behavior.

### Put implementation details in the hosted-probe skill

The spec should stay short and normative. The skill is the right place to spell out how to recognize legacy cleanup, when to migrate, how to rerun safely, and when bounded blocker evidence is acceptable.

### Prefer touch-on-use over batch migration

Touch-on-use keeps the scope tied to an active feature or evidence boundary. That makes it easier to preserve probe meaning, avoid stale-architecture debugging, and spend effort only where the current change needs a probe result.

## Risks / Trade-offs

- [Legacy backlog remains partially unmigrated] → Acceptable because any probe reused by active work must be upgraded before it can support that work.
- [Different agents may still over-debug stale probes] → Mitigate by making the bounded blocker rule explicit in both the spec and skill.
- [CI still treats active OpenSpec workspaces as full verification scope] → Acceptable for now; this change only updates workflow expectations, not the CI classifier.
