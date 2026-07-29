# Verification Matrix

Status: reviewer-oriented current verification summary.
Last reconciled against the `thesis-submission-v1` release candidate on
2026-07-29.

This document is the checked-in capability-level summary of current verification coverage. It should be read together with:

- [`scripts/report_verification_inventory.py`](../scripts/report_verification_inventory.py)
- [`docs/test-records/`](test-records/)
- [`openspec/specs/verification-evidence/spec.md`](../openspec/specs/verification-evidence/spec.md)

This file is a reviewer-oriented summary, not the formal source of truth. It may lag behind the exact checked-in coverage state. Formal authority lives in:

- the CI gate result
- the checked-in tests and harness registrations
- the relevant evidence under [`docs/test-records/`](test-records/)

## Thesis Submission V1 Release Gate

The curated public release candidate passed a fresh native F Prime generate
and build, UT generate and build, all 74 registered tests, `fprime-util check
--all`, OpenSpec validation, repository governance checks, and the selected
hosted secure-auth, CSP, observability, per-band, Mission Console, and Chapter
5 Route 1/2/3 paths.

This release gate is deliberately hosted. Raspberry Pi, physical UART,
SocketCAN, and target watchdog results remain previously demonstrated,
commit-scoped evidence and are not restated as fresh verification of the
public tag. See the
[`public-thesis-submission-v1` release record](test-records/public-thesis-submission-v1/README.md)
for the exact selection and non-claims.

Current follow-on navigation for the now-closed Chapter 5 route family and its
supporting COMM evidence:

- [`docs/test-records/chapter5-integrated-route-closure-v1/README.md`](test-records/chapter5-integrated-route-closure-v1/README.md)
  is the durable top-level route ledger for Route 1/2/3 status
- [`docs/test-records/uhf-primary-nonquiet-runtime-v1/README.md`](test-records/uhf-primary-nonquiet-runtime-v1/README.md)
  holds the maintained non-quiet UHF benchmark and per-command success/failure ledger
- [`docs/test-records/target-autonomous-uhf-failover-v1/README.md`](test-records/target-autonomous-uhf-failover-v1/README.md)
  holds the maintained detector-triggered failover proof and current bounded
  interpretation

This matrix now distinguishes two different verification obligations:

- **Component Coverage**: classic F' L2 harness coverage for real repository components that derive from `*ComponentBase`
- **Helper/Support Coverage**: direct L1 tests for parsers, stores, providers, framing helpers, and other non-component logic

That split matches the official F' testing model: component behavior is exercised through generated `TesterBase` / `GTestBase` harnesses, while non-component helper logic may be tested directly at the function or helper-class level.

## Legend

- **L1**: logic-focused unit tests
- **L2**: component or contract tests
- **L3**: integration tests
- **L4**: hosted probes, Raspberry Pi target evidence, or manual/system validation evidence

## Capability Matrix

| Capability | L1 | L2 | L3 | L4 | Current weak spots / constrained gaps |
|---|---|---|---|---|---|
| `platform-baseline` | none | none | none | bootstrap / deployment-runtime / RPi target / packaging / autostart / libcsp release-readiness / remote Pi+macOS topology evidence / dual-Pi split-host evidence | mostly deployment/evidence oriented; no direct component-style UT layer |
| `core-system-contracts` | none | `ModeManager`, `HealthMonitor`, `CspBridge` UT | none | `core-system-contracts-v1`, `mode-model-v2-v1` evidence | strong controller-style L2, little standalone L1 |
| `resource-storage` | `storage_scanner_unit_test` | `OBC_Components_StorageHealthBridge_ut_exe`, `storage_health_bridge_contract_test` | `storage_health_bridge_cached_state_integration_test` | packaging / storage-health evidence | retention / cleanup policy remains deferred |
| `eps-subsystem` | none | `EpsBridge` UT | `eps_csp_integration_test` | `eps-subsystem-v1`, `eps-csp-vertical-slice-v1`, `legacy-zmq-retirement-v1` evidence | real EPS hardware remains `Blocked-HW`; retired direct-ZMQ EPS business path is guarded by `check_legacy_zmq_retired.py` |
| `adcs-subsystem` | none | `AdcsBridge` UT | `adcs_csp_integration_test` | `adcs-subsystem-v1`, `adcs-csp-vertical-slice-v1`, `legacy-zmq-retirement-v1` evidence | real ADCS hardware remains `Blocked-HW`; retired direct-ZMQ ADCS business path is guarded by `check_legacy_zmq_retired.py` |
| `comm-subsystem` | `transparent_link_framing_unit_test` | `OBC_Components_CommController_ut_exe`, `OBC_Components_RadioController_ut_exe`, `OBC_Components_UartDriver_ut_exe` | `comm_transport_integration_test` | comm / GDS / UART / transparent / framed / Pi CSP+comm baseline / dual-Pi coexistence / physical serial TT&C file-downlink evidence / hosted secure-command proof / target secure-auth proof / maintained non-quiet UHF runtime and autonomous failover records | vendor-specific protocol, RF, one-GDS aggregation, and broader UHF transport/reliable-transfer widening remain out of scope; see `chapter5-integrated-route-closure-v1`, `uhf-primary-nonquiet-runtime-v1`, and `target-autonomous-uhf-failover-v1` for the current maintained closure and residual non-claims; older OBC-side `/dev/serial0` comm evidence is now historical after GPS reallocation |
| `boot-update` | none | `BootManager` UT | none | boot / target-metadata / packaging / autostart evidence | physical boot-chain, SD switching, and power-loss remain constrained |
| `verification-evidence` | none | none | baseline gate exercises build+test flow indirectly | repository evidence tree, CI gate, reconciliation docs, remote Pi+macOS path evidence, dual-Pi split-host evidence | no line/branch coverage report yet |
| `delivery-workflow` | `check_repo_consistency.py` | none | none | archived governance changes and repo entry docs | governance is verified mainly by docs, CI guardrails, and reconciliation checks, not runtime tests |
| `documentation-governance` | `check_documentation_governance.py` | none | none | `document-realignment-governance-v1` evidence, repo-root entry docs, and current/snapshot doc entrypoints | checker coverage is intentionally scoped to formalized entrypoints rather than every historical Markdown file |
| `ground-ttc-gateway` | none | none | none | hosted gateway-backed COMM omitted-RF TT&C evidence / physical lab-serial bounded TT&C and file-downlink evidence | RF, real radio, target OBC migration, and no-preamble first-byte-clean behavior remain future work |
| `mission-autonomy` | none | `OBC_Components_MissionExecutive_ut_exe` | `mission_executive_policy_integration_test` | historical low-battery / detumbling evidence; `mode-model-v2-v1` retirement evidence | provisional runtime behavior is inactive after `mode-model-v2`; HELL/SAFE policy and FDIR remain future scope |
| `scenario-driven-validation` | none | none | `scenario_bridge_integration_test` | scenario-driven / autonomy evidence | no live STK bridge or comm consumer yet |
| `gps-subsystem` | `gps_support_unit_test` | `OBC_Components_GpsBridge_ut_exe`, `gps_bridge_contract_test` | `gps_bridge_cached_state_integration_test` | `gps-subsystem-v1`, `gps-live-uart-source-v1`, `gps-live-uart-hardening-v1`, `ut-backfill-v1` evidence | first live UART slice proves hardware sentence ingestion and no-fix cached-state updates; follow-up hardening proves blank-line handling, overflow-baud fallback, and probe hygiene; live-sky fix quality, PPS, and higher-level navigation behavior remain out of scope |
| `storage-health` | `storage_scanner_unit_test` | `OBC_Components_StorageHealthBridge_ut_exe`, `storage_health_bridge_contract_test` | `storage_health_bridge_cached_state_integration_test` | `storage-health-v1`, `ut-backfill-v1`, `onboard-state-data-fdp-parity-v1` evidence | cleanup/retention and whole-filesystem free-space/wear modeling remain future scope |
| `onboard-data-products-and-live-beacon` | `onboard_state_data_unit_test`, `onboard_state_snapshot_source_unit_test` | `OBC_Components_OnboardStateMonitor_ut_exe`, `OBC_Components_BeaconPublisher_ut_exe`, `OBC_Components_HkTrendProductProducer_ut_exe`, `OBC_Components_DpCatalogFileDownlinkGate_ut_exe` | hosted live beacon broadcast + official HK DataProducts probe | `onboard-data-products-and-live-beacon-v1`, `onboard-state-data-fdp-parity-v1`, `mode-model-v2-v1` evidence | RF beacon transmission, target-side SD behavior, physical COMM `.fdp` byte-match, CFDP/segment retry, and pass/window scheduling remain future scope |
| `payload-operations` | none | `OBC_Components_PayloadOpsController_ut_exe` | none | payload operation / capture-modes / sensor-register / payload CSP shim / target backend / official hosted payload delivery / governed target node-`5` payload dual-artifact evidence | target real raw-register closure, broad payload size-envelope, and mission scheduler ownership remain out of scope |
| `payload-data-products` | none | `OBC_Components_PayloadOpsController_ut_exe` | none | payload official `.fdp` closure / async-ack target proof / dual-artifact hosted + governed target node-`5` evidence | broad target payload throughput, `full` raw official downlink, and payload-family reliable-transfer widening remain future scope |
| `verification-path-registry` | `check_legacy_zmq_retired.py` | none | none | registry document + guardrail evidence | path selection remains governed by docs and evidence; EPS/ADCS direct-ZMQ retirement and serial-resource ownership have dedicated records |
| `codex-skills` | none | none | none | `codex-skills-v1` evidence and skill quick validation | only a small set of narrow repo-local skills exists today |
| `project-reporting` | none | none | none | `project-reporting-pack-v1` evidence and checked-in reporting package | docs/manual package must be refreshed when the validated baseline changes |

## Component Coverage

| Real F' component | Capability | Classic F' L2 harness | Additional checked-in tests |
|---|---|---|---|
| `AdcsBridge` | `adcs-subsystem` | `OBC_Components_AdcsBridge_ut_exe` | `adcs_csp_integration_test` |
| `BootManager` | `boot-update` | `OBC_Components_BootManager_ut_exe` | target / packaging / autostart evidence |
| `CommController` | `comm-subsystem` | `OBC_Components_CommController_ut_exe` | `comm_transport_integration_test` |
| `CspBridge` | `core-system-contracts` | `OBC_Components_CspBridge_ut_exe` | none |
| `EpsBridge` | `eps-subsystem` | `OBC_Components_EpsBridge_ut_exe` | `eps_csp_integration_test` |
| `GpsBridge` | `gps-subsystem` | `OBC_Components_GpsBridge_ut_exe` | `gps_bridge_contract_test`, `gps_bridge_cached_state_integration_test` |
| `HealthMonitor` | `core-system-contracts` | `OBC_Components_HealthMonitor_ut_exe` | none |
| `MissionExecutive` | `mission-autonomy` | `OBC_Components_MissionExecutive_ut_exe` | `mission_executive_policy_integration_test`; component retained but inactive in the active runtime topology after `mode-model-v2` |
| `ModeManager` | `core-system-contracts` | `OBC_Components_ModeManager_ut_exe` | none |
| `OnboardStateMonitor` | `onboard-data-products-and-live-beacon` | `OBC_Components_OnboardStateMonitor_ut_exe` | `onboard_state_snapshot_source_unit_test`, hosted onboard data-products/live-beacon probe |
| `PayloadOpsController` | `payload-operations`, `payload-data-products` | `OBC_Components_PayloadOpsController_ut_exe` | hosted and governed target payload dual-artifact proof, payload `.fdp` extractor tooling |
| `BeaconPublisher` | `onboard-data-products-and-live-beacon` | `OBC_Components_BeaconPublisher_ut_exe` | `onboard_state_data_unit_test`, hosted onboard data-products/live-beacon probe |
| `HkTrendProductProducer` | `onboard-data-products-and-live-beacon` | `OBC_Components_HkTrendProductProducer_ut_exe` | hosted onboard data-products/live-beacon probe |
| `DpCatalogFileDownlinkGate` | `onboard-data-products-and-live-beacon` | `OBC_Components_DpCatalogFileDownlinkGate_ut_exe` | hosted onboard data-products/live-beacon probe |
| `RadioController` | `comm-subsystem` | `OBC_Components_RadioController_ut_exe` | `comm_transport_integration_test` |
| `StorageHealthBridge` | `storage-health` | `OBC_Components_StorageHealthBridge_ut_exe` | `storage_health_bridge_contract_test`, `storage_health_bridge_cached_state_integration_test` |
| `UartDriver` | `comm-subsystem` | `OBC_Components_UartDriver_ut_exe` | `comm_transport_integration_test` |

No current real component under `OBC/Components/` is allowed to remain without classic F' L2 coverage.

## Helper/Support Coverage

| Helper/support module | Owning component | Capability | Direct L1 coverage |
|---|---|---|---|
| `StorageScanner` | StorageHealthBridge | `storage-health` | `storage_scanner_unit_test` |
| `GpsSource/NmeaParser` | GpsBridge | `gps-subsystem` | `gps_support_unit_test` |
| `TransparentLinkFraming` | CommController | `comm-subsystem` | `transparent_link_framing_unit_test` |
| `OnboardStateData` | OnboardStateMonitor / BeaconPublisher / HkTrendProductProducer | `onboard-data-products-and-live-beacon` | `onboard_state_data_unit_test` |
| `OnboardStateSnapshotSource` | OnboardStateMonitor | `onboard-data-products-and-live-beacon` | `onboard_state_snapshot_source_unit_test` |

## Internal CSP Topology Inventory

| Topology | OBC location | CSP hub + simulator location | Ground path state | Governing evidence |
|---|---|---|---|---|
| Hosted local CSP topology | macOS hosted runtime | same macOS host | optional hosted `OBC -> GDS` and hosted `fprime-cli -> GDS` paths are registered separately | `internal-csp-foundation-v1`, `eps-csp-vertical-slice-v1`, `adcs-csp-vertical-slice-v1` |
| Raspberry Pi local CSP topology | Raspberry Pi | same Raspberry Pi target | external comm UART evidence is separate; GDS target path is separately registered | `rpi-csp-comm-baseline-validation-v1` |
| Raspberry Pi remote macOS CSP topology | Raspberry Pi target | remote macOS host running `csp_zmqproxy`, EPS node `2`, ADCS node `3` | remote target-side `fprime-cli -> GDS -> Pi OBC -> remote subsystem` is a distinct registered path | `rpi-remote-grounded-csp-validation-v1` |
| Dual-Pi split-host CSP topology | `obc.local` Raspberry Pi target | `macOS` hosts `csp_zmqproxy`; `subsystem.local` hosts EPS node `2` and ADCS node `3` | bounded `macOS fprime-cli -> GDS -> obc.local -> subsystem.local` and separate comm coexistence paths are registered distinctly | `dual-pi-subsystem-sim-host-validation-v1` |

## Remaining Gaps

The current unresolved gaps are no longer missing classic component harnesses. They are:

1. future GPS live-sky fix quality, PPS behavior, and higher-level navigation consumers beyond the first direct-UART ingestion slice
2. future storage retention / cleanup policy once that behavior exists
3. future payload broad size-envelope and throughput evidence beyond the current hosted plus bounded target dual-artifact proof
4. future onboard data-products target-side SD behavior, RF beacon transmission, CFDP/segment retry, and pass/window scheduling
5. vendor-specific radio control plane, RF behavior, and CAN-centered comm architecture, which remain future hardware work
6. hardware-constrained or target-constrained paths that already stay explicit through `Blocked-HW` / `Deferred-RPi`
