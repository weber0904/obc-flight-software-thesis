## Context

The libcsp migration was intentionally developed on a long-lived integration base rather than directly on `main`. That reduced risk while the internal substrate changed, but the integration branch now needs a formal release boundary so it does not become an unreviewed permanent fork.

## Decisions

1. **Use the integration branch as the release candidate.**
   - The merge target for the final PR is `main`.
   - The release readiness record cites the completed archived slices rather than rewriting their evidence.

2. **Do not retest unrelated hardware as part of release readiness.**
   - This change proves branch readiness through the shared local gate and focused libcsp checks.
   - Raspberry Pi and hardware UART validation are handled by the follow-on `rpi-csp-comm-baseline-validation-v1` change.

3. **Keep path wording strict.**
   - Internal EPS/ADCS CSP does not imply GDS, external comm, GPS, or real hardware validation.
   - libcsp ZMQHUB remains valid hosted infrastructure; project-local EPS/ADCS direct-ZMQ business traffic remains retired.

## Risks / Mitigations

- **Risk:** The release PR is treated as a feature slice and reviewers miss the integration scope.
  - **Mitigation:** The evidence names the exact archived changes included in the candidate baseline and records the full gate.
- **Risk:** Future docs still describe direct-ZMQ EPS/ADCS paths.
  - **Mitigation:** The release evidence reuses the legacy-ZMQ checker and updates status summaries.
