# Tasks: target-version-metadata-v1

## 1. OpenSpec baseline

- [x] 1.1 Write the proposal, delta specs, and design for the Raspberry Pi version metadata fix

## 2. Repo-local version override path

- [x] 2.1 Add a project-level version target override that can accept explicit framework/project version inputs without modifying `lib/fprime`
- [x] 2.2 Keep hosted builds working by falling back to the stock framework version lookup when overrides are not supplied

## 3. Raspberry Pi bootstrap integration

- [x] 3.1 Update the governed Raspberry Pi bootstrap flow to export host-derived framework and project version metadata into the target build
- [x] 3.2 Rebuild the synced Raspberry Pi workspace and verify the generated version metadata no longer falls back to `v3.5.0`

## 4. Evidence and archival

- [x] 4.1 Record target-side version metadata evidence and update the relevant operator-facing documentation
- [x] 4.2 Run OpenSpec validation for the change, archive it, and sync the main specs
