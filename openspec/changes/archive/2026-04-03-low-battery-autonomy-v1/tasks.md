## 1. MissionExecutive Scope

- [x] 1.1 Add the OpenSpec capability and delta specs for the first low-battery autonomy case.
- [x] 1.2 Define the first-version low-power entry and sun-safe pointing policy boundaries for this slice.

## 2. Core Implementation

- [x] 2.1 Add the `MissionExecutive` component and integrate it into the hosted OBC topology.
- [x] 2.2 Extend the EPS and ADCS bridge runtime helpers so the mission executive can consume cached state and issue the required commands without extra transport polling.
- [x] 2.3 Implement the low-battery response that latches `LOW_POWER` and commands ADCS pointing to the repository-owned sun-safe target.

## 3. Validation And Evidence

- [x] 3.1 Add automated tests for the mission executive policy and the hosted low-battery scenario path.
- [x] 3.2 Update repo-local evidence and user-facing docs for the first low-battery autonomy slice.
- [x] 3.3 Run the relevant local verification commands, validate OpenSpec, and prepare the change for archive.
