## Why

The public thesis release must be reproducible from its published Git tree and
must not carry development-workspace exceptions in its supported automation.
The pre-release audit found that the pinned F Prime revision is not currently
reachable from the configured public submodule remote, and that formal Route 1
automation still names a removed local thesis workspace.

## What Changes

- Require the declared public submodule revisions to be obtainable by a clean
  recursive clone before release approval.
- Remove the `.codex_thesis_work` provenance and target-sync exceptions from
  public Route 1 automation and its regression test.
- Preserve the existing F Prime Apache-2.0 modification notices and publish
  the pinned notice-only F Prime revision to its configured public fork.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `public-release-profile`: require a clean recursive checkout to resolve every
  declared public submodule revision and keep local thesis-workspace exceptions
  outside the public automation surface.

## Impact

This affects the public F Prime fork reachability, Route 1 campaign
provenance/synchronization scripts, their regression test, and release
verification. Runtime behavior, target service ownership, and recorded
evidence are unchanged.
