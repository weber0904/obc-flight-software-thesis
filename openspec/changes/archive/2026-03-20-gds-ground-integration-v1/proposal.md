# Change Proposal: gds-ground-integration-v1

## Why

`deployment-runtime-v1` made the repository runnable, but it still stopped short of the first-version acceptance bar that the hosted stack can participate in a real F' ground path. The project now needs the deployment to wire command, event, telemetry, and framing services together, plus a documented repo-local launch path that proves the hosted OBC can attach to `fprime-gds`.

## What Changes

- Upgrade the hosted `OBC` deployment topology from text-event-only wiring to a GDS-connected F' stack using `CdhCore`, `ComFprime`, rate groups, and `Drv::TcpClient`.
- Keep the existing local REPL and simulator-driven workflow, but add a repo-local GDS launch helper and document the exact `fprime-gds -n -g none ...` command that matches the hosted deployment.
- Record evidence showing the headless GDS server and hosted OBC stack establish the documented TCP ground link.

## Impact

- Affected specs: `platform-baseline`, `comm-subsystem`, `verification-evidence`, `delivery-workflow`
- Affected code: `OBC/Top/`, `OBC/Main.cpp`, `scripts/`
- Affected docs: `obc-dev-spec/`, `evidence/records/`
