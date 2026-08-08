## 1. Change Setup

- [x] 1.1 Finalize proposal, design, and spec deltas for target node-`5`/node-`6` migration and target/lab COMM detector redesign
- [x] 1.2 Add or update target profile mapping helpers so install/run/status/probe surfaces derive node and authority from `TARGET_COMM_PROFILE`

## 2. Target Service Graph

- [x] 2.1 Add subsystem node-`5` S-band service/target wiring on `subsystem.local`
- [x] 2.2 Add subsystem node-`6` UHF service/target wiring on `subsystem.local`
- [x] 2.3 Update OBC target service/install/status plumbing to use explicit target COMM profiles instead of node-`4` defaults
- [x] 2.4 Redefine target/lab COMM detector truth around repeated internal CSP ping failure to node `5`/`6`, while keeping ground-link state and transport noise as observability only

## 3. Probes And Verification

- [x] 3.1 Migrate target operational proof surfaces to node `5` default and node `6` quiet bounded paths
- [x] 3.1.1 Rework target `uhf-primary` proof to use explicit switch from node `5` before node `6` quiet command/readback
- [x] 3.1.2 Keep target `uhf-backup` proof separate from `uhf-primary-after-switch` semantics
- [x] 3.2 Move target recovery restart proof to node `5`
- [x] 3.3 Move target hardware-watchdog proof to node `5`

## 4. Documentation And Gates

- [x] 4.1 Update canonical docs/runbook/registry/evidence truth so node `4` is historical-only for target/lab
- [x] 4.2 Run fresh local verification plus focused target node-`5`, node-`6`, recovery, and watchdog proofs
