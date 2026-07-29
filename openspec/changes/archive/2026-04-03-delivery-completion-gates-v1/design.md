## Context

The repository already has strong norms around OpenSpec, local verification, commit hygiene, and CI, but those norms were refined during real delivery incidents and never written back clearly into the formal workflow. The key missing distinction is between "the change is ready locally" and "the change is formally complete after push and CI".

The most important practical lesson came from the earlier release flow: treating a local success as final before GitHub CI finished made it too easy to release/tag at the wrong moment. The delivery documents should therefore encode the checkpoints that the team now actually uses.

## Goals / Non-Goals

**Goals:**
- define a clear local-ready checkpoint before push
- define a clear formal-completion checkpoint after push and CI
- define when push happens in relation to developer confirmation
- define that release/tag work must wait for CI green
- mirror the same rules in narrative docs and README

**Non-Goals:**
- changing the repository branch model
- introducing automatic release tooling
- changing the existing verification commands or CI workflow contents
- revisiting broader OpenSpec governance beyond the specific completion gates

## Decisions

### Decision: Separate local-ready from formal completion

The workflow should explicitly distinguish the point where a change is implemented, verified locally, archived, and committed from the later point where the pushed commit has passed GitHub CI. This reflects the team's actual practice and removes ambiguity when reporting status.

Alternative considered:
- keep a single "done" state
  - rejected because it hides the operational difference between pre-push and post-CI status

### Decision: Require developer confirmation before push

The repo workflow should keep push as an explicit handoff step after the local-ready checkpoint is reported. This preserves the current collaboration model, where implementation is prepared first and then pushed only after the developer agrees.

Alternative considered:
- push automatically as soon as local work is ready
  - rejected because it does not match the established collaboration pattern

### Decision: Gate release and tag creation on CI green

Release and tag operations should only happen after the pushed commit has completed the required CI successfully. This captures the team's post-incident rule and prevents local success from being mistaken for a release-ready state.

Alternative considered:
- allow release/tag immediately after local verification
  - rejected because it recreates the failure mode the team already encountered

## Risks / Trade-offs

- [Risk] The workflow becomes a little more verbose. -> Mitigation: keep the new checkpoints narrowly defined and summarize them clearly in README.
- [Risk] Future contributors may still collapse local-ready and formal completion in conversation. -> Mitigation: encode the terminology directly in both formal and narrative delivery docs.
- [Risk] The repo still documents OpenSpec lifecycle separately from day-to-day collaboration steps. -> Mitigation: add the completion-gates section without rewriting the whole governance model.

## Migration Plan

1. Add delta requirements to `delivery-workflow`.
2. Update the narrative delivery workflow document with the explicit checkpoint sequence.
3. Update the README summary so readers can quickly see the local-ready versus CI-green distinction.
4. Validate, archive, and commit the change.
