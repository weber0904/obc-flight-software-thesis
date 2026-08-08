## 1. OpenSpec Artifacts

- [x] 1.1 Write proposal and design artifacts for the hosted simulator stale-reap safety fix
- [x] 1.2 Add delta specs for the maintained per-band launcher cleanup boundary and evidence expectations

## 2. Implementation And Documentation

- [x] 2.1 Thread orphan-only stale cleanup through the managed-process helpers
- [x] 2.2 Apply orphan-only stale cleanup to the maintained per-band EPS/ADCS simulator launches
- [x] 2.3 Extend the maintained hosted per-band proof with a live unrelated-simulator non-interference check
- [x] 2.4 Add or update evidence for the new hosted cleanup boundary
- [x] 2.5 Sync the current-facing operator, progress, and architecture documents that still lag the merged per-band baseline or current timing truth

## 3. Verification And Closeout

- [x] 3.1 Run fresh local verification plus the affected hosted probe surfaces
- [x] 3.2 Run `openspec validate hosted-simulator-stale-reap-safety-v1` and `openspec validate --specs`
- [x] 3.3 Archive the change, regenerate reconciliation surfaces, and prepare the branch for review
