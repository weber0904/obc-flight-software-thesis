## Context

The repository already distinguishes current maintained probes from historical
or retired families in the registry and runbooks. That governance layer is
working, but it is not enough to stop accidental direct execution from the
`scripts/` directory. The current risk is operational, not architectural:
someone sees a plausible probe filename, runs it, and only later discovers that
the wrapper was never meant to be part of the current baseline.

The wrappers that need hardening now are narrow and already classified:

- `run_official_sequencing_system_resources_v1_probe.sh`
  - supplemental historical reference only
- `run_target_timing_empirical_ceiling_freeze_v1_probe.sh`
  - retired historical timing wrapper
- `run_target_timing_wcet_profile_proof_v1_probe.sh`
  - retired historical timing wrapper

## Goals / Non-Goals

**Goals:**

- Make the above wrappers self-identify at the script entrypoint.
- Default retired wrappers to immediate refusal with a clear explanation.
- Default supplemental historical wrappers to refusal unless the caller opts in
  explicitly.
- Keep archived evidence intact and avoid rewriting historical records.

**Non-Goals:**

- Requalifying any historical wrapper back into the maintained gate set
- Migrating retired timing probes onto secure-auth or Mission Console paths
- Deleting archived evidence or historical script files
- Changing product/runtime behavior outside wrapper entrypoint handling

## Decisions

### 1. Retired timing wrappers fail closed unconditionally

The timing wrappers are already classified as retired historical surfaces, not
maintained closeout gates. Their entrypoint behavior should match that
governance truth directly. They will exit immediately with a message that
points developers to the registry/runbook/current-baseline authority.

Alternative considered:

- Leave them runnable and rely on docs only
  - Rejected because that preserves the current footgun.

### 2. Supplemental historical wrappers require explicit opt-in

The hosted official sequencing wrapper is still potentially useful as a
historical reference surface, but it should not look like a current default
probe. Requiring `ALLOW_HISTORICAL_WRAPPER=1` keeps the file reviewable while
making accidental execution less likely.

Alternative considered:

- Convert it to unconditional refusal like retired wrappers
  - Rejected because this wrapper is still intentionally kept as a bounded
    historical reference surface.

### 3. Match script behavior with registry/runbook wording

The entrypoint messages will use the same vocabulary already established in the
governing docs: `supplemental historical`, `retired historical`, `not current
maintained gate`, and redirect to current secure-baseline runbooks.

Alternative considered:

- Add a new checker that scans filenames only
  - Rejected because it does not help the person who already executed the
    script.

## Risks / Trade-offs

- [Risk] A developer who intentionally wants the historical wrapper may be
  surprised by the new refusal.
  - Mitigation: supplemental wrappers get a documented explicit override.

- [Risk] A future change may need to rehabilitate one of these wrappers.
  - Mitigation: the script message is localized and easy to revise as part of a
    governed requalification change.

- [Risk] Nearby historical wrappers outside this first batch may still remain
  directly runnable.
  - Mitigation: keep this change narrow and document the pattern so follow-on
    cleanup can apply it consistently.
