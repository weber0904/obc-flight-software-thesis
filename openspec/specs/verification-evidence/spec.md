# verification-evidence Specification

## Purpose
Define the layered verification model, evidence requirements, constrained-validation rules, and baseline scenarios for the first version.
## Requirements
### Requirement: Layered Verification Model
The project SHALL organize verification into L1 unit tests, L2 component tests, L3 integration tests, and L4 system or mission-scenario tests, and each layer SHALL have a distinct purpose rather than replacing another layer.

#### Scenario: Integration behavior requires higher-level evidence
- **WHEN** a change modifies cross-component or simulator behavior
- **THEN** that change SHALL include L3 or L4 evidence in addition to any lower-level tests

#### Scenario: Runtime-owner refactor carries both local and target evidence
- **WHEN** a change centralizes deployed CSP runtime ownership and alters how COMM, EPS, or ADCS clients reach the runtime
- **THEN** that change SHALL provide local build or test evidence for the owner-injected topology
- **AND** it SHALL keep any target secure-auth proof separate and reviewable as system-level evidence rather than claiming architecture closure from unit tests alone

### Requirement: Evidence Capture
The project SHALL preserve reviewable evidence for both automated and manual testing, including test identity, execution time, result summary, commands or steps used, expected outcomes, observed outcomes, and the final verdict. Governance or reporting-package changes that do not alter runtime behavior SHALL still capture the checked-in truth sources consulted, the review surfaces updated, and the local verification commands used to keep that reporting package auditable.

#### Scenario: Reporting package evidence is reviewable
- **WHEN** a change adds or refreshes a checked-in project-reporting package
- **THEN** the evidence record SHALL name the repo-truth sources used, the reporting artifacts updated, and the validation commands that kept the package aligned with the formal baseline

### Requirement: Constrained Validation Status
The project SHALL use `Blocked-HW` only for tests blocked by unavailable hardware, cables, or devices, SHALL use `Deferred-RPi` only for tests that require the Raspberry Pi target integration phase, and SHALL update previously deferred records once the relevant Raspberry Pi target validation has been executed. Each constrained test SHALL include the original goal, the reason it is constrained, replacement evidence, summary results, and the condition to clear the constraint.

#### Scenario: Hardware-limited comms test
- **WHEN** real UART validation cannot run because the hardware path is unavailable
- **THEN** the record SHALL use `Blocked-HW` and SHALL include PTY, mock, or equivalent replacement evidence

#### Scenario: Raspberry Pi target execution clears a deferred item
- **WHEN** a previously `Deferred-RPi` behavior is exercised on the Raspberry Pi target and reviewable evidence is captured
- **THEN** the related evidence record SHALL no longer describe that behavior as deferred and SHALL either mark it passed or leave only the remaining constrained sub-cases explicit

### Requirement: First-Version Baseline Scenarios

The first-version verification baseline SHALL include scenarios covering `CSP_INIT` / `CSP_PING`, EPS state changes and low-battery reporting, ADCS detumble and pointing behavior, TCP mock and PTY/UART comms switching, the boot update prepare/verify/activate/confirm-or-rollback sequence, a hosted end-to-end runtime launch, a hosted GDS-connected launch that demonstrates the integrated software-only OBC stack can start and attach to the documented ground adapter path, a Raspberry Pi native build and integrated stack launch, and a target-side boot metadata restart flow that demonstrates persisted confirm/rollback state on actual target hardware.

#### Scenario: Hosted GDS evidence is reviewable
- **WHEN** the hosted ground-integration change completes
- **THEN** reviewers SHALL be able to inspect the GDS launch command, the hosted stack launch command, and the observed connection outcome from the repository evidence tree

#### Scenario: Raspberry Pi target evidence is reviewable
- **WHEN** the Raspberry Pi target-integration change completes
- **THEN** reviewers SHALL be able to inspect the target sync/build/run commands, the observed target launch outcome, and the boot-state restart evidence from the repository documentation tree

### Requirement: Bootstrap Gate Evidence
The bootstrap phase SHALL include a minimum verification gate consisting of `fprime-util generate` and `fprime-util build` executed from the generated project virtual environment, and the results SHALL be captured as reviewable evidence in the repository.

#### Scenario: Bootstrap gate passes
- **WHEN** the F' project tree has been populated successfully
- **THEN** the project SHALL run `generate` and `build` from the generated virtual environment and SHALL record the summary outcomes as bootstrap evidence

### Requirement: Bootstrap Evidence Storage
Bootstrap-phase evidence SHALL be stored under a project documentation path that is discoverable by later implementation changes and review workflows.

#### Scenario: Later changes look up the initial baseline evidence
- **WHEN** a later capability change needs to reference the initial platform bootstrap result
- **THEN** it SHALL be able to find the bootstrap evidence from the repository documentation tree

### Requirement: EPS Implementation Evidence
The first EPS implementation slice SHALL record its build, bridge-unit-test, and host transport integration results under `evidence/records/eps-subsystem-v1/`.

#### Scenario: EPS evidence is reviewable after implementation
- **WHEN** the EPS subsystem change completes
- **THEN** reviewers SHALL be able to inspect the recorded commands, outcomes, and key verification notes from the repository documentation tree

### Requirement: EPS Hardware Gaps Are Explicit
Any EPS behavior that still depends on unavailable real hardware or physical power characterization SHALL be called out explicitly in the EPS evidence record using the `Blocked-HW` status term.

#### Scenario: Host verification does not cover real EPS hardware
- **WHEN** the EPS change completes with simulator and host-side verification only
- **THEN** the remaining real-hardware validation gap SHALL be labeled `Blocked-HW`

### Requirement: ADCS Implementation Evidence
The first ADCS implementation slice SHALL record its build, bridge-unit-test, and host transport integration results under `evidence/records/adcs-subsystem-v1/`.

#### Scenario: ADCS evidence is reviewable after implementation
- **WHEN** the ADCS subsystem change completes
- **THEN** reviewers SHALL be able to inspect the recorded commands, outcomes, and key verification notes from the repository documentation tree

### Requirement: ADCS Hardware Gaps Are Explicit
Any ADCS behavior that still depends on unavailable real hardware, physical closed-loop validation, or real calibration equipment SHALL be called out explicitly in the ADCS evidence record using the `Blocked-HW` status term.

#### Scenario: Host verification does not cover real ADCS hardware
- **WHEN** the ADCS change completes with simulator and host-side verification only
- **THEN** the remaining real-hardware validation gap SHALL be labeled `Blocked-HW`

### Requirement: Boot Implementation Evidence

The first boot/update implementation slice SHALL record its build and `BootManager` unit-test results under `evidence/records/boot-update-v1/`.

#### Scenario: Boot evidence is reviewable after implementation
- **WHEN** the boot/update change completes
- **THEN** reviewers SHALL be able to inspect the recorded commands, outcomes, and key verification notes from the repository documentation tree

### Requirement: Boot Hardware Gaps Are Explicit

Any boot/update behavior that still depends on Raspberry Pi boot-chain integration, real SD-card slot switching, or power-loss testing SHALL be called out explicitly in the boot evidence record using `Deferred-RPi` or `Blocked-HW`, whichever applies.

#### Scenario: Hosted verification stops short of real Raspberry Pi reboot flow
- **WHEN** the boot/update change completes with host-only verification
- **THEN** the remaining boot-chain and physical-media validation gaps SHALL be labeled with the correct constrained status term

### Requirement: Shared Verification Gate Script

The repository SHALL provide a repo-local verification gate script that records per-step logs and a summary while running the baseline build, unit/integration test, and OpenSpec validation commands.

#### Scenario: Local developer reuses the same gate as CI
- **WHEN** a developer wants to run the baseline verification flow before or during a change
- **THEN** they SHALL be able to use the same repo-local script that the CI workflow invokes

### Requirement: Evidence Template

The repository SHALL provide a reusable markdown template for change-level evidence records under `evidence/records/templates/`.

#### Scenario: Later change needs a consistent evidence structure
- **WHEN** a later capability change records automated or constrained-validation evidence
- **THEN** it SHALL be able to start from the checked-in template instead of inventing a new record structure

### Requirement: Verification CI Evidence

The first verification CI slice SHALL record the shared script, workflow coverage, and local verification result under `evidence/records/verification-ci-v1/`.

#### Scenario: CI baseline is reviewable after implementation
- **WHEN** the verification CI change completes
- **THEN** reviewers SHALL be able to inspect the workflow entrypoint, local run summary, and remaining CI scope limits from the repository documentation tree

### Requirement: Raspberry Pi Version Metadata Evidence
The Raspberry Pi target evidence tree SHALL record the observed framework and project version metadata produced by the governed target build path, together with the commands used to generate or inspect that metadata.

#### Scenario: Target version metadata can be reviewed
- **WHEN** the target-version-metadata change completes
- **THEN** reviewers SHALL be able to inspect the target build commands and the resulting framework and project version outputs from the repository evidence tree

### Requirement: Raspberry Pi Packaging Evidence
The Raspberry Pi evidence tree SHALL record the commands and observed results for target bundle creation, target installation into the governed install root, and installed-stack launch from that install root.

#### Scenario: Installed target flow is reviewable
- **WHEN** the Raspberry Pi packaging change completes
- **THEN** reviewers SHALL be able to inspect the bundle metadata, the install command path, and the installed-stack launch evidence from the repository documentation tree

### Requirement: Raspberry Pi Autostart Evidence
The Raspberry Pi evidence tree SHALL record the commands and observed results for governed service installation, service status inspection, target reboot, and post-reboot launch from the installed release.

#### Scenario: Autostart flow is reviewable after reboot
- **WHEN** the Raspberry Pi autostart change completes
- **THEN** reviewers SHALL be able to inspect the service install path, the reboot probe, the post-reboot service status, and the observed installed-release startup evidence from the repository documentation tree

### Requirement: Raspberry Pi Host UART Hardware Evidence
The verification evidence tree SHALL record the commands, device-path selections, observed link behavior, and final verdict for the governed Raspberry Pi to development-host UART/RS485 hardware validation flow that exercises the existing comm stack against the hosted mock-radio peer.

#### Scenario: Hardware UART flow is reviewable
- **WHEN** the Raspberry Pi to host hardware-UART validation change completes
- **THEN** reviewers SHALL be able to inspect the host peer launch path, the Raspberry Pi launch path, the chosen serial device identifiers, and the observed comm outcome from the repository evidence tree

#### Scenario: Remaining hardware gaps stay explicit
- **WHEN** the Raspberry Pi to host hardware-UART path passes but real radio or other external hardware validation is still not available
- **THEN** the evidence SHALL mark only the unresolved sub-cases as `Blocked-HW` instead of treating the full hardware-UART path as blocked

### Requirement: Radio Protocol Adapter Evidence
The verification evidence tree SHALL record which radio protocol adapter was selected during validation, together with the commands and observed regression outcome proving the default adapter still preserves the current comm behavior.

#### Scenario: Default adapter regression is reviewable
- **WHEN** the radio protocol adapter change completes
- **THEN** reviewers SHALL be able to inspect the selected adapter name, the validation commands, and the observed comm result from the repository evidence tree

#### Scenario: Future adapter work stays distinct
- **WHEN** the repository still lacks a real KISS or vendor-specific adapter
- **THEN** the evidence SHALL describe only the default-adapter regression result and SHALL keep future real-radio protocol work explicitly out of scope

### Requirement: Legacy EnduroSat Transparent UART Evidence
The verification evidence tree SHALL record the commands, selected serial devices, UART settings, payload-exchange behavior, and final verdict for the governed legacy EnduroSat transparent-UART validation path.

#### Scenario: Transparent-UART validation is reviewable
- **WHEN** the legacy EnduroSat transparent-UART change completes
- **THEN** reviewers SHALL be able to inspect the host peer launch path, the Raspberry Pi launch path, the selected host and target serial devices, the UART settings used, and the observed payload exchange outcome from the repository evidence tree

### Requirement: Legacy Transparent Scope Boundaries Stay Explicit
The evidence for the legacy EnduroSat transparent-UART path SHALL explicitly distinguish validated transparent payload transport from any still-unimplemented legacy control/configuration behavior or newer `csp-es`-oriented hardware paths.

#### Scenario: Legacy transparent validation does not imply full radio integration
- **WHEN** the legacy transparent-UART path passes
- **THEN** the evidence SHALL mark only the validated transparent data-path behavior as passed and SHALL keep the remaining legacy control-plane and future-generation hardware work explicit as out of scope or `Blocked-HW`, whichever applies

### Requirement: Transparent Link Framing Evidence
The verification evidence tree SHALL record the frame format, representative payload bytes, framing mode selected, commands used, and observed framed payload exchange result for the governed transparent link framing path.

#### Scenario: Framed transparent validation is reviewable
- **WHEN** the transparent link framing change completes
- **THEN** reviewers SHALL be able to inspect the selected host-peer mode, the target launch path, representative binary-safe payloads, and the observed framing/deframing outcome from the repository evidence tree

### Requirement: Framed Transparent Scope Stays Distinct From Ground-Station Integration
The evidence for the framed transparent path SHALL explicitly distinguish transparent link framing from the direct TCP/GDS baseline and from any still-unimplemented ground gateway, RF behavior, or vendor-specific control/configuration work.

#### Scenario: Framed transparent pass does not imply full ground-station integration
- **WHEN** the framed transparent path passes
- **THEN** the evidence SHALL mark only the transparent framed UART/RS485 link behavior as passed and SHALL keep ground-gateway, RF, and vendor-specific control work explicit as future scope or `Blocked-HW`, whichever applies

### Requirement: Framed Transparent Robustness Evidence
The verification evidence tree SHALL record the commands, representative payloads, observed repeated frame behavior, and final verdict for the governed sustained framed transparent UART validation path.

#### Scenario: Sustained framed exchange evidence is reviewable
- **WHEN** the transparent link robustness change completes
- **THEN** reviewers SHALL be able to inspect the framed peer mode, representative repeated payload set, the target launch path, and the observed sustained exchange result from the repository evidence tree

### Requirement: Framed Transparent Reconnect Evidence
The verification evidence tree SHALL record the commands, interruption steps, restart steps, and observed post-restart framed exchange result for the governed host-peer reconnect flow.

#### Scenario: Reconnect recovery is reviewable
- **WHEN** the transparent link robustness change completes
- **THEN** reviewers SHALL be able to inspect how the host peer was interrupted, how it was restarted, and the observed framed exchange result after recovery from the repository evidence tree

### Requirement: Scenario Replay Architecture Evidence
The verification evidence tree SHALL record the scenario timeline used, the simulator-level validation commands, the observed EPS replay behavior, the observed ADCS deployment-rate seeding behavior, and the final verdict for the first scenario-driven simulator architecture slice.

#### Scenario: First scenario replay slice is reviewable
- **WHEN** the first scenario-driven simulator architecture change completes
- **THEN** reviewers SHALL be able to inspect the replay timeline contract, the validation commands, and the observed simulator replay outcomes from the repository evidence tree

### Requirement: Historical Housekeeping Archive Evidence
The verification evidence tree SHALL preserve the capture cadence used,
the archive-slot limits, the observed rotation behavior, the generated index
contents, the downlink commands exercised, and the final verdict for the first
housekeeping archive slice as historical evidence for the now-retired fallback
surface.

#### Scenario: Housekeeping archive slice is reviewable
- **WHEN** the housekeeping archive change completes
- **THEN** reviewers SHALL be able to inspect hosted validation steps covering archive capture, slot rotation, index generation, and commanded index or slot downlink from the repository evidence tree

### Requirement: Evidence Names The Exact Path Under Test
Each evidence record SHALL name the exact validation path under test, including the transport or interface layering that the verdict applies to, instead of describing only the feature name.

#### Scenario: Evidence states the validated path
- **WHEN** a change records manual or integration evidence
- **THEN** the evidence SHALL identify the path being proven, such as direct `OBC -> GDS` TCP connectivity, `fprime-cli -> GDS` command dispatch, or transparent UART framing

### Requirement: Evidence Distinguishes Adjacent Paths
When two neighboring paths could be confused with one another, the evidence SHALL explicitly say which neighboring paths are not covered by the current verdict.

#### Scenario: GDS command-path evidence distinguishes TCP adapter coverage
- **WHEN** a change validates a `fprime-cli -> GDS` command path
- **THEN** the evidence SHALL state whether direct `OBC -> GDS` TCP adapter connectivity is merely a prerequisite or is also being revalidated in the same record

### Requirement: Evidence Keeps Direct GDS And Omitted-RF TT&C Separate
The verification evidence tree SHALL distinguish the stock direct `GDS -> TCP -> OBC` development path from any future omitted-RF TT&C path that traverses the comm subsystem or a ground-side gateway.

#### Scenario: Future TT&C evidence does not overwrite direct GDS meaning
- **WHEN** a later change captures evidence for a gateway-backed TT&C path
- **THEN** that evidence SHALL state that the direct TCP GDS baseline is a separate neighboring path unless it is also explicitly revalidated

#### Scenario: Gateway-backed COMM evidence identifies reused and newly proven paths
- **WHEN** the repository records the first gateway-backed COMM TT&C evidence
- **THEN** that record SHALL say which direct GDS or external comm baselines are merely reused prerequisites
- **AND** it SHALL identify the gateway-backed COMM path itself as the newly proven verdict boundary

### Requirement: Evidence Separates Spacecraft Internal Bus Claims From Lab Ingress Claims
The verification evidence tree SHALL distinguish lab-side omitted-RF ingress behavior from spacecraft-side internal bus behavior when the two are exercised in the same overall architecture.

#### Scenario: Lab UART ingress does not imply internal CAN FD proof
- **WHEN** a later change validates subsystem-side UART ingress plus spacecraft-side CSP traffic
- **THEN** the evidence SHALL name which part of the verdict applies to the lab ingress and which part applies to the internal spacecraft-side bus

#### Scenario: Gateway-backed COMM evidence names both ingress and internal-bus segments
- **WHEN** the repository records the first gateway-backed COMM TT&C evidence
- **THEN** that record SHALL explicitly identify the lab-side serial ingress segment and the internal COMM-to-OBC CSP segment as separate parts of the overall architecture instead of collapsing them into one transport claim

### Requirement: Evidence Names GPS As A Separate Direct Sensor Path
Future evidence involving live GPS hardware SHALL identify GPS as a direct sensor path distinct from both the comm TT&C path and the shared internal subsystem bus.

#### Scenario: GPS evidence is not conflated with comm or shared bus work
- **WHEN** a later change captures live GPS UART evidence
- **THEN** that record SHALL identify the direct GPS path explicitly and SHALL NOT reuse comm or internal-bus evidence labels

### Requirement: Internal CSP Foundation Path Is Tracked Separately
The verification evidence tree SHALL treat the hosted internal libcsp foundation path as distinct from the direct `OBC -> GDS` ground path and the external comm path, and the repository SHALL name that internal path explicitly in both the verification-path registry and the governing evidence record.

#### Scenario: Foundation proof does not imply GDS or external comm proof
- **WHEN** the hosted libcsp foundation path passes
- **THEN** the evidence SHALL mark only hosted internal CSP runtime bring-up and peer reachability as passed and SHALL keep ground-path and external-comm claims separate

#### Scenario: Hosted internal CSP evidence is reviewable
- **WHEN** the internal CSP foundation change completes
- **THEN** reviewers SHALL be able to inspect the hub or proxy launch command, the hosted OBC launch command, the hosted peer command, and the observed ping or raw-send outcome from the repository evidence tree

### Requirement: EPS CSP Vertical Slice Evidence Is Tracked Separately
The verification evidence tree SHALL treat the hosted EPS internal libcsp business path as distinct from the foundation-only CSP path, the direct `OBC -> GDS` ground path, the external comm path, and the still-legacy ADCS path.

#### Scenario: EPS CSP evidence is reviewable
- **WHEN** the EPS CSP vertical slice passes
- **THEN** the evidence SHALL identify the hub/proxy launch, OBC/client node id, EPS simulator node id, EPS application service ports, exercised EPS services, observed replies, and paths that remain unproven

#### Scenario: EPS CSP evidence does not imply ADCS migration
- **WHEN** the EPS CSP vertical slice passes
- **THEN** the evidence SHALL mark only EPS business traffic as migrated and SHALL keep ADCS migration explicit as future scope

### Requirement: ADCS CSP Vertical Slice Evidence Is Tracked Separately
The verification evidence tree SHALL treat the hosted ADCS internal libcsp business path as distinct from the foundation-only CSP path, the direct `OBC -> GDS` ground path, the external comm path, the GPS path, and real ADCS hardware validation.

#### Scenario: ADCS CSP evidence is reviewable
- **WHEN** the ADCS CSP vertical slice passes
- **THEN** the evidence SHALL identify the hub/proxy launch, OBC/client node id, ADCS simulator node id, ADCS application service ports, exercised ADCS services, observed replies, and paths that remain unproven

### Requirement: Carrier Abstraction Evidence Keeps Development Carrier Separate From Future Hardware Buses
The verification evidence tree SHALL record the commands, selected carrier or backend, observed libcsp startup outcome, and final verdict for any internal CSP carrier-abstraction change, and that evidence SHALL keep the active development carrier separate from future hardware-bus validation claims.

#### Scenario: ZMQHUB refactor evidence stays bounded
- **WHEN** the repository validates a carrier-abstraction refactor while `zmqhub` remains the only active carrier
- **THEN** the evidence SHALL identify `zmqhub` as the validated carrier, cite the rerun CSP/EPS/ADCS checks, and SHALL NOT describe the result as proof of future `UART`, `CAN`, `RS485`, or other physical-bus behavior

### Requirement: Remote Pi-To-macOS CSP Evidence Is Recorded Separately From Ground Commands
The verification evidence tree SHALL record the commands, host or IP settings, node ids, observed Pi-side CSP reachability, and final verdict for the remote `Pi OBC -> macOS EPS/ADCS simulator` internal CSP path as a distinct result from any ground-driven command path.

#### Scenario: Remote internal CSP proof stays distinct from GDS command proof
- **WHEN** the repository validates the remote Pi-to-macOS simulator topology
- **THEN** the evidence SHALL identify the remote CSP host, CSP ports, node ids, and Pi-side observed `csp ping`, `eps get`, and `adcs get` results without treating that section as proof of `fprime-cli -> GDS` command dispatch

### Requirement: Target-Side GDS-Driven Subsystem Command Evidence Is Reviewable
The verification evidence tree SHALL record the commands, GDS ports, observed `fprime-cli` dispatch, Pi-side observed subsystem state changes, and final verdict for the target-side `fprime-cli -> GDS -> Pi OBC -> remote EPS/ADCS simulator` command path.

#### Scenario: Remote ground-driven subsystem command path is reviewable
- **WHEN** the remote Pi+macOS GDS-driven subsystem probe completes
- **THEN** reviewers SHALL be able to inspect the headless GDS launch command, the `fprime-cli` commands used for EPS and ADCS, and the Pi-side observed `eps get` and `adcs get` output that confirms the commanded state changes

#### Scenario: Adjacent paths remain out of scope
- **WHEN** the remote Pi+macOS GDS-driven subsystem probe passes
- **THEN** the evidence SHALL keep direct hosted-only GDS validation, external comm, GPS live UART, RF behavior, and future physical-bus validation separate

#### Scenario: ADCS CSP evidence does not imply ground or hardware validation
- **WHEN** the ADCS CSP vertical slice passes
- **THEN** the evidence SHALL mark only hosted ADCS business traffic as migrated and SHALL keep GDS, external comm, GPS, and real ADCS hardware paths separate

### Requirement: Legacy Direct-ZMQ Retirement Evidence
The verification evidence tree SHALL record the cleanup result that retires the legacy EPS and ADCS direct ZMQ request/reply business path from active source while preserving libcsp's hosted ZMQHUB-backed substrate as the current internal CSP transport.

#### Scenario: Retirement evidence is reviewable
- **WHEN** the legacy direct-ZMQ retirement change completes
- **THEN** reviewers SHALL be able to inspect the source cleanup summary, regression checker output, EPS CSP smoke, ADCS CSP smoke, scenario bridge regression, OpenSpec validation, and full baseline gate evidence

#### Scenario: Retirement evidence does not rewrite history
- **WHEN** reviewers inspect archived legacy evidence
- **THEN** the repository SHALL preserve historical records as historical context while the current verification matrix and path registry identify the active libcsp-first baseline

### Requirement: Evidence Cites The Governing Baseline
If a change reuses an already proven validation path, its evidence SHALL cite the governing repository baseline or verification-path registry entry instead of assuming the reader will infer that reuse from framework conventions alone.

#### Scenario: Change reuses an established path
- **WHEN** a later change depends on a path already proven by an earlier archived slice
- **THEN** the evidence SHALL cite the archived evidence or verification-path registry entry that establishes that path before building new conclusions on top of it

### Requirement: Debugging Lessons Preserve Path-Selection Mistakes
When a verification failure was caused by choosing the wrong path or by conflating adjacent paths, the repository debugging-lessons document SHALL record that mistake and the corrected path-selection rule.

#### Scenario: Wrong-path lesson becomes repository guidance
- **WHEN** a verification effort discovers that a failing result was caused by using the wrong command or transport path
- **THEN** the debugging-lessons record SHALL describe the mistaken assumption, the corrected path distinction, and the future rule that prevents the same error

### Requirement: GPS Hosted Validation Evidence
The verification evidence tree SHALL record the source mode, representative supported GPS sentences, malformed or no-fix inputs, observed telemetry or event behavior, and final verdict for the first GPS hosted validation slice.

#### Scenario: GPS hosted slice is reviewable
- **WHEN** the first GPS subsystem change completes
- **THEN** reviewers SHALL be able to inspect the hosted validation steps, the fake or replay GPS inputs used, and the observed GPS cached-state outcomes from the repository evidence tree

### Requirement: GPS Hardware Scope Boundary Remains Explicit
The first GPS evidence SHALL explicitly distinguish hosted fake/replay validation from any still-unimplemented Raspberry Pi UART hardware bring-up or live-sky reception validation.

#### Scenario: Hosted GPS validation does not imply target hardware integration
- **WHEN** the first GPS hosted validation passes
- **THEN** the evidence SHALL mark only the fake or replay-backed hosted GPS path as passed and SHALL keep Raspberry Pi UART wiring, live receiver integration, and live-fix validation explicit as future scope or `Blocked-HW`, whichever applies

### Requirement: Storage Health Hosted Validation Evidence
The verification evidence tree SHALL record the runtime roots used, the representative synthetic root contents, the configured warning condition, the observed storage telemetry or event behavior, and the final verdict for the first hosted storage health slice.

#### Scenario: Storage health hosted slice is reviewable
- **WHEN** the first storage health change completes
- **THEN** reviewers SHALL be able to inspect the hosted validation steps, the synthetic runtime-root contents, and the observed storage cached-state outcomes from the repository evidence tree

### Requirement: Storage Hardware Scope Boundary Remains Explicit
The first storage health evidence SHALL explicitly distinguish hosted governed-root validation from any still-unimplemented Raspberry Pi target disk-health or retention-policy validation.

#### Scenario: Hosted storage validation does not imply target disk coverage
- **WHEN** the first hosted storage health validation passes
- **THEN** the evidence SHALL mark only the hosted governed-root storage path as passed and SHALL keep Raspberry Pi target disk behavior, cleanup policy, and retention policy explicit as future scope or `Deferred-RPi`, whichever applies

### Requirement: Formal Verification Matrix
The repository SHALL provide a checked-in verification matrix that summarizes each current formal capability's L1, L2, L3, and L4 coverage together with any explicitly constrained or currently weak areas.

#### Scenario: Capability coverage is reviewable in one place
- **WHEN** a reviewer asks what verification currently exists for a capability
- **THEN** the repository SHALL provide one matrix that identifies the capability's current L1, L2, L3, and L4 coverage and its known constrained gaps

#### Scenario: Weak coverage areas stay explicit
- **WHEN** a capability relies more heavily on integration tests, hosted probes, or constrained evidence than on component-level tests
- **THEN** the matrix SHALL describe that unevenness explicitly instead of implying the capability has balanced coverage at every layer

#### Scenario: Matrix distinguishes real components from helper support
- **WHEN** the matrix summarizes repository coverage
- **THEN** it SHALL distinguish real F' component coverage from helper or support-module coverage instead of mixing those categories into one undifferentiated component list

### Requirement: Repository Verification Inventory Report
The repository SHALL provide a repo-local inventory/report tool that aggregates the currently registered tests, baseline verification gate entrypoint, hosted probe scripts, and checked-in evidence directories from the repository state.

#### Scenario: Inventory report reflects registered tests and probes
- **WHEN** a developer runs the verification inventory tool
- **THEN** it SHALL report the repository's registered UT/IT entrypoints, baseline verification gate script, hosted probe scripts, and evidence directories using the current checked-in sources

#### Scenario: Inventory report marks classic component L2 coverage explicitly
- **WHEN** the inventory report scans the repository components
- **THEN** it SHALL identify which real F' components have classic `register_fprime_ut()` harness coverage and which helper/support modules only provide direct logic tests

### Requirement: Targeted Automated Test Backfill For Uneven Slices
The repository SHALL backfill additional automated tests when the formal verification matrix identifies a current capability area as materially uneven across L1/L2/L3/L4 and the affected slice exposes a practical logic or contract seam.

#### Scenario: Weak slices receive the smallest durable automated test
- **WHEN** a current capability area is verified mainly by integration tests, hosted probes, or constrained evidence
- **AND** the implementation exposes a stable contract or logic seam
- **THEN** the repository SHALL add the smallest durable automated test that strengthens reviewability without changing flight behavior

#### Scenario: Classic F prime harnesses are optional for later slices
- **WHEN** a later-phase slice does not map cleanly onto the earlier classic F' component tester pattern
- **THEN** the repository MAY use a smaller contract or pure logic test instead of forcing `register_fprime_ut()`
- **AND** the resulting test SHALL still be inventoried and classified in the formal verification matrix

### Requirement: Remaining Exceptions Stay Explicit
The repository SHALL keep the verification matrix explicit when a currently weak area still relies primarily on integration tests, hosted probes, or constrained evidence after a targeted backfill pass.

#### Scenario: No clean unit seam exists yet
- **WHEN** a targeted area still lacks a clean standalone seam after review
- **THEN** the matrix SHALL keep that area listed as an explicit remaining weak spot or constrained path instead of implying balanced coverage

### Requirement: Component And Helper Coverage Stay Distinct
The verification model SHALL distinguish classic component L2 coverage for real F' components from plain L1 helper coverage for support logic, and the repository SHALL keep those layers reviewable as separate categories.

#### Scenario: Helper tests do not satisfy missing component coverage
- **WHEN** a real repository component lacks classic F' L2 coverage but helper tests exist for its supporting logic
- **THEN** the repository SHALL still report the component L2 gap explicitly instead of treating the helper tests as a complete substitute

#### Scenario: Helper modules keep direct logic tests
- **WHEN** a non-component helper such as a parser, store, framing helper, or topology provider contains nontrivial behavior
- **THEN** the repository SHALL keep direct L1 tests for that helper even when the owning component also has classic F' L2 coverage

### Requirement: Later Real Components Recover The Classic L2 Layer
When later repository work has introduced a real F' component without the repository's classic component harness, the recovery change SHALL add that harness while preserving the existing helper and integration evidence for the same slice.

#### Scenario: Classic harness backfill keeps the helper layer
- **WHEN** the repository backfills classic F' L2 coverage for a component such as `GpsBridge` or `StorageHealthBridge`
- **THEN** the change SHALL retain the existing helper-level tests and integration evidence instead of replacing them with only the classic harness

#### Scenario: Classic harness backfill is reviewable
- **WHEN** the classic harness backfill change completes
- **THEN** reviewers SHALL be able to inspect the focused component tester commands and outcomes together with the retained helper and integration layers from the repository evidence tree

### Requirement: Libcsp Mainline Release Evidence
The verification evidence tree SHALL record the local verification commands and result summary used to qualify the libcsp integration base for merge back to `main`.

#### Scenario: Release readiness evidence is reviewable
- **WHEN** the libcsp integration base is ready for a mainline PR
- **THEN** the evidence SHALL identify the branch, the included archived CSP migration changes, the legacy-ZMQ checker result, OpenSpec validation result, and shared baseline gate verdict

#### Scenario: Release evidence does not over-claim hardware coverage
- **WHEN** the release-readiness evidence passes
- **THEN** it SHALL still leave Raspberry Pi CSP execution, external comm UART hardware, GPS live UART, and real EPS/ADCS hardware as separately governed paths

### Requirement: Raspberry Pi CSP And Comm Baseline Evidence
The verification evidence tree SHALL record the commands, target device paths, CSP node ids, UART settings, observed CSP reachability, observed EPS/ADCS state, observed comm/radio response, and final verdict for the combined Raspberry Pi CSP + external comm baseline validation.

#### Scenario: Target CSP and comm evidence is reviewable
- **WHEN** the combined target probe completes
- **THEN** the evidence SHALL identify the host serial device, target serial device, target runtime root, CSP hub ports, EPS/ADCS node ids, and observed command output that proves the scoped paths

#### Scenario: Adjacent paths remain out of scope
- **WHEN** the combined target probe passes
- **THEN** the evidence SHALL keep GDS, GPS live UART, real EPS/ADCS hardware, RF behavior, and vendor radio control semantics separate

### Requirement: Live GPS UART Evidence Is Reviewable
The verification evidence tree SHALL record the commands, target serial device, baudrate, observed source mode, observed sentence-ingestion state change, and final verdict for the governed `obc.local` live GPS UART path.

#### Scenario: First hardware GPS proof stays bounded
- **WHEN** the governed live GPS probe passes
- **THEN** the evidence SHALL show that real hardware sentences entered `GpsBridge`, advanced accepted-sentence state, and updated cached runtime fields without claiming live-sky fix quality, PPS behavior, RF behavior, or comm migration coverage

### Requirement: Historical OBC-Side Serial Comm Evidence Is Not Reused As Current GPS Or Comm Baseline
When the repository realigns `obc.local:/dev/serial0` to GPS, the evidence tree SHALL keep the older OBC-side serial comm records as historical proof and SHALL not reuse them as if they still represent the current target baseline.

#### Scenario: New GPS evidence does not erase old comm history
- **WHEN** the new GPS live UART evidence is added
- **THEN** reviewers SHALL still be able to inspect the old comm evidence, but the new record and related docs SHALL state that those OBC-side serial comm results are historical rather than the active target baseline

### Requirement: Three-Host Split-Host Internal CSP Evidence Is Reviewable
The verification evidence tree SHALL record the host roles, SSH targets, workspace roots, CSP ports, node ids, observed `csp ping`, and observed `eps get` / `adcs get` output for the governed three-host split-host internal CSP path.

#### Scenario: Three-host internal CSP proof names each host role
- **WHEN** the governed three-host split-host probe completes
- **THEN** the evidence SHALL identify `macOS` as the hub host, `obc.local` as the OBC host, `subsystem.local` as the simulator host, and SHALL keep that proof separate from adjacent GDS or external-comm claims

### Requirement: Three-Host GDS-Driven Subsystem Command Evidence Is Reviewable
The verification evidence tree SHALL record the GDS launch command, `fprime-cli` commands, observed dispatch, and `obc.local` state readback for the bounded `macOS fprime-cli -> GDS -> obc.local -> subsystem.local` subsystem command path.

#### Scenario: Three-host ground-driven subsystem commands are auditable
- **WHEN** the main three-host split-host probe completes
- **THEN** reviewers SHALL be able to inspect the bounded EPS and ADCS commands together with the Pi-side observed state changes that confirm those commands traversed the governed GDS and internal CSP layers

### Requirement: Three-Host Comm Coexistence Evidence Is Reviewable
The verification evidence tree SHALL record the serial-device settings, host-side mock-radio launch, subsystem-host launch, observed remote CSP reachability, observed radio command exchange, and final verdict for the three-host coexistence path.

#### Scenario: Comm coexistence proof stays scoped
- **WHEN** the three-host coexistence probe passes
- **THEN** the evidence SHALL identify the host serial device, `obc.local` comm device, remote subsystem host, and observed `radio enable on` / `uart raw STATUS` results while keeping RF behavior, transparent/framed UART paths, and future physical carriers out of scope

### Requirement: Verification Baseline Records Physical-Bus Scope Carefully
When the repository adds a physical internal CSP carrier proof, the governing evidence SHALL distinguish bus capability, logical-node behavior, and physical-controller limits instead of collapsing those into one claim.

#### Scenario: First SocketCAN evidence distinguishes bus capability from frame usage
- **WHEN** the first governed CAN FD-capable SocketCAN evidence is recorded
- **THEN** that evidence SHALL distinguish between the bus being configured as CAN FD-capable and the actual observed libcsp frame behavior on the wire

#### Scenario: First SocketCAN evidence records CAN health and mapping
- **WHEN** the first governed CAN FD-capable SocketCAN evidence is recorded
- **THEN** it SHALL include `parentdev` mapping, active/reserved channel identification, pre/post CAN statistics, and enough traffic capture or idle capture to support both the active-bus path and reserved-channel isolation claims

### Requirement: Gateway-Backed COMM TT&C Evidence Is Reviewable
The verification evidence tree SHALL record the commands, launcher scripts, selected transport endpoints, observed bounded traffic, and final verdict for the first gateway-backed COMM omitted-RF TT&C path.

#### Scenario: First gateway-backed COMM evidence is reviewable
- **WHEN** the first gateway-backed COMM TT&C change completes
- **THEN** reviewers SHALL be able to inspect the ground-side GDS launch path, the gateway launch path, the subsystem-side COMM node launch path, the OBC launch path, and the observed bounded `command`, `event`, and `telemetry` behavior from the repository evidence tree

### Requirement: Subsystem COMM UART Preflight Evidence Is Reviewable
The verification evidence tree SHALL record the host serial device, subsystem serial device, baudrate, launch commands, host peer log, subsystem probe log, observed bounded exchange, final verdict, and any known reverse-direction diagnostic limitations for the subsystem-side COMM UART preflight.

#### Scenario: Subsystem UART preflight evidence separates adjacent paths
- **WHEN** the subsystem COMM UART preflight change completes
- **THEN** reviewers SHALL be able to inspect which macOS and `subsystem.local` endpoints were used
- **AND** the evidence SHALL identify the newly proven path as macOS-initiated physical serial request/reply byte exchange with `subsystem.local`
- **AND** the evidence SHALL state whether clean subsystem-origin cold-first traffic into a passive macOS receiver was proven or remains unproven
- **AND** the evidence SHALL keep historical OBC-side UART comm, hosted PTY gateway, gateway-backed TT&C, RF, file/downlink, target OBC, and COMM shared CAN FD behavior out of the verdict boundary

### Requirement: Lab Serial Acquisition Evidence
The verification evidence tree SHALL record the commands, endpoints, baudrate, acquisition framing settings, observed decoded frames, raw byte counts, and final verdict for the subsystem-origin lab serial acquisition probe.

#### Scenario: Acquisition evidence is reviewable
- **WHEN** the lab serial acquisition probe completes
- **THEN** reviewers SHALL be able to inspect the macOS receiver command, subsystem sender command, selected serial devices, baudrate, frame count, decoded payload count, and any raw-byte diagnostic summary

#### Scenario: Failed TT&C attempt remains diagnostic
- **WHEN** the acquisition change records the prior guarded TT&C attempt
- **THEN** the evidence SHALL label that attempt as diagnostic context only
- **AND** it SHALL NOT register or imply a successful physical lab serial TT&C path

### Requirement: Lab Serial Ingress Evidence Is Staged
The verification evidence tree SHALL record physical lab serial ingress evidence with separate Stage 0 prerequisite, Stage 1 uplink ingress, and Stage 2 downlink/full-TT&C verdicts.

#### Scenario: Uplink ingress evidence is reviewable
- **WHEN** the staged lab serial ingress probe completes Stage 1
- **THEN** reviewers SHALL be able to inspect the host serial device, subsystem serial device, baudrate, GDS ports, CSP hub ports, gateway launch, remote COMM node launch, OBC launch mode, commands issued, and OBC readback used for the verdict
- **AND** when the probe uses a gateway serial acquisition preamble or bounded command retries, the evidence SHALL record the preamble settings and retry bound

#### Scenario: Downlink evidence is not implied by uplink
- **WHEN** Stage 1 passes but Stage 2 fails or is inconclusive
- **THEN** the evidence SHALL state that only physical lab serial uplink ingress was proven
- **AND** it SHALL keep `fprime-cli events`, `fprime-cli channels`, file/downlink, RF, target OBC, and COMM shared CAN FD outside the proven verdict

#### Scenario: Full TT&C evidence requires both directions
- **WHEN** the evidence describes the physical lab serial result as bounded TT&C
- **THEN** it SHALL include both OBC command readback and ground-side event/telemetry observations

### Requirement: Lab Serial Downlink Evidence Is Reviewable
The verification evidence tree SHALL record physical lab serial downlink evidence with a distinct verdict from the prior physical lab serial uplink-ingress-only record.

#### Scenario: Downlink evidence includes the physical path and prerequisite readback
- **WHEN** the physical lab serial downlink probe passes
- **THEN** reviewers SHALL be able to inspect the host serial device, subsystem serial device, baudrate, GDS ports, CSP hub ports, gateway launch, remote COMM node launch, OBC launch mode, commands issued, and OBC readback used as the prerequisite for the verdict

#### Scenario: Downlink evidence includes event and telemetry observations
- **WHEN** the evidence describes bounded physical lab serial TT&C
- **THEN** it SHALL include ground-side command event observations from `fprime-cli events`
- **AND** it SHALL include ground-side telemetry observations from `fprime-cli channels` for `GROUND_LINK_TX_BYTES`

#### Scenario: Downlink evidence keeps adjacent paths separate
- **WHEN** the physical lab serial downlink evidence is recorded
- **THEN** it SHALL cite the prior uplink ingress record as a reused/supporting boundary rather than replacing it
- **AND** it SHALL keep file/downlink, RF, target OBC, no-preamble first-byte-clean behavior, and COMM shared CAN FD outside the proven verdict

### Requirement: COMM File Downlink Evidence Is Reviewable
The verification evidence tree SHALL record the commands, endpoints, archive slot identifiers, received file paths, source file paths, byte-comparison results, and final verdict for COMM TT&C file/downlink validation.

#### Scenario: Hosted regression evidence is captured
- **WHEN** the hosted PTY COMM file/downlink probe passes
- **THEN** the evidence SHALL record the hosted GDS ports, runtime root, GDS file-storage directory, command sequence, selected archive files, and byte-comparison verdict

#### Scenario: Physical file evidence includes TT&C prerequisite
- **WHEN** the physical lab serial COMM file/downlink probe passes
- **THEN** the evidence SHALL include the command/event/channel TT&C prerequisite result before the file/downlink verdict
- **AND** it SHALL record host serial device, subsystem serial device, baudrate, CSP hub ports, GDS ports, preamble settings, and command pacing settings

#### Scenario: File comparison evidence is explicit
- **WHEN** the evidence describes a file/downlink PASS
- **THEN** it SHALL identify the downlinked official `.fdp` files selected by `DpCatalog`
- **AND** it SHALL state that each received file was compared byte-for-byte against the OBC runtime source file

#### Scenario: Evidence keeps adjacent paths separate
- **WHEN** COMM file/downlink evidence is recorded
- **THEN** it SHALL cite the prior bounded command/event/channel TT&C evidence as a reused prerequisite
- **AND** it SHALL keep RF, target OBC migration, no-preamble first-byte-clean behavior, arbitrary file downlink, archive wraparound, ScenarioBridge, and COMM shared CAN FD outside the proven verdict

### Requirement: COMM SocketCAN TT&C Evidence
The repository SHALL maintain an evidence record for COMM SocketCAN TT&C that captures corrected CAN bring-up, physical interface health, gateway-backed TT&C observations, and excluded adjacent paths.

#### Scenario: Evidence records corrected Stage 0
- **WHEN** the change records Stage 0 CAN health evidence
- **THEN** it SHALL state that the probe brought up `obc.local:can0` and `subsystem.local:can0` before checking EPS/ADCS reachability
- **AND** it SHALL preserve the distinction between setup failures and physical bus failures

#### Scenario: Evidence records active CAN interfaces
- **WHEN** the formal COMM SocketCAN TT&C probe passes
- **THEN** the evidence SHALL record `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1` as active CAN interfaces
- **AND** it SHALL include pre/post health or statistics showing `ERROR-ACTIVE` and no `bus-off`

#### Scenario: Evidence records TT&C observations
- **WHEN** bounded TT&C is claimed
- **THEN** the evidence SHALL include OBC command readback, fprime-cli command events, `GROUND_LINK_TX_BYTES`, and non-empty CAN capture evidence

#### Scenario: Evidence excludes adjacent paths
- **WHEN** COMM SocketCAN TT&C evidence is recorded
- **THEN** it SHALL keep file/downlink, RF, no-preamble serial behavior, dual-bus redundancy, and independent COMM hardware outside the proven verdict

### Requirement: COMM SocketCAN File Downlink Evidence
The repository SHALL maintain an evidence record for COMM SocketCAN file/downlink that captures TT&C prerequisites, archive slot identifiers, received file paths, source snapshots, byte-comparison results, CAN health, and excluded adjacent paths.

#### Scenario: Evidence records SocketCAN TT&C prerequisite
- **WHEN** the formal COMM SocketCAN file/downlink probe passes
- **THEN** the evidence SHALL first record the command/event/channel TT&C prerequisite result over the same SocketCAN-backed COMM path
- **AND** it SHALL include OBC command readback, fprime-cli command events, `GROUND_LINK_TX_BYTES`, and active CAN capture evidence

#### Scenario: Evidence records file comparisons
- **WHEN** SocketCAN-backed file/downlink is claimed
- **THEN** the evidence SHALL identify the downlinked official `.fdp` files selected by `DpCatalog`
- **AND** it SHALL identify the target OBC source snapshot and GDS received path for each file
- **AND** it SHALL state that each received file was compared byte-for-byte against the source snapshot

#### Scenario: Evidence records constrained-link mitigations
- **WHEN** COMM SocketCAN file/downlink evidence is recorded
- **THEN** it SHALL record the bounded F' file packet size used for the formal run
- **AND** it SHALL record whether bounded file/downlink command retries were needed before the final byte-match
- **AND** it SHALL distinguish bounded whole-command retries from missing-packet retransmission and SHALL NOT claim packet-loss recovery unless that behavior is separately implemented and proven

#### Scenario: Evidence records active transport health
- **WHEN** COMM SocketCAN file/downlink evidence is recorded
- **THEN** it SHALL record host serial device, subsystem serial device, baudrate, preamble settings, GDS ports, GDS file-storage directory, `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1`
- **AND** it SHALL include CAN health showing `ERROR-ACTIVE` and no `bus-off` on active CAN interfaces

#### Scenario: Evidence excludes adjacent paths
- **WHEN** COMM SocketCAN file/downlink evidence is recorded
- **THEN** it SHALL cite the prior physical COMM SocketCAN command/event/channel TT&C evidence as a reused prerequisite
- **AND** it SHALL keep arbitrary file downlink, RF, no-preamble serial behavior, archive wraparound, ScenarioBridge/pass automation, dual-bus redundancy, independent COMM hardware, and packet-loss-tolerant reliable file transfer outside the proven verdict

### Requirement: Lab Target Operational Evidence
The verification evidence baseline SHALL require service-managed evidence before
the lab target COMM CSP path is treated as reusable operational baseline. The
historical lab target evidence may remain HK-fallback based, but current or
future mission-history file/downlink claims over that path SHALL cite official
`.fdp` / `DpCatalog` byte-match evidence instead of treating the historical HK
fallback byte matches as current proof.

#### Scenario: Evidence records OBC service migration and reboot
- **WHEN** the lab target operational probe passes
- **THEN** the evidence SHALL record the OBC package release id, installed release pointer, `obc-comm-csp-stack.service` enabled/active state, the older `obc-installed-stack.service` disabled/stopped state, and an `obc.local` reboot followed by autostart from the installed release

#### Scenario: Evidence records subsystem service boundaries
- **WHEN** the subsystem lab services are part of the verdict
- **THEN** the evidence SHALL record the relevant subsystem aggregate target state (`subsystem-sband-csp-stack.target` or `subsystem-uhf-csp-stack.target`) plus the separate EPS, ADCS, and COMM service states and journal summaries

#### Scenario: Evidence records lab CAN provisioning
- **WHEN** CAN interface state is part of the verdict
- **THEN** the evidence SHALL record the CAN oneshot service states, configured timing, interface state, and absence of bus-off on `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1`
- **AND** it SHALL state that oneshot provisioning is a lab/development helper rather than final flight OS provisioning

#### Scenario: Evidence records ground and E2E observations
- **WHEN** the full lab path is accepted
- **THEN** the evidence SHALL record ground launcher settings, command/event/channel observations, `GROUND_LINK_TX_BYTES`, OBC readback for bounded EPS and ADCS commands, and live GPS UART source-mode observations
- **AND** any current mission-history file/downlink claim SHALL additionally cite official `.fdp` / `DpCatalog` byte matches for the path under review

#### Scenario: Evidence excludes adjacent futures
- **WHEN** the lab target operational evidence is recorded
- **THEN** it SHALL explicitly exclude final flight deployment, RF behavior, no-preamble first-byte-clean behavior, reliable retransmission, arbitrary onboard file downlink, ScenarioBridge/pass automation, and final OS CAN provisioning

### Requirement: Node-5 Residual Observability Evidence Distinguishes Truth, Reviewable Proof, And Diagnostics

The verification evidence baseline SHALL record the final node-`5` residual
observability classification so resource truth, reviewable proof surfaces, and
diagnostics-only residuals are not left ambiguous.

#### Scenario: Evidence records the formal resource truth boundary
- **WHEN** the residual cleanup evidence is written
- **THEN** it SHALL identify `SYS_CPU_USAGE`, `SYS_MEM_RSS_MB`,
  `SYS_RESOURCE_DEGRADED`, and `SYS_LOW_MEMORY` as the formal node-`5`
  resource keep-live truth
- **AND** it SHALL explicitly state that `SYS_MEM_RSS_MB` is current resident
  memory and that `SYS_RESOURCE_DEGRADED` / `SYS_LOW_MEMORY` are
  threshold-crossing warnings
- **AND** it SHALL explicitly state that `SystemResources.*` remains
  supplemental diagnostics-only live telemetry.

#### Scenario: Evidence records the formal reviewable proof surfaces that remain required
- **WHEN** the same evidence summarizes node-`5` transport and residual proof
  surfaces
- **THEN** it SHALL identify `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`,
  transport-error growth, `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
  `CSP_OWNER_TIMEOUT`, `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band
  `CommEgressMux` counters as formal reviewable observability
- **AND** it SHALL keep those surfaces distinct from both pass-time keep-live
  truth and diagnostics-only residual chatter.

#### Scenario: Evidence separates ambient residual governance from explicit detailed GET requalification
- **WHEN** the same change also repairs representative detailed `GET_*`
  requalification
- **THEN** it SHALL keep that explicit bounded proof distinct from the ambient
  residual-governance inventory
- **AND** it SHALL not treat detailed `GET_*` closure as permission to reopen
  broad non-baseline live packet chatter.

#### Scenario: Hosted dual-GDS evidence proves the corrected RSS semantics
- **WHEN** the hosted manual dual-GDS headless path is rerun after this change
- **THEN** the evidence SHALL compare hosted `SYS_MEM_RSS_MB` with the
  OS-observed current `OBC` process RSS
- **AND** it SHALL show that normal hosted manual dual-GDS operation no longer
  produces spurious repeated `SYS_LOW_MEMORY` warnings from historical peak RSS
  growth alone.

### Requirement: Replacement Change Evidence
The verification evidence baseline SHALL record reviewable evidence for reduced-state behavior, live beacon broadcast decode, official F' HK trend data product generation, catalog behavior, and explicit scope exclusions.

#### Scenario: Reduced-state evidence is reviewable
- **WHEN** focused helper and component tests pass
- **THEN** the evidence SHALL identify reduced-state classification, missing-source behavior, and stale-state invalidation coverage

#### Scenario: Beacon broadcast evidence is reviewable
- **WHEN** the live beacon hosted probe passes
- **THEN** the evidence SHALL identify the simulated COMM-facing broadcast path, captured beacon sequence, decoded payload fields, CRC result, and source values used for comparison
- **AND** the evidence SHALL state that RF behavior and ground ACK behavior are not covered

#### Scenario: Official HK data product evidence is reviewable
- **WHEN** the HK trend hosted probe passes
- **THEN** the evidence SHALL identify generated official F' data product files, `DpCatalog` catalog behavior, `DpCatalog` file queueing, and the absence of the superseded repo-local `catalog.csv` baseline

#### Scenario: Telemetry and event regression remains separate
- **WHEN** this change validates live beacon and HK data products
- **THEN** the evidence SHALL also confirm that original command, event, and telemetry paths continue to use the existing `TlmChan` and `ComFprime` baseline

### Requirement: Hosted FDP Parity Evidence
The verification evidence SHALL distinguish hosted official `.fdp` received-file parity from generated-file or queue-only evidence.

#### Scenario: Hosted FDP parity evidence is reviewable
- **WHEN** the FDP parity change completes
- **THEN** the evidence record SHALL include the hosted probe command, generated source `.fdp`, GDS-received `.fdp`, byte-match verdict, decode verdict, and final scope boundary
- **AND** the verification-path registry SHALL identify hosted official `.fdp` byte-match/decode as proven only for the direct hosted GDS path

#### Scenario: Parity exclusions remain explicit
- **WHEN** the evidence discusses official `.fdp` downlink parity
- **THEN** it SHALL explicitly exclude RF behavior, physical COMM `.fdp` parity, CFDP, ARQ/NACK, packet-loss recovery, and arbitrary onboard file downlink

### Requirement: Mode Model V2 Evidence
The verification evidence SHALL record reviewable local evidence for the primary mode enum replacement, runtime mode command/status behavior, live beacon mode decode behavior, HK trend mode decode behavior, and explicit exclusions for deferred autonomy and communication work.

#### Scenario: Mode model evidence is reviewable
- **WHEN** mode-model-v2 completes local verification
- **THEN** the evidence SHALL identify the generated/build/test commands, focused mode command coverage, invalid mode rejection, beacon mode encode/decode coverage, beacon schema version, HK trend payload version, and HK trend mode decode coverage

#### Scenario: Deferred scope is explicit
- **WHEN** mode-model-v2 evidence is recorded
- **THEN** it SHALL state that the evidence does not prove COMM split-link behavior, CCSDS behavior, storage-policy behavior, FDIR, MissionExecutive autonomy, target hardware behavior, RF behavior, reliable transfer, or pass scheduling

### Requirement: Mode Safety Policy Evidence
The verification evidence SHALL record reviewable local evidence for the SoC-driven mode safety policy, including component tests, helper tests, focused integration tests, hosted probe coverage, OpenSpec validation, and explicit out-of-scope boundaries.

#### Scenario: Component and helper evidence is reviewable
- **WHEN** mode-safety-policy-v1 completes local verification
- **THEN** the evidence SHALL identify the focused `ModeSafetyController` component tests, pure policy-helper tests, and affected `EpsBridge` component test coverage that cover missing EPS cache, stale-cache invalidation after failed EPS polling, unconfigured runtime dependency behavior, strict threshold boundaries, `SAFE -> HELL`, `HELL -> SAFE`, `IDLE/PAYLOAD/TTC -> SAFE`, already-target no-op behavior, high-SoC `SAFE` no-auto-recovery behavior, and non-fatal EPS critical-battery alarm behavior

#### Scenario: Integration evidence uses the normal mode surface
- **WHEN** mode-safety-policy-v1 records integration evidence
- **THEN** the evidence SHALL show that safety fallback requests go through `ModeManager::applyModeForInternalSource` or the equivalent normal runtime mode-control path and result in the normal `SYS_MODE_CHANGE` mode surface rather than a parallel mode store

#### Scenario: Hosted focused evidence is reviewable
- **WHEN** mode-safety-policy-v1 records hosted evidence
- **THEN** the evidence SHALL identify the hosted focused probe command, isolated runtime roots or ports, EPS simulator initial SoC cases, expected mode fallback outcome for each case, observed outcome, and final verdict

#### Scenario: Exclusions stay explicit
- **WHEN** mode-safety-policy-v1 evidence is recorded
- **THEN** the evidence SHALL state that it does not prove COMM split-link behavior, CCSDS behavior, watchdog or FDIR expansion, subsystem timeout/retry/reset behavior, target hardware behavior, RF behavior, payload scheduling, TTC pass automation, TLE handling, or reliable transfer

#### Scenario: OpenSpec validation is recorded
- **WHEN** mode-safety-policy-v1 is ready for closeout
- **THEN** the evidence SHALL include `openspec validate mode-safety-policy-v1` and `openspec validate --specs` results

### Requirement: Dual-Link COMM Simulator Foundation Evidence
The verification evidence baseline SHALL record bounded evidence for the hosted dual-link COMM simulator foundation without over-claiming later S-band or UHF transport paths.

#### Scenario: Evidence records explicit process identities
- **WHEN** the dual-link COMM simulator foundation probe passes
- **THEN** the evidence SHALL record the generic `comm_csp_node` node `4`, `sband_comm_csp_node` node `5`, and `uhf_comm_csp_node` node `6` executable identities
- **AND** it SHALL record distinct logs or output excerpts for each executable identity

#### Scenario: Evidence records bounded CSP service behavior
- **WHEN** the dual-link foundation probe exercises COMM services
- **THEN** the evidence SHALL show that each hosted COMM identity responds on services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the evidence SHALL show that OBC can reach nodes `4`, `5`, and `6` over the governed hosted internal CSP substrate

#### Scenario: Evidence exclusions stay explicit
- **WHEN** dual-link foundation evidence is recorded
- **THEN** it SHALL state that the evidence does not prove complete S-band GDS behavior, UHF UART/RS485/USB/macOS backup behavior, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, file/downlink behavior, target hardware behavior, or Pi hardware deployment

### Requirement: S-band TCP Ground Link Evidence Is Reviewable
The verification evidence baseline SHALL record the commands, launcher scripts, endpoints, observed bounded traffic, and final verdict for the hosted S-band TCP through-COMM path.

#### Scenario: Evidence records S-band process identity and path
- **WHEN** the hosted S-band TCP ground-link probe passes
- **THEN** the evidence SHALL record `sband_comm_csp_node` as link identity `sband` and CSP node `5`
- **AND** it SHALL record the path as `fprime-cli -> fprime-gds -> ground_ttc_gateway -> S-band TCP -> sband_comm_csp_node(node 5) -> OBC`

#### Scenario: Evidence records bounded TT&C proof
- **WHEN** the evidence describes S-band TCP command/event/channel PASS
- **THEN** it SHALL include bounded command readback from OBC
- **AND** it SHALL include ground-side command event observations from `fprime-cli events`
- **AND** it SHALL include ground-side telemetry observations from `fprime-cli channels` for `GROUND_LINK_TX_BYTES`

#### Scenario: Evidence records bounded file/downlink proof
- **WHEN** the evidence describes S-band TCP file/downlink PASS
- **THEN** it SHALL identify the downlinked official `.fdp` files selected by `DpCatalog`
- **AND** it SHALL state that each received file was compared byte-for-byte against an OBC runtime source snapshot

#### Scenario: Evidence exclusions stay explicit
- **WHEN** S-band TCP ground-link evidence is recorded
- **THEN** it SHALL state that the evidence does not prove direct `GDS -> TCP -> OBC`, UHF UART backup, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, pass scheduling, or arbitrary onboard file downlink

### Requirement: UHF UART Backup Link Evidence Is Reviewable
The verification evidence baseline SHALL record the commands, launcher scripts, endpoints, observed bounded traffic, BeaconV1 capture/decode artifacts, and final verdict for the hosted UHF UART backup/beacon path.

#### Scenario: Evidence records UHF process identity and command path
- **WHEN** the hosted UHF UART backup probe passes
- **THEN** the evidence SHALL record `uhf_comm_csp_node` as link identity `uhf` and CSP node `6`
- **AND** it SHALL record the command ingress path as `fprime-cli -> fprime-gds -> ground_ttc_gateway(link=uhf serial) -> hosted serial -> uhf_comm_csp_node(node 6) -> OBC`

#### Scenario: Evidence records bounded backup command ingress
- **WHEN** the evidence describes UHF backup command ingress PASS
- **THEN** it SHALL include bounded command readback from OBC
- **AND** it SHALL include ground-side command event observations from `fprime-cli events`
- **AND** it SHALL include ground-side telemetry observations from `fprime-cli channels` for `GROUND_LINK_TX_BYTES`

#### Scenario: Evidence records UHF BeaconV1 capture
- **WHEN** the evidence describes UHF beacon PASS
- **THEN** it SHALL identify the captured BeaconV1 binary artifact and decoded JSON artifact
- **AND** it SHALL record that the captured frame was decoded with expected wire size, schema version, CRC, and representative populated state fields
- **AND** it SHALL identify the beacon path as UHF node-6 side-channel capture rather than file/downlink or command response behavior

#### Scenario: Evidence exclusions stay explicit
- **WHEN** UHF UART backup evidence is recorded
- **THEN** it SHALL state that the evidence does not prove S-band TCP, direct `GDS -> TCP -> OBC`, full UHF command authority, failover policy, arbitrary file downlink, CCSDS behavior, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, physical USB serial hardware, or physical RS485 electrical validation

### Requirement: Runtime Maintainability Evidence Proves Behavior Preservation
The verification evidence SHALL record reviewable local evidence for hosted runtime maintainability refactors, including focused helper tests, build coverage, reused hosted probe coverage, OpenSpec validation, and explicit out-of-scope boundaries.

#### Scenario: Helper tests cover extracted dispatch behavior
- **WHEN** hosted command parsing or dispatch logic is extracted into helper code
- **THEN** the change SHALL include focused helper tests for command routing, stop commands, unknown commands, parser failure messages, help text, and launch argument validation

#### Scenario: Existing hosted paths are rerun as behavior-preservation evidence
- **WHEN** the runtime refactor is complete
- **THEN** repository-owned hosted probes SHALL be rerun after a fresh build for the relevant default hosted runtime and CCSDS spike runtime behavior
- **AND** the evidence SHALL identify those probes as reused behavior-preservation paths rather than newly proven validation paths

#### Scenario: Evidence avoids new path claims
- **WHEN** runtime maintainability evidence is recorded
- **THEN** it SHALL state that the refactor does not create new direct-GDS, S-band-through-COMM, UHF serial backup, CCSDS adoption, target/Pi, RF, reliable-transfer, command-authority, failover, or pass-scheduler claims
- **AND** it SHALL cite existing registry entries only for the paths actually reused during verification

### Requirement: CCSDS Hosted Adoption Evidence Is Independent
The verification evidence baseline SHALL record default hosted S-band CCSDS adoption evidence independently from historical CCSDS spike evidence and existing stock `ComFprime` S-band and UHF gateway records.

#### Scenario: Evidence identifies default adoption path
- **WHEN** CCSDS hosted adoption evidence is recorded
- **THEN** it SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> default hosted OBC`
- **AND** it SHALL identify existing S-band and UHF records as `ComFprime` gateway baselines, not CCSDS adoption evidence
- **AND** it SHALL identify `ccsds-ground-link-spike-v1` as historical spike evidence rather than the default hosted adoption evidence

#### Scenario: Evidence records framing and decoded APID observations
- **WHEN** CCSDS hosted adoption evidence is recorded
- **THEN** it SHALL record `framing=space-packet-space-data-link`, `scid=0x44`, `vcid=1`, and `frame-size=1024`
- **AND** it SHALL record decoded frame/APID observations for command `0`, telemetry `1`, log/event `2`, and file `3`
- **AND** it SHALL record observed sequence counts where available from decoded frame inspection without treating those counts as reliable-transfer evidence

#### Scenario: Evidence records bounded TT&C and file proof
- **WHEN** CCSDS hosted adoption evidence records a PASS
- **THEN** it SHALL include bounded command readback for `EPS_SET_PDU` and `ADCS_SET_MODE`
- **AND** it SHALL include command event observations, `GROUND_LINK_TX_BYTES` telemetry observations, and `DpCatalog`-driven `.fdp` file downlink
- **AND** it SHALL state that received `.fdp` files were compared byte-for-byte against OBC runtime source snapshots

#### Scenario: Evidence exclusions stay explicit
- **WHEN** CCSDS hosted adoption evidence is finalized
- **THEN** it SHALL state that the evidence does not prove RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, command authority, failover policy, pass scheduling, arbitrary onboard file downlink, or HK data-product field alignment

### Requirement: Active UHF CCSDS Adoption Evidence Is Independent
The verification evidence baseline SHALL record active hosted UHF CCSDS adoption evidence independently from the historical UHF `ComFprime` gateway records and from the default hosted S-band CCSDS adoption record.

#### Scenario: Evidence identifies the active UHF CCSDS path
- **WHEN** active UHF CCSDS adoption evidence is recorded
- **THEN** it SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(link=uhf serial) -> hosted serial -> uhf_comm_csp_node(node 6) -> default hosted OBC`
- **AND** it SHALL state that the bounded proof traffic occurs only after the active COMM runtime explicitly switches the command, telemetry, and file roles to UHF on the default hosted `OBC` baseline
- **AND** it SHALL identify historical UHF records as `ComFprime` gateway baselines or regression evidence rather than active UHF CCSDS adoption evidence

#### Scenario: Evidence records framing and decoded APID observations
- **WHEN** active UHF CCSDS adoption evidence is recorded
- **THEN** it SHALL record `framing=space-packet-space-data-link`, `scid=0x44`, `vcid=2`, and `frame-size=1024`
- **AND** it SHALL record decoded frame/APID observations for command `0`, telemetry `1`, log/event `2`, and file `3`

#### Scenario: Evidence records bounded active UHF behavior
- **WHEN** active UHF CCSDS adoption evidence records a PASS
- **THEN** it SHALL include bounded command uplink, command event observations, telemetry observations, and bounded file/downlink byte-match on the active hosted `OBC` path after explicit switch to UHF primary

#### Scenario: Evidence exclusions stay explicit
- **WHEN** active UHF CCSDS adoption evidence is finalized
- **THEN** it SHALL state that the evidence does not prove RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, broader UHF authority expansion, or historical legacy retirement

### Requirement: Payload TTC Mode Entry Evidence
The verification evidence SHALL record reviewable local evidence for payload-ttc-mode-entry-v1, including OpenSpec validation, focused component/helper tests, hosted shell regression, default hosted CCSDS S-band command/event/telemetry coverage, and explicit deferred boundaries.

#### Scenario: Component and helper evidence covers the matrix
- **WHEN** payload-ttc-mode-entry-v1 completes local verification
- **THEN** the evidence SHALL identify focused tests covering all 25 operator from/to mode pairs, same-mode no-op side effects, rejected transition no-change semantics, rejection reason mapping, guard-unconfigured response mapping, generated invalid-enum `FORMAT_ERROR` behavior, parser invalid spelling behavior, and internal safety apply path separation

#### Scenario: HELL recovery guard evidence is reviewable
- **WHEN** payload-ttc-mode-entry-v1 records guard evidence
- **THEN** the evidence SHALL identify tests covering operator `HELL -> SAFE` with cached EPS SoC greater than `15%`, exactly `15%`, less than `15%`, and unavailable cache
- **AND** the evidence SHALL identify tests or probes showing autonomous `ModeSafetyController` fallback and recovery still use the existing cached-EPS policy

#### Scenario: Hosted shell regression evidence is reviewable
- **WHEN** payload-ttc-mode-entry-v1 records hosted shell evidence
- **THEN** the evidence SHALL include the hosted shell probe command, isolated runtime root or ports, boot-mode precondition check for `SYS_MODE == SAFE`, bounded command sequence, command outcomes, final mode observations, parser rejection cases, and final verdict

#### Scenario: Default hosted CCSDS S-band command path evidence is reviewable
- **WHEN** payload-ttc-mode-entry-v1 records hosted command path evidence
- **THEN** the evidence SHALL use the existing default hosted CCSDS S-band OBC path through `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> CSP -> OBC`
- **AND** it SHALL identify bounded `MODE_SET` commands, observed command responses, mode-change or rejection events, final `SYS_MODE` telemetry, and the reused verification registry path
- **AND** it SHALL NOT depend on event and telemetry transport arrival ordering

#### Scenario: Evidence record has reproducibility fields
- **WHEN** payload-ttc-mode-entry-v1 records final evidence
- **THEN** the evidence SHALL include branch name, base commit SHA, final local commit SHA, OpenSpec change name, exact build and verification commands, unit/component test result summary, hosted shell sequence and results, CCSDS/F Prime command sequence and results, command responses, event excerpts, `SYS_MODE` before/after telemetry snapshots, reused or new verification path, and deferred work

#### Scenario: Exclusions stay explicit
- **WHEN** payload-ttc-mode-entry-v1 evidence is recorded
- **THEN** the evidence SHALL state that it does not prove scheduler/pass-window automation, ADCS ground tracking, payload/camera control, payload power sequencing, link authority, auth/session policy, UHF failover, CCSDS route changes, HK data-product field changes, watchdog behavior, subsystem timeout/retry/reset behavior, broader FDIR, RF behavior, target hardware, or real payload/TTC mission execution

#### Scenario: OpenSpec validation is recorded
- **WHEN** payload-ttc-mode-entry-v1 is ready for closeout
- **THEN** the evidence SHALL include `openspec validate payload-ttc-mode-entry-v1` and `openspec validate --specs` results

### Requirement: Authority Vocabulary Evidence Is Traceable
Verification evidence for command authority vocabulary SHALL connect each OpenSpec requirement to policy behavior and at least one focused test or probe expectation.

#### Scenario: Traceability is reviewable
- **WHEN** reviewers inspect `link-authority-vocabulary-v1` evidence
- **THEN** they SHALL be able to trace vocabulary requirements to policy source behavior, dictionary catalog generation, and focused tests.

### Requirement: Authority Catalog Coverage Is Verified
Verification evidence SHALL show that maintained active OBC topology dictionary
commands are covered by the command authority policy.

#### Scenario: Dictionary coverage is enforced
- **WHEN** focused authority catalog tests run
- **THEN** they SHALL verify that every command in the maintained active CCSDS
  topology dictionary has a classification
- **AND** they SHALL verify that generated runtime opcode catalog output matches
  the checked-in policy source and current dictionary command names
- **AND** they SHALL NOT require retired `OBCAppComFprimeLegacy.*` policy
  entries.

### Requirement: Command Ingress Authority Evidence Is Reviewable
Verification evidence for command ingress authority SHALL record the component tests, policy/catalog tests, hosted `sband-primary` and `uhf-backup` configured-profile proof, and explicit deferred boundaries.

#### Scenario: Component evidence covers response semantics
- **WHEN** command ingress authority component tests are reviewed
- **THEN** they SHALL show allowed forwarding, denied no-forward behavior, synthetic response exactly once, source/context preservation, restricted malformed and unknown fail-closed behavior, invalid config denial, throttled events, and unthrottled counters.

#### Scenario: Hosted evidence covers configured S-band and UHF roles
- **WHEN** hosted command ingress authority evidence is recorded
- **THEN** it SHALL include a default CCSDS S-band path result with `sband-primary` configured authority where an authorized command reaches subsystem execution
- **AND** it SHALL include a default CCSDS S-band routed path result with `uhf-backup` configured authority where one allowed status command succeeds and one high-authority command is denied before subsystem execution
- **AND** it SHALL state that this hosted profile proof does not establish physical UHF serial provenance or infer flight-side link identity from `Fw.Com.context`.

#### Scenario: Evidence does not over-claim link authority
- **WHEN** reviewers inspect command ingress authority evidence
- **THEN** the evidence SHALL state that file packet uplink, unknown packet routing, full link authority, full uplink authority, crypto authentication, sessions, sequence windows, replay protection, dynamic failover, and persistent authority config store remain out of scope.

### Requirement: Command Ingress Source Index Evidence Is Reviewable
The verification evidence SHALL record the configured ingress source-index
behavior introduced by `command-ingress-source-index-v1`.

#### Scenario: Component tests prove multi-port behavior
- **WHEN** the change is closed out
- **THEN** evidence SHALL list component tests covering configured port `0`,
  configured port `1`, unconfigured port fail-closed behavior, legacy
  `configure(config)` clearing semantics, and context preservation.

#### Scenario: Hosted probe proof is scoped to port zero
- **WHEN** hosted probe evidence is recorded
- **THEN** it SHALL state that current hosted default CCSDS topology wires only
  authority ingress index `0`
- **AND** it SHALL avoid claiming hosted proof for ingress port `1` or
  simultaneous S-band/UHF routed command ingress.

### Requirement: Session Sequence Foundation Evidence Is Reviewable
The verification evidence SHALL record direct helper tests and explicitly separate them from active runtime command enforcement.

#### Scenario: Helper tests cover strict monotonic behavior
- **WHEN** the change is closed out
- **THEN** evidence SHALL list tests covering first sequence acceptance, increasing sequence acceptance, duplicate rejection, lower sequence rejection, independent ingress/session/role keys, explicit reset, and wraparound rejection without reset.

#### Scenario: Evidence avoids runtime security overclaims
- **WHEN** evidence cites `command-session-sequence-foundation-v1`
- **THEN** it SHALL state that the helper is not wired into the current `Fw.Com` command path
- **AND** it SHALL state that replay protection, command envelopes, authentication, session-open protocol, and active sequence enforcement are deferred.

### Requirement: Historical Legacy Command Envelope Metadata Evidence Is Reviewable
The verification evidence SHALL keep command envelope metadata behavior
reviewable as historical legacy compatibility evidence without claiming active
current session authority or trusted source provenance.

#### Scenario: Unit and component tests prove envelope behavior
- **WHEN** the change is closed out
- **THEN** evidence SHALL list helper and classic component tests covering legacy command compatibility, valid envelope unwrap, context preservation, envelope metadata observation, malformed envelope fail-closed behavior, and UHF backup denial of enveloped high-authority commands.

#### Scenario: Hosted proof covers the routed CCSDS command path
- **WHEN** hosted probe evidence is recorded
- **THEN** it SHALL show legacy command dispatch still works through `fprime-cli`
- **AND** it SHALL show a repo-owned envelope injector sending an enveloped command through the existing hosted CCSDS S-band routed command path
- **AND** it SHALL show envelope observed/rejected events from ground-side event capture.

#### Scenario: Evidence scope remains bounded
- **WHEN** evidence cites `command-envelope-metadata-v1`
- **THEN** it SHALL state that the proof covers current hosted authority ingress port `0` only
- **AND** it SHALL NOT claim active session enforcement, replay protection, authentication, crypto, physical UHF provenance, dual-link simultaneous proof, file authority, unknown packet authority, or reliable transfer.

### Requirement: Command Session Sequence Evidence Covers Active Runtime Rejection

Verification evidence SHALL prove active command session sequence enforcement for valid enveloped commands without claiming full replay protection.

#### Scenario: Component tests cover runtime sequence acceptance and rejection
- **WHEN** `CommandIngressAuthority` component tests are reviewed
- **THEN** evidence SHALL list tests for legacy command compatibility, first sequence acceptance, increasing sequence acceptance, duplicate rejection, lower rejection, wraparound rejection, per-key independence, sequence-window-full rejection, and synthetic response behavior
- **AND** it SHALL list tests proving authority-denied envelopes do not consume sequence state.

#### Scenario: Hosted probe covers current ingress port only
- **WHEN** hosted command session sequence evidence is recorded
- **THEN** it SHALL reuse the current hosted CCSDS S-band routed command path and current authority ingress port `0`
- **AND** it SHALL include accepted, increasing, duplicate, lower, and authority-denied-does-not-consume observations
- **AND** it SHALL state that hosted port `1`, simultaneous S-band/UHF routed ingress, physical UHF provenance, trusted source, authentication, full replay protection, reliable transfer, file authority, unknown packet authority, RF behavior, target hardware, and Pi deployment remain out of scope.

#### Scenario: Evidence records sequence rejection observations
- **WHEN** sequence rejection evidence is recorded
- **THEN** it SHALL include command responses, `COMMAND_SEQUENCE_REJECTED` event excerpts, sequence telemetry excerpts, and confirmation that the rejected inner command did not reach the downstream component.

### Requirement: Mode SoC Admission And Exit Evidence
The verification evidence SHALL record reviewable local evidence for `mode-soc-admission-and-exit-v1`, including focused helper/component/integration coverage, hosted shell regression, default hosted CCSDS S-band command-path coverage, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover SoC admissions and payload exit
- **WHEN** `mode-soc-admission-and-exit-v1` completes local verification
- **THEN** the evidence SHALL identify tests covering:
  - operator `SAFE -> IDLE` accepted only when cached SoC is strictly greater than `50%`
  - operator `IDLE -> PAYLOAD` accepted only when cached SoC is strictly greater than `70%`
  - unavailable cached EPS fail-closed behavior for SoC-guarded admits
  - automatic `PAYLOAD -> IDLE` when cached SoC is less than `60%`
  - `PAYLOAD -> SAFE` below `40%` taking precedence over `PAYLOAD -> IDLE`
  - unchanged `IDLE -> TTC` behavior without a new SoC admission threshold

#### Scenario: Hosted shell evidence is reviewable
- **WHEN** `mode-soc-admission-and-exit-v1` records hosted shell evidence
- **THEN** the evidence SHALL include the shell probe command, isolated runtime root or ports, bounded operator sequence, command outcomes, rejection reasons, final mode observations, and final verdict

#### Scenario: Default hosted CCSDS S-band MODE_SET evidence is reviewable
- **WHEN** `mode-soc-admission-and-exit-v1` records hosted command-path evidence
- **THEN** the evidence SHALL use the existing default hosted CCSDS S-band OBC path through `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> CSP -> OBC`
- **AND** it SHALL identify bounded `MODE_SET` commands, observed command responses, mode-change or rejection events, final `SYS_MODE` telemetry, and the reused verification registry path
- **AND** it SHALL include at least one fail-closed unavailable-cache case for `SAFE -> IDLE` or `IDLE -> PAYLOAD`

#### Scenario: Evidence record captures closeout commands
- **WHEN** `mode-soc-admission-and-exit-v1` is ready for closeout
- **THEN** the evidence SHALL include:
  - fresh build and affected test commands
  - focused hosted probe commands
  - `openspec validate mode-soc-admission-and-exit-v1`
  - `openspec validate --specs`
  - `python3 scripts/check_repo_consistency.py`

#### Scenario: Exclusions stay explicit
- **WHEN** `mode-soc-admission-and-exit-v1` evidence is recorded
- **THEN** the evidence SHALL state that it does not prove TTC pass scheduling, TLE/GPS/pass-window logic, ADCS ground tracking, payload/camera control, payload power sequencing, watchdog behavior, subsystem timeout/retry/reset FDIR, command auth/session lifecycle, configurable SoC threshold commands, RF behavior, target hardware, or a new mode engine

### Requirement: EPS Timeout FDIR Evidence
The verification evidence SHALL record reviewable local evidence for `fdir-subsystem-timeout-v1`, including focused component/helper/integration coverage, hosted probe evidence, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover timeout thresholding and recovery
- **WHEN** `fdir-subsystem-timeout-v1` completes local verification
- **THEN** the evidence SHALL identify tests covering:
  - EPS consecutive poll-failure counting
  - cumulative EPS poll comm-error counting
  - reset-to-healthy behavior on the first successful poll
  - no escalation on failures `1` and `2`
  - one-shot escalation at failure `3`
  - no duplicate escalation while the fault remains latched
  - recovery clear on the first successful poll after fault
  - `SAFE` and `HELL` fault-only behavior with no duplicate mode request

#### Scenario: Integration evidence uses the normal mode surface
- **WHEN** `fdir-subsystem-timeout-v1` records integration evidence
- **THEN** the evidence SHALL show that EPS timeout escalation requests go through `ModeManager::applyModeForInternalSource` or the equivalent normal runtime mode-control path
- **AND** it SHALL show that the outward-facing mode result remains the normal `SYS_MODE_CHANGE` surface rather than a parallel fault-owned mode store

#### Scenario: Hosted probe evidence is reviewable
- **WHEN** `fdir-subsystem-timeout-v1` records hosted evidence
- **THEN** the evidence SHALL include the hosted probe command, isolated runtime roots or ports, simulator stop or restart method, expected outcomes for transient-failure, threshold-crossing, and recovery-after-fault cases, observed outcomes, and final verdict

#### Scenario: Evidence record captures closeout commands
- **WHEN** `fdir-subsystem-timeout-v1` is ready for closeout
- **THEN** the evidence SHALL include:
  - `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-fdir-subsystem-timeout-v1`
  - focused `ctest` commands for affected EPS and mission-autonomy coverage
  - `bash scripts/run_eps_csp_integration.sh`
  - `bash scripts/run_eps_timeout_fdir_hosted_probe.sh`
  - `openspec validate fdir-subsystem-timeout-v1`
  - `openspec validate --specs`

#### Scenario: Exclusions stay explicit
- **WHEN** `fdir-subsystem-timeout-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove timestamp freshness policy, EPS reset or power-cycle recovery, multi-subsystem FDIR, watchdog behavior, persistent fault/event storage, CCSDS command-path expansion, RF behavior, target hardware, command auth/session/QoS work, TTC pass automation, payload control, or a broader mission-execution framework

### Requirement: Historical Legacy Command Session Lifecycle Evidence Is Reviewable

Verification evidence SHALL keep explicit legacy session-open and recovery
behavior reviewable only as archived historical compatibility evidence and
SHALL NOT require rerunnable legacy `SESSION_OPEN` proof as current maintained
baseline authority.

#### Scenario: Historical lifecycle evidence remains reviewable
- **WHEN** archived legacy command-session lifecycle evidence is cited
- **THEN** it SHALL remain valid to review unopened-command rejection,
  historical accepted `SESSION_OPEN`, historical sequence behavior, and
  historical restart semantics on that retired path
- **AND** it SHALL identify that evidence as historical compatibility only.

#### Scenario: Current maintained closeout does not require rerunning legacy lifecycle wrappers
- **WHEN** a current secure-baseline change reaches local-ready
- **THEN** maintained verification evidence SHALL require the fresh local gate
  plus re-qualified maintained secure-auth probes
- **AND** it SHALL NOT require rerunning legacy hosted `SESSION_OPEN`
  lifecycle wrappers as current closeout authority.

### Requirement: Service-Managed Timing Ceiling Freeze Records Control Preflight

Archived timing-ceiling-freeze evidence SHALL remain reviewable for its
original control-preflight and measurement boundary, but current maintained
closeout flows SHALL not require rerunning that wrapper family after timing
retirement.

#### Scenario: Retired timing preflight is not a current maintained gate
- **WHEN** a later unrelated secure-baseline change reaches closeout
- **THEN** verification evidence SHALL NOT require
  `target-timing-empirical-ceiling-freeze-v1` control-preflight reruns as a
  current maintained gate
- **AND** it MAY still cite the archived historical timing record when a
  change explicitly discusses timing ancestry.

### Requirement: Service-Managed Timing Ceiling Freeze Uses Repeated Clean Runs

Archived numeric timing-ceiling evidence SHALL remain reviewable as a retired
historical path rather than a current maintained closeout dependency.

#### Scenario: Retired timing ceiling evidence stays historical
- **WHEN** reviewers inspect the frozen empirical ceiling records after this
  change
- **THEN** they SHALL find those repeated-clean-run records preserved as
  archived historical evidence
- **AND** they SHALL NOT find them required as current maintained proof for
  secure-auth-only command-ingress retirement.

### Requirement: Authenticated Command Envelope Evidence Is Reviewable

Verification evidence for the hosted secure-auth baseline SHALL now cover the
tracked keystore contract and the widened bounded uplink authority story in
addition to secure command proof.

#### Scenario: Hosted secure-auth evidence proves keystore-backed bootstrap
- **WHEN** `uplink-authority-and-key-hardening-v1` evidence is cited
- **THEN** it SHALL show that hosted S-band and UHF secure auth use the shared
  tracked keystore asset rather than runtime-injected command-auth keys
- **AND** it SHALL record the maintained helper or simulator commands used to
  read that asset.

#### Scenario: Unknown uplink reject evidence is reviewable
- **WHEN** the same evidence cites comm-managed unknown uplink closure
- **THEN** it SHALL include malformed or unsupported APID `0x00FE` rejection
  cases
- **AND** it SHALL show that those rejects do not create or mutate active auth
  state.

#### Scenario: Staged file-uplink authority evidence is reviewable
- **WHEN** the same evidence cites staged file-uplink authority
- **THEN** it SHALL include:
  - S-band secure auth plus `.sequence-staging/<leaf>` upload success
  - UHF backup secure auth plus staged upload denial
  - explicit UHF failover-primary re-auth plus staged upload success
  - mid-transfer revoke or timeout drop behavior
  - non-staging destination rejection

#### Scenario: Legacy compatibility remains separate and keystore-backed
- **WHEN** the same evidence cites retained comm-managed legacy v1
  compatibility
- **THEN** it SHALL state that legacy comm-managed auth also used the shared
  tracked keystore contract
- **AND** it SHALL keep that legacy coverage separate from the hosted secure
  auth and staged-file closure proof.

### Requirement: Boot Trust Chain Evidence

The verification evidence tree SHALL record the commands, artifacts, trust model, manifest schema, rejection cases, and final verdict for the first boot trust-chain implementation.

#### Scenario: Boot trust evidence is reviewable
- **WHEN** `boot-trust-chain-v1` completes
- **THEN** reviewers SHALL be able to inspect the selected manifest schema, signer/trust-anchor model, version policy, runtime files changed, tests run, probes run, and observed pass/fail outcomes from `evidence/records/boot-trust-chain-v1/`.

#### Scenario: Evidence covers trust rejection behavior
- **WHEN** boot trust-chain evidence is recorded
- **THEN** it SHALL include explicit results for valid signed manifest acceptance, invalid signature rejection, unknown signer/key rejection, malformed manifest rejection, downgrade rejection, and staged image digest mismatch rejection.

#### Scenario: Evidence bounds hosted and Raspberry Pi claims
- **WHEN** hosted or Raspberry Pi boot trust probes are cited
- **THEN** the evidence SHALL identify which runtime path was actually proven
- **AND** it SHALL NOT claim hardware secure boot, bootloader partition switching, power-loss resilience, asymmetric signing, or physical media robustness unless those were separately validated.

### Requirement: Boot Component And Helper Coverage

The boot trust-chain change SHALL include both classic F' `BootManager` component coverage and direct helper coverage for parser/verifier behavior.

#### Scenario: BootManager L2 coverage remains required
- **WHEN** `BootManager` component behavior changes for boot trust-chain enforcement
- **THEN** its classic F' L2 harness SHALL cover success, rejection, activation, confirm, rollback, and metadata reload behavior through the component command/event/telemetry surface.

#### Scenario: Helper tests do not replace component coverage
- **WHEN** manifest parsing or signature verification helpers are added
- **THEN** direct L1 helper tests SHALL cover parser/verifier edge cases
- **AND** those helper tests SHALL NOT replace the required `BootManager` component harness coverage.

### Requirement: COMM Session-And-Downlink QoS Evidence Separates Command And File Verdicts

The verification evidence tree SHALL record command-policy proof and shared file/downlink proof for the COMM session-and-downlink QoS slice as separate, reviewable verdict surfaces.

#### Scenario: Command-policy evidence remains distinct

- **WHEN** the COMM session-and-downlink QoS change completes
- **THEN** reviewers SHALL be able to inspect separate evidence for S-band full-authority command admission, UHF backup low-risk admission, UHF backup high-risk rejection, and policy-driven session transition behavior

#### Scenario: File/downlink evidence remains distinct

- **WHEN** the COMM session-and-downlink QoS change completes
- **THEN** reviewers SHALL be able to inspect separate evidence for HK/DP arbitration behavior and bounded UHF-primary file/downlink behavior instead of inferring those from command-policy output alone

### Requirement: Watchdog-v1 Evidence Is Reviewable
The verification evidence SHALL record reviewable local evidence for watchdog-v1, including focused component/helper coverage, hosted probe evidence, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover watchdog thresholds and recovery
- **WHEN** watchdog-v1 completes local verification
- **THEN** the evidence SHALL identify tests covering:
  - migrated CPU/RSS resource-monitoring behavior under the new owner
  - healthy heartbeat behavior for all supervised sources
  - warning-only threshold crossing
  - fault latch without overstating `SAFE_REQUESTED` before a real watchdog `SAFE` request
  - at-most-one watchdog `SAFE` escalation once the current mode is requestable
  - no duplicate `SAFE` request while watchdog fault remains latched
  - continued stale progression to feed suppression
  - first-beat source recovery clear
  - aggregate recovery waiting for all enabled supervised sources

#### Scenario: Integration evidence uses the normal mode surface
- **WHEN** watchdog-v1 records integration evidence
- **THEN** the evidence SHALL show that watchdog fault escalation requests go through `ModeManager::applyModeForInternalSource` or the equivalent normal runtime mode-control path
- **AND** it SHALL show that the outward-facing mode result remains the normal `SYS_MODE_CHANGE` surface rather than a parallel watchdog-owned mode store

#### Scenario: Hosted probe evidence is reviewable
- **WHEN** watchdog-v1 records hosted evidence
- **THEN** the evidence SHALL include the hosted probe command, isolated runtime roots or ports, the bounded beat-suppression method used to create stale watchdog sources, expected outcomes for healthy, warning, latched, suppressed, and recovered cases, observed outcomes, and final verdict

#### Scenario: Evidence record captures closeout commands
- **WHEN** watchdog-v1 is ready for closeout
- **THEN** the evidence SHALL include:
  - `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-watchdog-v1`
  - focused `ctest` commands for affected watchdog, mission-autonomy, and migrated resource-monitoring coverage
  - `bash scripts/run_watchdog_v1_probe.sh`
  - `openspec validate watchdog-v1`
  - `openspec validate --specs`

#### Scenario: Exclusions stay explicit
- **WHEN** watchdog-v1 evidence is recorded
- **THEN** it SHALL state that the evidence does not prove Raspberry Pi hardware watchdog reset, boot-safe-image recovery, reset-cause persistence, process restart executors, subsystem reset executors, broad multi-subsystem FDIR, persistent fault/event storage, RF behavior, target hardware closure, or a broader system-supervisor framework

### Requirement: Recovery Executor Evidence Is Reviewable
The verification evidence SHALL record reviewable local evidence for `recovery-executors-v1`, including focused component/helper coverage, shared-detector integration coverage, hosted reboot-equivalent probe evidence, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover shared recovery progression
- **WHEN** `recovery-executors-v1` completes local verification
- **THEN** the evidence SHALL identify tests covering:
  - shared incident open, status, relatch, and clear behavior
  - watchdog detector request/clear emission into the shared executor path
  - EPS detector request/clear emission into the shared executor path
  - reviewable process-restart intent recording without over-claiming a real process restart
  - real EPS interface reset execution
  - one-shot `SAFE` fallback through the normal mode-control surface
  - reboot escalation on repeated failure
  - boot metadata schema upgrade, reset-cause truth, boot-count truth, repeated-reset safe clamp, and stable-ack clearing

#### Scenario: Integration evidence keeps SAFE on the normal mode path
- **WHEN** `recovery-executors-v1` records integration evidence
- **THEN** the evidence SHALL show that shared recovery `SAFE` fallback still goes through `ModeManager::applyModeForInternalSource` or the equivalent normal runtime mode-control path
- **AND** it SHALL show that the outward-facing mode result remains the normal `SYS_MODE_CHANGE` surface rather than a parallel recovery-owned mode store

#### Scenario: Hosted probe evidence proves reboot-equivalent closure
- **WHEN** `recovery-executors-v1` records hosted evidence
- **THEN** the evidence SHALL include the hosted probe command, isolated runtime roots or ports, bounded stale/timeout injection method, same-runtime-root relaunch method, expected outcomes for watchdog progression, EPS progression, reboot relaunch truth, and final verdict

#### Scenario: Evidence record captures closeout commands
- **WHEN** `recovery-executors-v1` is ready for closeout
- **THEN** the evidence SHALL include:
  - the fresh local verification build command used for the change
  - focused `ctest` commands for affected recovery, watchdog, EPS, boot, and snapshot coverage
  - `bash scripts/run_recovery_executors_v1_probe.sh`
  - `openspec validate recovery-executors-v1`
  - `openspec validate --specs`

#### Scenario: Exclusions stay explicit
- **WHEN** `recovery-executors-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove Raspberry Pi hardware watchdog reset, target hardware reboot, subsystem power-cycle action, broad all-subsystem FDIR, persistent event-log infrastructure, TTC/pass scheduling, payload autonomy, or full secure boot redesign

### Requirement: Multi-Subsystem FDIR Evidence Records Shared Recovery Closure
The verification evidence SHALL record reviewable local and hosted proof for `multi-subsystem-fdir-v1`, including focused component/helper coverage, classic F' component coverage, hosted shared-recovery probe evidence, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover bounded detector and executor behavior
- **WHEN** `multi-subsystem-fdir-v1` completes
- **THEN** evidence SHALL list focused coverage for:
  - shared recovery source and action mapping
  - lock-outside-action execution for `RecoveryExecutor`
  - unchanged EPS timeout behavior
  - scheduled ADCS poll-health thresholds and clear behavior
  - command-triggered ADCS transport failure staying outside shared ADCS FDIR
  - COMM detector fault truth without detector-side primary-link switching
  - `WATCHDOG_ADCS_FDIR` supervision and normalization
  - reboot-only boot reset-cause truth for ADCS and COMM escalation

#### Scenario: Hosted probe proves bounded three-subsystem closure
- **WHEN** the repository-owned hosted `multi-subsystem-fdir-v1` probe runs
- **THEN** the evidence SHALL include the probe command, isolated runtime roots or ports, the bounded EPS, ADCS, and COMM fault-injection method, the shared recovery status observations, the reboot-equivalent relaunch observations, and the final verdict
- **AND** it SHALL separately state what the hosted path does not prove

#### Scenario: Archived multi-subsystem evidence does not imply a maintained rerunnable wrapper forever
- **WHEN** later baselines keep the archived `multi-subsystem-fdir-v1` evidence but retire its original hosted wrapper
- **THEN** current registry or operator-facing docs SHALL describe that evidence as historical archived closure rather than as a maintained rerunnable current proof surface

#### Scenario: Scope boundaries remain explicit
- **WHEN** `multi-subsystem-fdir-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove GPS, payload, TTC pass scheduling, storage-health recovery, generic all-subsystem FDIR, COMM RF recovery, target-hardware reset proof, persistent recovery configuration, or a broader spacecraft fault-protection platform

### Requirement: TTC Pass-Window Mode Evidence
The verification evidence SHALL record reviewable local and hosted proof for ttc-pass-window-mode-v1, including focused helper/component tests, hosted TTC policy proof, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Helper and component evidence is reviewable
- **WHEN** ttc-pass-window-mode-v1 completes local verification
- **THEN** the evidence SHALL identify tests covering epoch-window validation, window-active comparison, UTC-to-epoch conversion, GPS freshness validity, COMM loss timeout behavior, TTC config/window command handling, policy status semantics, auto-entry, auto-exit, manual coexistence, and safety/recovery precedence interactions

#### Scenario: Hosted TTC pass-window probe evidence is reviewable
- **WHEN** ttc-pass-window-mode-v1 records hosted proof
- **THEN** the evidence SHALL include the hosted probe command, isolated runtime roots or ports, bounded GPS/COMM setup method, expected outcomes, observed outcomes, and final verdict
- **AND** it SHALL identify proof for:
  - no-entry when TTC is disabled
  - auto-entry when TTC is enabled and the active pass window is valid
  - auto-exit when the pass window ends
  - safety precedence during TTC
  - manual and automatic TTC coexistence without split ownership
  - truthful hosted COMM-loss behavior on the chosen hosted path, including whether shared recovery precedence prevents direct observation of TTC policy timeout exit

#### Scenario: Evidence record has reproducibility fields
- **WHEN** ttc-pass-window-mode-v1 records final evidence
- **THEN** the evidence SHALL include branch name, base commit SHA, final local commit SHA, OpenSpec change name, exact build and verification commands, focused test summary, hosted probe sequence and results, command/status excerpts, reused or new verification path, and deferred work

#### Scenario: Exclusions stay explicit
- **WHEN** ttc-pass-window-mode-v1 evidence is recorded
- **THEN** it SHALL state that the evidence does not prove generic scheduler behavior, TLE upload/parsing, orbital propagation, ADCS ground tracking, payload scheduling, COMM RF lock closure, target hardware behavior, Raspberry Pi deployment closure, or a broader mission-planning framework

#### Scenario: OpenSpec validation is recorded
- **WHEN** ttc-pass-window-mode-v1 is ready for closeout
- **THEN** the evidence SHALL include `openspec validate ttc-pass-window-mode-v1` and `openspec validate --specs` results

### Requirement: Archived Official Sequencing Evidence Proves Admission, Upload, and Execution Together

The archived hosted official sequencing evidence family SHALL prove the
governed upload, admission, and execution path together whenever the
repository cites that family.

#### Scenario: Hosted sequence proof covers the bounded active path

- **WHEN** evidence cites `official-sequencing-system-resources-v1`
- **THEN** it SHALL include the governed hosted CCSDS file upload path into the sequence staging directory
- **AND** it SHALL include wrapper-driven validate/run/manual control results
- **AND** it SHALL include dispatcher or sequencer observations proving the admitted sequence executed on the official sequencing surface
- **AND** it SHALL include the configured tick interval and declared timing-latency bound

### Requirement: Evidence Covers Backup Bounds And Stock-Control Denial

The first archived official sequencing evidence SHALL prove the bounded backup
and direct-stock-command policy.

#### Scenario: Evidence records bounded sequence control policy

- **WHEN** the hosted probe is recorded
- **THEN** it SHALL include direct external denial of stock `SeqDispatcher.RUN`, `RUN_ARGS`, and raw `CmdSequencer` controls
- **AND** it SHALL include a backup-allowed read-only sequence acceptance case
- **AND** it SHALL include a backup rejection case where one inner command requires higher authority
- **AND** it SHALL include a rejected ownership-violating manual or cancel request

### Requirement: Evidence States Timing And Non-Claims Truthfully

Archived official sequencing evidence SHALL not over-claim scheduler or
mission-time closure.

#### Scenario: Evidence records bounded truth

- **WHEN** official sequencing evidence is written
- **THEN** it SHALL state that absolute timing truth depends on correct hosted POSIX wallclock
- **AND** it SHALL NOT claim mission scheduler behavior, persistent onboard schedule, GPS mission-time scheduling, payload planner behavior, or conflict-free multi-sequence arbitration

### Requirement: Persistent Fault Ring Evidence Is Reviewable
The verification evidence tree SHALL record reviewable local and hosted proof
for `persistent-fault-ring-v1`, including helper coverage, classic F'
component coverage, boot/recovery writer coverage, dual-copy fallback, same-
runtime-root relaunch proof, OpenSpec validation, and explicit deferred
boundaries.

#### Scenario: Focused tests cover store and owner contracts
- **WHEN** `persistent-fault-ring-v1` completes focused local verification
- **THEN** the evidence SHALL identify tests covering empty load, append,
  wraparound, newest-first readback, newer-copy corruption fallback, both-
  invalid empty load, and the `PersistentFaultManager` command or event or
  hosted-shell contract

#### Scenario: Hosted probe proves reboot-equivalent persistence only
- **WHEN** `persistent-fault-ring-v1` records hosted evidence
- **THEN** the evidence SHALL show a repository-owned probe that triggers shared
  recovery, relaunches the same runtime root, and reads back both pre-reboot
  recovery breadcrumbs and later boot breadcrumbs from the persistent ring
- **AND** it SHALL state that the verdict proves same-runtime-root relaunch
  persistence rather than target power-loss robustness

#### Scenario: Closeout gate stays explicit
- **WHEN** `persistent-fault-ring-v1` is ready for closeout
- **THEN** the evidence SHALL name the exact build, focused test, hosted probe,
  `openspec validate persistent-fault-ring-v1`, and
  `openspec validate --specs` commands used for the final verdict

### Requirement: Active OBC Raspberry Pi Alignment Evidence
The verification evidence tree SHALL record reviewable Raspberry Pi package,
install, and autostart evidence that the governed target-facing release path
ships the active `OBC` / `TopCcsds` payload and no longer preserves a maintained
legacy deployment in the active build or script surface.

#### Scenario: Package evidence proves active payload truth
- **WHEN** `active-topccsds-target-alignment-v1` records final package evidence
- **THEN** reviewers SHALL be able to inspect the package command, manifest, and
  bundle contents showing that `bin/OBC` and
  `dict/AppTopologyDictionary.json` were sourced from the active `OBC`
  deployment
- **AND** any legacy helper or evidence reference SHALL be labeled as historical
  rather than part of active package truth.

#### Scenario: Installed and rebooted target evidence stays on the active path
- **WHEN** `active-topccsds-target-alignment-v1` records installed-release and
  autostart evidence
- **THEN** reviewers SHALL be able to inspect the installed current-release
  launch result and the post-reboot service result on the corrected active
  package path
- **AND** the evidence SHALL describe that proof as target package/startup
  alignment rather than as full COMM lab-operational closure.

### Requirement: Historical Persistent Command Freshness Evidence Is Reviewable
The verification evidence tree SHALL record reviewable local, hosted, and
bounded Raspberry Pi proof for `persistent-command-freshness-v1` as retained
legacy compatibility evidence.

#### Scenario: Focused tests cover store and session contracts
- **WHEN** `persistent-command-freshness-v1` completes focused local
  verification
- **THEN** the evidence SHALL identify tests covering first-boot empty load,
  persisted-floor advance on accepted open, restart replay rejection, fresh
  higher reopen acceptance, stale old-session rejection before dispatch,
  per-source independence, newer-copy corruption fallback, and both-invalid
  fail-closed behavior.

#### Scenario: Historical persistence probes remain archived only
- **WHEN** `persistent-command-freshness-v1` archived records are cited
- **THEN** the evidence SHALL remain reviewable for the retired legacy
  persistence path
- **AND** it SHALL describe that ancestry as historical compatibility rather
  than current maintained secure-baseline closure.

#### Scenario: Current maintained closeout does not require persistence reruns
- **WHEN** a later secure-auth-only change reaches local-ready
- **THEN** verification evidence SHALL NOT require rerunning hosted or target
  `persistent-command-freshness-v1` probes as current maintained gates
- **AND** it MAY still cite the archived records when a change explicitly
  discusses historical persistence ancestry.

### Requirement: Active Probe Cleanup Hardening Evidence Is Reviewable

The verification evidence tree SHALL record reviewable proof for
`active-probe-cleanup-hardening-v1`, including interruption handling, rerun
success, and owned-helper cleanup on the governed active verification path.

#### Scenario: Hosted and Raspberry Pi rerun safety is reviewable
- **WHEN** `active-probe-cleanup-hardening-v1` records final evidence
- **THEN** reviewers SHALL be able to inspect repository-owned interruption and
  rerun proof for at least one representative hosted active probe and the
  active Raspberry Pi command-persistence probe
- **AND** the evidence SHALL show that each probe can be rerun immediately
  without manual process cleanup.

#### Scenario: Shell launcher cleanup proof is bounded and explicit
- **WHEN** `active-probe-cleanup-hardening-v1` records shell-launcher proof
- **THEN** the evidence SHALL include an interrupted and relaunched
  `run_remote_csp_gds_stack.sh` scenario plus a check that no owned stale
  `fprime-gds` helper processes remain for the owned port or log-root tuple
- **AND** it SHALL describe that proof as bounded launcher rerun safety rather
  than as a generic system-wide process-management guarantee.

#### Scenario: Evidence states active-only scope truthfully
- **WHEN** `active-probe-cleanup-hardening-v1` is written
- **THEN** the evidence SHALL state that the cleanup hardening claim covers only
  the governed active `OBC` / `TopCcsds` verification path
- **AND** it SHALL NOT claim legacy-path cleanup closure, flight-runtime
  behavior changes, or system-wide orphan-process prevention.

### Requirement: Target Recovery Closure Evidence
The verification evidence tree SHALL record reviewable hosted and Raspberry Pi evidence for `target-recovery-closure-v1`, including focused unit coverage, hosted R2/R6 distinction, service-managed target R2 restart proof, OpenSpec validation, and explicit remaining hardware-reset boundaries.

#### Scenario: Focused tests cover R2 restart semantics
- **WHEN** `target-recovery-closure-v1` completes local verification
- **THEN** the evidence SHALL identify tests covering R2 metadata persistence, process-restart pending/count state, R2 versus R6 exit-code separation, boot metadata reload after R2, and repeated-recovery clamp behavior

#### Scenario: Hosted probes prove R2 and R6 distinction
- **WHEN** hosted recovery probes are recorded for this change
- **THEN** the evidence SHALL include the probe command, isolated runtime roots or ports, expected R2 process-restart exit observations, expected R6 reboot-equivalent observations, post-relaunch boot metadata observations, and final verdict

#### Scenario: Raspberry Pi target probe proves service-managed R2 restart
- **WHEN** target evidence is recorded for this change
- **THEN** the evidence SHALL include the service-managed target path, the exact R2 trigger method exercised by the governing evidence, OBC service restart observations, command-path recovery observation, boot metadata readback, and final verdict

#### Scenario: Evidence keeps hardware reset out of scope
- **WHEN** `target-recovery-closure-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove Raspberry Pi hardware watchdog reset, Linux reboot, bootloader or partition handoff, power-loss recovery, RF behavior, or final flight deployment behavior

### Requirement: Legacy Top Retirement Evidence Is Reviewable
The verification evidence tree SHALL record reviewable local evidence for
`legacy-top-retirement-docs-reorg-v1`, including the source/build cleanup,
script and policy cleanup, documentation reorganization, thesis refresh,
OpenSpec validation, and explicit historical-evidence boundary.

#### Scenario: Cleanup evidence records local gates
- **WHEN** `legacy-top-retirement-docs-reorg-v1` records final evidence
- **THEN** the evidence SHALL identify commands that checked for forbidden
  active references, generated and built the active deployment, generated and
  built unit tests, ran the full verification CI gate, and validated the
  OpenSpec change and current specs.

#### Scenario: Historical evidence remains reviewable
- **WHEN** reviewers inspect old evidence records that mention
  `OBC_ComFprimeLegacy`, `OBCAppComFprimeLegacy`, or `OBC/Top`
- **THEN** current evidence and indexes SHALL make clear that those references
  are historical records rather than current build, script, or runtime
  requirements.

### Requirement: Target Hardware Watchdog Reset Evidence

The verification evidence tree SHALL record reviewable capability-gate,
integration, and Raspberry Pi target evidence for
`target-hardware-watchdog-reset-proof-v1`, including proof that the active OBC
service owns the hardware watchdog device, that watchdog-source stale
suppression leads to board reboot, and that existing `R2` process restart truth
remains intact.

#### Scenario: Capability gate records lifecycle compatibility
- **WHEN** `target-hardware-watchdog-reset-proof-v1` completes its capability
  gate
- **THEN** the evidence SHALL record watchdog presence, driver identity, fixed
  timeout behavior, clean-stop observation, reopen observation, and any
  constraints discovered for the active baseline

#### Scenario: Target probe proves hardware watchdog reset
- **WHEN** Raspberry Pi target evidence is recorded for this change
- **THEN** the evidence SHALL include the active service path, the bounded proof
  trigger, the observation that watchdog stroking stopped, SSH disconnect and
  reconnect observations, reboot evidence, service recovery, boot metadata
  readback, persistent fault readback, and final verdict
- **AND** if the proof uses a temporary quiet diagnostic path, the evidence
  SHALL record that quiet mode was probe-owned, that journal-first acceptance
  was used, and that the service was restored to normal non-quiet mode before
  the probe exited

#### Scenario: Evidence proves R2 did not regress
- **WHEN** this change records target watchdog-reset evidence
- **THEN** it SHALL also record a rerun of the existing target `R2`
  process-restart proof or an equivalent focused regression result
- **AND** that evidence SHALL confirm `R2` still uses service-managed restart
  rather than hardware watchdog reset

#### Scenario: Evidence keeps power-loss and external supervisor out of scope
- **WHEN** `target-hardware-watchdog-reset-proof-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove power-loss recovery,
  external supervisor IC behavior, bootloader or partition handoff, secure boot,
  RF behavior, or final flight deployment behavior

### Requirement: Target Node-5 Migration Evidence Is Reviewable
The verification evidence tree SHALL record reviewable target/lab evidence showing that node `5` replaced generic node `4` as the default target COMM path.

#### Scenario: Evidence records target node-5 default proof
- **WHEN** target node-`5` migration evidence is recorded
- **THEN** the evidence SHALL include the target profile used, subsystem node-`5` service identity, target OBC node-`5` runtime configuration, command/readback observations, and final verdict

### Requirement: Target Node-6 Quiet UHF Evidence Is Reviewable
The verification evidence tree SHALL record reviewable target/lab evidence for
node `6` under both `uhf-primary-after-failover` and `uhf-backup` bounded
quiet-mode proofs.

#### Scenario: Evidence records both node-6 role variants
- **WHEN** target node-`6` migration evidence is recorded
- **THEN** the evidence SHALL identify separate bounded results for
  `uhf-primary-after-failover` and `uhf-backup`
- **AND** it SHALL record that quiet mode was probe-owned and not a new nominal operator baseline
- **AND** it SHALL record that operator profile `uhf-primary` evidence used an
  explicit node-`5` switch-to-UHF step before node-`6` command/readback and
  therefore exercised the `uhf-primary-after-failover` runtime role

### Requirement: Target Reboot-Class Evidence Follows Node 5 After Migration
The verification evidence tree SHALL update target reboot-class records so the migrated default node-`5` path is the governing COMM path for those proofs.

#### Scenario: Recovery and watchdog evidence cite node 5
- **WHEN** target `R2` recovery restart or target hardware-watchdog reset evidence is refreshed after migration
- **THEN** the evidence SHALL identify the default node-`5` target path as the governing COMM path for that proof
- **AND** it SHALL keep any separate quiet-mode node-`6` operational evidence as adjacent rather than as the reboot-class baseline

### Requirement: Payload Operation Evidence Is Split By Contract And Hardware Truth

The first payload operation slice SHALL record separate hosted contract proof
and Raspberry Pi target camera proof.

#### Scenario: Hosted proof is explicit about its limit

- **WHEN** the repository records hosted payload verification for this change
- **THEN** the evidence SHALL state that the hosted path proves the governed
  payload contract, sequencing integration, proxy power semantics, and storage
  boundary
- **AND** it SHALL state that hosted proof does not prove real `libcamera`
  sensor interaction

#### Scenario: Target proof is explicit about the real camera path

- **WHEN** the repository records Raspberry Pi target verification for this
  change
- **THEN** the evidence SHALL state the actual camera hardware path, the target
  runtime root, the real capture output, and the proxy EPS channel behavior

### Requirement: Payload evidence distinguishes request settings from actual target AUTO results

Current maintained payload target evidence SHALL separate request and
controller truth from actual camera-runtime truth.

#### Scenario: AUTO to deterministic operator workflow evidence is fresh

- **GIVEN** a governed target payload proof for the current real-camera path
- **WHEN** the proof performs one prepare, an `AUTO` capture, actual metadata
  readback, and a later `DETERMINISTIC` capture without re-prepare
- **THEN** the evidence SHALL record both the request and controller-resolved
  settings and the actual target auto result fields
- **AND** the record SHALL state that the proof demonstrates the operator
  workflow of using `AUTO` to inspect actual values before deterministic replay

### Requirement: Closeout-Ready Status Requires Target Payload Proof

The change SHALL not be treated as closeout-ready on hosted proof alone.

#### Scenario: Missing target proof remains a bounded verification gap

- **WHEN** the change has hosted contract proof but lacks Raspberry Pi target
  camera proof
- **THEN** the evidence SHALL record that gap honestly using the repository's
  constrained-validation language
- **AND** the change SHALL NOT claim full closeout-ready payload verification

### Requirement: Capture Mode V2 Evidence Distinguishes Auto And Deterministic Proof

The verification evidence for payload capture modes v2 SHALL record separate
hosted proof for auto and deterministic capture behavior, capability readback,
and sidecar metadata.

#### Scenario: Hosted capture-mode proof is reviewable

- **WHEN** payload-capture-modes-v2 completes hosted verification
- **THEN** the evidence SHALL record the commands, session kind, capture
  artifact path, sidecar metadata path, and final verdict

### Requirement: Raw Sensor Evidence Distinguishes Hosted Fake And Target Truth

The verification evidence for raw sensor register control SHALL distinguish
hosted contract proof from target real-register proof.

#### Scenario: Hosted proof does not claim real sensor effects

- **WHEN** hosted verification records raw-register command behavior
- **THEN** the evidence SHALL state that it proves only contract, authority, and
  sequence behavior

### Requirement: Payload CSP Service Proof Uses Internal CSP Evidence

The verification evidence for the payload CSP service SHALL identify the
internal CSP path and distinguish it from the public ground ingress path.

#### Scenario: Payload service proof states its boundary

- **WHEN** the repository records payload CSP service evidence
- **THEN** the evidence SHALL identify that the proof covers the OBC-internal
  payload service on node `1` shim ports `34..36` rather than a second ground
  operator plane or a physically split node `7` deployment

### Requirement: Target Hardening Evidence Records Real Camera And Power-Investigation Truth

The verification evidence for target backend hardening SHALL record real target
camera closure and the Raspberry Pi camera power-control investigation result.

#### Scenario: Target camera proof is explicit about the achieved boundary

- **WHEN** payload-target-backend-hardening-v1 records target proof
- **THEN** the evidence SHALL state whether the proof achieved real JPEG
  capture, real register round-trip, and any maintainable power-control path

### Requirement: Target Payload Capture Sanity Evidence Distinguishes Onboard Source Artifacts From Downlink Closure

The verification evidence tree SHALL record a distinct target real-camera
payload sanity proof whenever the repository closes a target source-image
validity defect without rerunning full official downlink closure.

#### Scenario: Target capture sanity evidence stays source-side and reviewable

- **WHEN** the repository records final target capture sanity evidence
- **THEN** reviewers SHALL be able to inspect the governed A/B/C target proof
  command path, the onboard source artifacts fetched from `obc.local`, the
  recorded payload metadata, and the bounded image-content sanity result
- **AND** the record SHALL state explicitly that those source artifacts were
  recovered from target storage rather than received over the ground or
  downlink path

#### Scenario: Route 1 historical closure remains transport-scoped

- **WHEN** adjacent payload or Chapter 5 records reference the older Route 1
  target formal rerun
- **THEN** they SHALL describe that rerun as transport, hash, and downlink
  closure only
- **AND** they SHALL point current source-image validity claims to the
  dedicated target capture sanity record instead of implying that Route 1
  already proved non-black source imagery

### Requirement: Matrix Foundation Metadata Is Stable

The formal communication verification matrix SHALL record a stable
machine-readable case metadata contract so later follow-on changes can close
cells without redefining summary semantics.

#### Scenario: Required case metadata stays present
- **WHEN** a matrix case finishes with pass, fail, or blocked status
- **THEN** its metadata SHALL include the case id, environment, carrier kind,
  verdict, blocker class when applicable, rerun-safety value, and artifact
  root
- **AND** malformed or incomplete metadata SHALL be treated as a harness bug

### Requirement: Matrix Wrapper Intent Is Reviewable

The matrix evidence SHALL distinguish reused probes, narrowed probes, and
bounded blockers so a green cell is not confused with an unimplemented one.

#### Scenario: Blocked and reused cells are distinguishable
- **WHEN** reviewers inspect the matrix summaries or wrapper metadata
- **THEN** they SHALL be able to tell whether the case reused an existing probe,
  narrowed a larger proof, or stayed blocked awaiting a dedicated harness

### Requirement: Historical Matrix Sequence-Subsystem Evidence Records Use Official Sequencing

The historical matrix sequence-subsystem evidence family SHALL preserve that
the cited cells used the official sequence upload, admission, and run path
rather than an ad hoc command bundle.

#### Scenario: Hosted sequence-subsystem evidence is reviewable
- **WHEN** a hosted sequence-subsystem matrix evidence record is cited
- **THEN** the evidence SHALL show sequence upload over the active ground path,
  non-reject validation, `SEQ_RUN(..., WAIT)` success, and EPS plus ADCS
  readback returning through the same path

### Requirement: Historical Matrix Sequence Shape Remains Stable

The first shared sequence-subsystem harness SHALL use a stable read-only
sequence shape so later carrier closures compare like-for-like behavior.

#### Scenario: Same sequence payload is reused across carriers
- **WHEN** hosted, target CAN, or target TCP historical sequence-subsystem cells are
  compared
- **THEN** the evidence SHALL identify the same EPS and ADCS read/status
  sequence shape unless a later governed change expands it explicitly

### Requirement: Target CAN Node-6 Matrix Cells Stay Quiet-Aware

Target CAN node-`6` matrix evidence SHALL preserve the current quiet-UHF
acceptance boundary instead of implying non-quiet background telemetry closure.

#### Scenario: Quiet-UHF target CAN evidence is reviewable
- **WHEN** a target CAN UHF matrix cell passes
- **THEN** the evidence SHALL identify the physical UART southbound path and
  the bounded quiet-UHF acceptance used for that result

### Requirement: Target CAN Failover Evidence Requires Ground-Side Closure

Target CAN failover evidence SHALL prove session continuity from S-band to UHF
primary with ground-side readback rather than local journal evidence alone.

#### Scenario: Failover command continuity is reviewable
- **WHEN** target CAN `failover-command` passes
- **THEN** the evidence SHALL show S-band healthy state, induced loss,
  explicit switch to UHF primary, a reopened session, and successful
  command/readback over the new path

### Requirement: Target TCP Parity Topology Is Explicit

Target TCP matrix evidence SHALL identify the governed three-host parity
topology so later target TCP cells reuse the same launch boundary.

#### Scenario: Three-host target TCP topology is reviewable
- **WHEN** target TCP matrix evidence is recorded
- **THEN** reviewers SHALL be able to identify the macOS ground stack, the
  OBC-only process on `obc.local`, and the COMM plus subsystem services on
  `subsystem.local`

### Requirement: Target TCP UHF Evidence Stays Development-Carrier Scoped

Target TCP node-`6` evidence SHALL remain explicitly scoped to TCP-based
southbound emulation.

#### Scenario: Target TCP UHF proof is not mistaken for physical UART
- **WHEN** target TCP `uhf-primary-*` cells pass
- **THEN** their evidence SHALL identify TCP-based southbound emulation
- **AND** it SHALL NOT describe those results as target physical-UHF closure

### Requirement: Historical Target TCP Sequence Evidence Reuses The Shared Official Helper

Historical target TCP sequence-subsystem evidence SHALL reuse the shared official
sequencing helper so carrier comparisons remain like-for-like.

#### Scenario: Target TCP sequence-subsystem evidence is reviewable
- **WHEN** a target TCP sequence-subsystem evidence record is cited
- **THEN** the evidence SHALL show same-path upload, non-reject validation,
  `SEQ_RUN(..., WAIT)` success, and subsystem readback on the parity topology

### Requirement: Target TCP Failover Evidence Stays Narrow

Target TCP failover evidence SHALL prove command continuity only and SHALL NOT
smuggle file or sequence closure into the same case.

#### Scenario: Target TCP failover command continuity is reviewable
- **WHEN** target TCP `failover-command` passes
- **THEN** the evidence SHALL show S-band healthy state, induced loss, UHF
  primary switch, session reopen, and successful command/readback over the new
  path

### Requirement: Target Direct-Control Matrix Evidence Stays Separate

Target direct-control matrix evidence SHALL remain separate from satcom and
southbound parity claims.

#### Scenario: Target direct-control proof is reviewable
- **WHEN** a target TCP or target CAN direct-control cell passes
- **THEN** the evidence SHALL show the target direct `OBC -> GDS` path with
  isolated artifacts and without routing through `ground_ttc_gateway`, node `5`,
  or node `6`

### Requirement: Target Direct-Control Wrappers Are Rerun-Safe

Dedicated target direct-control wrappers SHALL not recreate the earlier
repo-root sequence alias residue or similar owned artifacts outside their case
roots.

#### Scenario: Target direct-control rerun leaves no repo-root residue
- **WHEN** a target direct-control wrapper is interrupted or rerun
- **THEN** the wrapper SHALL preserve isolated case-owned roots and SHALL NOT
  leave repo-root staging or sequence alias artifacts behind

### Requirement: Formal Comm Verification Matrix Evidence Is Reviewable

The verification evidence tree SHALL preserve reviewable matrix-oriented
evidence for `formal-comm-verification-matrix-v1`, including the environment
run command, per-case verdicts, carrier-kind metadata, artifact roots, and any
explicit blocker classification.

#### Scenario: Hosted, target TCP, and target CAN matrix outputs are reviewable
- **WHEN** `formal-comm-verification-matrix-v1` records evidence
- **THEN** reviewers SHALL be able to inspect a machine-readable summary and a
  human-readable summary for each of the three governed environments
- **AND** each summary SHALL identify the nine formal communication cases
  explicitly rather than relying on historical script names alone

#### Scenario: Carrier provenance remains explicit in matrix evidence
- **WHEN** a matrix case records UHF or S-band evidence
- **THEN** the evidence SHALL distinguish at least direct control, hosted
  serial stand-in, target TCP southbound, and target physical UHF UART carrier
  kinds where applicable
- **AND** it SHALL NOT describe target TCP UHF proof as physical UART closure

#### Scenario: Failed or blocked cells stay truthful
- **WHEN** a matrix case fails or remains blocked
- **THEN** the evidence SHALL record that case as failed or blocked with a
  bounded blocker classification
- **AND** it SHALL NOT promote the corresponding verification-path claim into
  the registry until passing governed evidence exists

### Requirement: Service-Managed Target Timing Evidence Is Reviewable

The verification evidence tree SHALL preserve target timing/WCET records as
historical reviewable evidence after timing-wrapper retirement, and current
maintained closeout flows SHALL NOT treat those wrappers as required gates for
unrelated secure-baseline changes.

#### Scenario: Historical timing records remain reviewable
- **WHEN** reviewers inspect archived target timing evidence
- **THEN** the evidence SHALL continue to record the service-managed path,
  workload windows, and timing observations that were proven on that historical
  path
- **AND** it SHALL remain available for review without claiming current
  maintained gate authority.

### Requirement: Target Timing Evidence Keeps Workload And Residual Gaps Explicit

Historical service-managed target timing evidence SHALL keep its workload
labels and residual non-claims explicit after retirement.

#### Scenario: Historical timing records keep residual limits explicit
- **WHEN** archived timing evidence is cited after this change
- **THEN** it SHALL keep its workload descriptions and residual gaps explicit
- **AND** it SHALL NOT be promoted back into current maintained closeout
  requirements by implication.

### Requirement: Service-Managed Timing Ceiling Freeze Records Control Preflight

Archived timing-ceiling-freeze evidence SHALL remain reviewable for its
original control-preflight and measurement boundary, but current maintained
closeout flows SHALL not require rerunning that wrapper family after timing
retirement.

#### Scenario: Retired timing preflight is not a current maintained gate
- **WHEN** a later unrelated secure-baseline change reaches closeout
- **THEN** verification evidence SHALL NOT require
  `target-timing-empirical-ceiling-freeze-v1` control-preflight reruns as a
  current maintained gate
- **AND** it MAY still cite the archived historical timing record when a
  change explicitly discusses timing ancestry.

### Requirement: Service-Managed Timing Ceiling Freeze Uses Repeated Clean Runs

Archived numeric timing-ceiling evidence SHALL remain reviewable as a retired
historical path rather than a current maintained closeout dependency.

#### Scenario: Retired timing ceiling evidence stays historical
- **WHEN** reviewers inspect the frozen empirical ceiling records after this
  change
- **THEN** they SHALL find those repeated-clean-run records preserved as
  archived historical evidence
- **AND** they SHALL NOT find them required as current maintained proof for
  secure-auth-only command-ingress retirement.

### Requirement: Service-Managed Timing Blocker Evidence Stays Separate And Reviewable

The repository SHALL record narrower product-side timing blockers under
dedicated evidence instead of folding them back into broad timing `TBD`
wording.

#### Scenario: Target-side timing blocker is captured as prerequisite truth

- **WHEN** current Raspberry Pi `obc-comm-csp-stack.service` timing closure is
  blocked by queue backpressure, restart-path instability, or an equivalent
  narrower product-side behavior
- **THEN** the evidence SHALL record that blocker under a dedicated change-level
  evidence root
- **AND** it SHALL identify whether the blocker concerns telemetry backpressure,
  restart stability, or another named prerequisite class
- **AND** it SHALL name the affected workload or restart phase
- **AND** it SHALL state explicitly that final service-managed timing ceilings
  remain pending until that blocker is removed or further narrowed

### Requirement: Policy Clarification Changes Reuse Evidence And Preserve Non-Claims

A policy-clarification change that does not add new transport proof SHALL reuse
existing governed evidence records for any claimed current behavior and SHALL
preserve unproven behavior as non-claims, bounded assumptions, or explicit
future follow-up.

#### Scenario: COMM policy clarification stays evidence-backed without inventing new proof
- **WHEN** a clarification change cites retained legacy `SESSION_OPEN`
  lifecycle,
  target node-`5` or node-`6` behavior, radio or link observability, beacon
  capture, or current file/downlink role boundaries
- **THEN** it SHALL cite the existing governing evidence records rather than a
  chat summary
- **AND** it SHALL NOT claim new reliable-transfer, dual-link runtime, or
  session-aware beacon suppress closure unless fresh evidence is added
- **AND** if the change newly separates UHF primary packet quiet from beacon
  suppress ownership, it SHALL distinguish which part reuses existing beacon
  evidence and which part requires fresh packet-quiet evidence

### Requirement: UHF reliable-transfer evidence proves both success and bounded failure

The verification evidence catalog SHALL require fresh evidence for both success
and bounded degraded behavior before `uhf-reliable-transfer-v1` is treated as
proven.

#### Scenario: Hosted proof covers the bounded switched-UHF claim

- **WHEN** hosted evidence is recorded for `uhf-reliable-transfer-v1`
- **THEN** it SHALL include one happy-path official HK `.fdp` transfer through
  the explicit-switched `uhf-primary-after-failover` node-`6` path
- **AND** it SHALL include at least one degraded ACK/no-progress case that
  triggers resend before final success
- **AND** it SHALL include at least one bounded final failure case with retry
  exhaustion and no final artifact promotion
- **AND** it SHALL state that the `160`-byte data-segment payload ceiling is a
  bounded helper-path transport fact, not a generic repo MTU claim

#### Scenario: Target/lab proof stays bounded to the quiet switched node-`6` path

- **WHEN** target/lab evidence is recorded for `uhf-reliable-transfer-v1`
- **THEN** it SHALL use the current explicit-switched quiet node-`6` governed
  path
- **AND** it SHALL identify quiet mode as probe-owned diagnostic control
- **AND** it SHALL record that proof-only overrides were removed and that the
  services were restored to the normal non-quiet baseline after the proof
- **AND** it SHALL NOT restate that quiet-path proof as nominal non-quiet UHF
  reliable-transfer closure

### Requirement: UHF Beacon Suppress Runtime Evidence States What Was Newly Proven

The verification evidence for `uhf-beacon-suppression-runtime-v1` SHALL record
the exact governed path under test, what was newly proven, and what remains
outside the claim boundary.

#### Scenario: Evidence names the exact new runtime claim
- **WHEN** the change records its final evidence
- **THEN** it SHALL identify that accepted authenticated UHF
  `SESSION_OPEN(seq0)` starts suppress, same-session accepted UHF activity
  refreshes the bounded window, and inactivity timeout or explicit invalidation
  resumes beaconing
- **AND** it SHALL state the owner as `CommController`
- **AND** it SHALL preserve explicit non-claims for simultaneous dual-link
  arbitration, UHF reliable transfer, RF closure, and broader handshake state

### Requirement: UHF Beacon Suppress Runtime Evidence Includes Hosted And Target Proof

The verification evidence for `uhf-beacon-suppression-runtime-v1` SHALL
include both a hosted governed node-`6` proof and a target/lab quiet-UHF
node-`6` proof.

#### Scenario: Hosted proof records start, hold, resume, and a negative case
- **WHEN** the hosted node-`6` suppress/runtime probe passes
- **THEN** the evidence SHALL identify the hosted CCSDS S-band plus UHF node-`6`
  path, baseline beacon visibility before suppress, accepted UHF
  `SESSION_OPEN(seq0)` start, accepted same-session UHF read/status refresh,
  inactivity-timeout resume, and at least one invalid or non-qualifying
  no-suppress result

#### Scenario: Target quiet-UHF proof stays bounded
- **WHEN** the target/lab quiet node-`6` suppress/runtime probe passes
- **THEN** the evidence SHALL identify the physical quiet-UHF path, probe-owned
  beacon capture markers, journal or event evidence for suppress transitions,
  and the bounded quiet-UHF acceptance used for that result
- **AND** it SHALL state that the target proof does not prove non-quiet
  background-TM stability, simultaneous dual-link runtime, UHF reliable
  transfer, or RF behavior

### Requirement: Target Node-6 Non-Quiet Diagnosis Evidence Is Multi-Oracle

The verification evidence tree SHALL record reviewable target node-`6`
non-quiet diagnosis evidence that keeps target truth, ground truth, and byte
transport observations distinct.

#### Scenario: Evidence records quiet control and non-quiet case together
- **WHEN** `target-nonquiet-background-tm-stability-v1` records target
  diagnosis evidence
- **THEN** it SHALL include a quiet target CAN node-`6` control result
- **AND** it SHALL include at least one non-quiet target CAN node-`6` case
- **AND** it SHALL keep those two verdicts adjacent instead of folding them
  into one generic node-`6` claim

#### Scenario: Evidence records all diagnosis truth surfaces
- **WHEN** the non-quiet diagnosis runs
- **THEN** the evidence SHALL record:
  - target journal observations
  - ground `fprime-cli events` observations
  - ground `fprime-cli channels` observations
  - gateway byte-capture artifacts
  - node-`6` beacon/debug capture artifacts when used

#### Scenario: Evidence classifies the residual honestly
- **WHEN** the diagnosis evidence is finalized
- **THEN** it SHALL state whether the residual issue is `oracle`, `runtime`, or
  `mixed`
- **AND** it SHALL name the chosen isolation boundary
- **AND** it SHALL state what was changed
- **AND** it SHALL keep any remaining non-quiet node-`6` non-claims explicit

#### Scenario: Evidence records a degraded oracle case when present
- **WHEN** a ground-only oracle would misclassify or fail the target node-`6`
  non-quiet case
- **THEN** the evidence SHALL record that degraded oracle outcome explicitly
- **AND** it SHALL distinguish that from target-side command truth

### Requirement: Clarification-Only Target Dual-Link Boundary Changes Say When No New Path Was Proven

The verification evidence tree SHALL require a clarification-only change that
freezes the future target-bearing dual-link claim, oracle, and PASS boundary
without adding fresh proof to state explicitly that no new verification path
was proven.

#### Scenario: Clarification-only closeout keeps evidence truth narrow
- **WHEN** the repository closes a clarification-only dual-link claim slice
- **THEN** its final evidence or closeout wording SHALL state explicitly that
  the change proved no new simultaneous target path
- **AND** it SHALL cite reused evidence only for the exact boundaries already
  governed by those records

### Requirement: Future Target-Bearing Dual-Link Evidence Uses A Dual-Verdict Target-First Oracle

The verification evidence tree SHALL require the next implementation-bearing
target-bearing simultaneous dual-link proof to use a target-first mixed oracle
with separate `target-claim` and `operator-observability` verdicts.

#### Scenario: Target claim verdict gives precedence to target command truth
- **WHEN** the future proof records its main target-bearing result
- **THEN** the `target-claim` verdict SHALL use post-switch journal-first
  target command truth as the authoritative acceptance surface
- **AND** ground-only live observability degradation SHALL NOT overturn a
  passing target-side command result by itself

#### Scenario: Operator observability verdict stays separate
- **WHEN** the same future proof evaluates ground events, channels, gateway
  captures, or beacon/debug artifacts
- **THEN** it SHALL record those observations under a separate
  `operator-observability` verdict
- **AND** that verdict MAY be `PASS`, `DEGRADED`, or `FAIL` without rewriting
  the `target-claim` verdict into a single combined result

#### Scenario: Quiet fallback is recorded as adjunct rescue only
- **WHEN** a failed non-quiet node-`6` command attempt is followed by quiet
  node-`6` rescue
- **THEN** the evidence SHALL record quiet fallback as bounded adjunct rescue
- **AND** it SHALL NOT restate that rescue as proof that non-quiet
  operator-observability was clean

#### Scenario: Official file continuity stays adjunct-only when used
- **WHEN** the same future proof includes an official file/downlink continuity
  check
- **THEN** the evidence SHALL record that result as an adjunct outcome
- **AND** it SHALL NOT silently promote that adjunct into a mandatory main PASS
  condition unless a later governed change widens the future claim explicitly

### Requirement: Target Dual-Link Evidence Records Only The Exact Successful Branch

The verification evidence tree SHALL record the first implementation-bearing
target dual-link proof as the exact successful branch exercised by the official
governed run.

#### Scenario: Evidence records the exact successful branch only
- **WHEN** the official target-bearing dual-link proof completes successfully
- **THEN** the evidence SHALL state whether the run landed as:
  - `target-claim=PASS`, `operator-observability=PASS`
  - or `target-claim=PASS`, `operator-observability=DEGRADED`
- **AND** it SHALL NOT describe an unrun degraded or rescue branch as already
  proven

#### Scenario: Evidence records phase-B rescue without widening phase-C truth
- **WHEN** quiet node-`6` rescue is used for phase B
- **THEN** the evidence SHALL record that rescue as phase-B adjunct-only use
- **AND** it SHALL separately show that phase-C switched non-quiet truth still
  passed on its own before the overall target claim was accepted

#### Scenario: Evidence names minimum ground/operator support explicitly
- **WHEN** the same proof publishes its `operator-observability` result
- **THEN** the evidence SHALL identify the exact ground artifacts used for that
  verdict
- **AND** it SHALL state whether ground-side reviewability was clean or
  degraded without collapsing that result into the target-truth verdict

### Requirement: Hosted Per-Band Stock-Stack Operator Baseline Evidence Is Reviewable

The verification evidence tree SHALL record reviewable hosted-first evidence
for the maintained per-band stock-stack operator baseline as distinct S-band
and UHF stock ground surfaces with launcher-owned hosted runtime roots, where
combined mode exposes one shared hosted runtime for both surfaces.

#### Scenario: Evidence records distinct operator surfaces and launcher-owned runtime
- **WHEN** the repository records the maintained hosted operator-baseline proof
- **THEN** the evidence SHALL identify the exact launcher or proof commands,
  the launcher-owned hosted runtime root, per-stack GDS and TTS ports,
  southbound endpoints, file-storage directories, process logs, startup order,
  and final verdict
- **AND** it SHALL state when combined mode uses one shared hosted runtime for
  both exposed stock surfaces
- **AND** it SHALL show that both stock stacks start successfully while
  remaining reviewably distinct

#### Scenario: Evidence records bounded non-interference with unrelated active simulators
- **WHEN** the repository updates or reruns the maintained hosted
  operator-baseline proof after cleanup hardening
- **THEN** the evidence SHALL show that launcher startup and teardown do not
  terminate unrelated active EPS/ADCS simulator runs that are using different
  CSP hub ports
- **AND** it SHALL keep that claim bounded to hosted launcher cleanup ownership
  rather than broad system-wide orphan-process prevention

#### Scenario: Evidence states what is reused versus newly proven
- **WHEN** the same hosted operator-baseline proof depends on existing S-band
  or UHF transport and policy paths
- **THEN** the evidence SHALL identify which adjacent governed paths are reused
  prerequisites and which maintained operator-baseline behavior is newly proven
- **AND** it SHALL avoid restating the proof as new simultaneous runtime
  arbitration, new UHF policy semantics, or new target-bearing transport proof

#### Scenario: Evidence preserves hosted-only scope and explicit non-claims
- **WHEN** the hosted operator-baseline evidence is finalized
- **THEN** it SHALL state that the proof is hosted-only
- **AND** it SHALL keep explicit non-claims for simultaneous dual-link runtime
  arbitration, one-GDS multi-upstream operator behavior, one-gateway
  multiplexer behavior, target/lab simultaneous closure, and RF closure

### Requirement: Hosted Dual-Link Orchestration Owner Evidence Is Reviewable

The verification evidence tree SHALL record reviewable hosted-first evidence
for the layer-2 dual-link orchestration owner as a distinct proof boundary
above the maintained per-band stock-stack baseline.

#### Scenario: Evidence proves orchestration-owned lifecycle and failure state
- **WHEN** the repository records the hosted orchestration-owner proof
- **THEN** the evidence SHALL identify the owner manifest and status artifacts,
  lifecycle transition history, owned process set, shared-runtime interaction,
  and the final verdict
- **AND** it SHALL record a happy-path lifecycle sequence and a bounded
  startup-failure sequence owned by the orchestration surface itself

#### Scenario: Evidence avoids older COMM semantic oracles
- **WHEN** the hosted orchestration-owner evidence is finalized
- **THEN** it SHALL use orchestration-owned manifest, status, failure, and
  cleanup surfaces plus process/listener inspection as its acceptance oracle
- **AND** it SHALL NOT require `run_comm_session_and_downlink_qos_probe.sh`
  or any COMM semantic event or channel oracle to declare orchestration
  success

#### Scenario: Evidence keeps layer-1 and target claims separate
- **WHEN** the same orchestration-owner evidence cites adjacent paths
- **THEN** it SHALL identify `43B` as a separate layer-1 prerequisite and
  rerun it independently as non-regression
- **AND** it SHALL keep explicit non-claims for target-bearing simultaneous
  proof, one-GDS heterogeneous upstream behavior, one-gateway multiplexer
  behavior, RF closure, and any reopening of frozen UHF semantics

### Requirement: Transport Governance Evidence Separates Derivation From Path Scope

The verification evidence catalog SHALL require transport-governance evidence
to distinguish numeric ceiling derivation proof from reused path-scope proof
and from remaining residuals.

#### Scenario: Numeric derivation is reviewable

- **WHEN** evidence is recorded for `transport-mtu-apid-governance-v1`
- **THEN** it SHALL cite the checked-in constants, serializer sizes, and
  formulas used to derive the frozen ceiling values
- **AND** it SHALL identify the resulting current ceilings for
  `sband-primary`, `uhf-backup`, and `uhf-primary-after-failover`

#### Scenario: Reused path evidence is not overstated as numeric proof

- **WHEN** the evidence cites hosted or target/lab CCSDS records
- **THEN** it SHALL use those records to identify the operational path or APID
  flow scope that reuses the frozen contract
- **AND** it SHALL NOT claim that those reused path records directly measured
  the numeric ceiling values unless the evidence actually did so

#### Scenario: Residuals and skill-audit verdict stay explicit

- **WHEN** the transport governance evidence is finalized
- **THEN** it SHALL state which broader MTU, APID-expansion, or
  reliable-transfer questions remain intentionally unfrozen
- **AND** it SHALL state whether `.codex/skills/change-closeout/SKILL.md`
  required clarification, with a concrete reason either way

### Requirement: Reliable-Transfer Evidence Keeps The `160`-Byte Ceiling Narrow

The verification evidence catalog SHALL keep the `160`-byte
reliable-transfer `DATA` segment ceiling scoped to the bounded helper
path instead of letting that number become a generic transport claim.

#### Scenario: Hosted and target evidence state the bounded segment scope

- **WHEN** evidence is recorded for `uhf-reliable-transfer-v1`
- **THEN** it SHALL state that the `160`-byte data-segment payload ceiling is a
  bounded helper-path transport fact, not a generic repo MTU claim

### Requirement: Target Secure Auth Evidence Is Reviewable

Verification evidence for `target-secure-auth-proof-v1` SHALL record the target
commands, proof root, provenance gates, gateway captures, target journal
snapshots, checkpoint stream, summary JSON, cleanup status, and final verdict.

#### Scenario: Evidence records installed release provenance
- **WHEN** target secure-auth evidence is recorded
- **THEN** it SHALL include the installed release `current` symlink, service
  `WorkingDirectory`, bundled `config/security/command-auth.ini` SHA,
  manifest SHA, expected target and subsystem service identities, and absence
  of forbidden `COMMAND_AUTH_*` service environment injection and
  `--command-auth*` CLI injection
- **AND** it SHALL state whether the existing installed release was accepted or
  whether sync/bootstrap/package/install was rerun.

#### Scenario: Evidence records S-band target proof cases
- **WHEN** target S-band secure-auth evidence is recorded
- **THEN** it SHALL include APID `0x00FE` challenge auth, secure command v2 on
  the command APID, non-`1` first accepted sequence, strict next-sequence
  rejection, malformed handshake fail-closed behavior, and
  `.sequence-staging/<leaf>` upload admission after secure auth.

#### Scenario: Evidence records bounded UHF target proof cases
- **WHEN** target UHF secure-auth evidence is recorded
- **THEN** it SHALL include physical node-`6` `ServiceID = 2` auth,
  `uhf-backup` read/status acceptance, `uhf-backup` high-authority denial,
  `uhf-backup` staged-upload denial, switch invalidation of old UHF
  auth/session state, and re-auth before `uhf-primary-after-failover`
  secure-command acceptance
- **AND** it SHALL state that UHF primary staged-upload success remains outside
  this change.

#### Scenario: Failed target proof is classified before product changes
- **WHEN** the target proof fails
- **THEN** the evidence SHALL classify the failure first as provenance,
  environment, or probe-oracle failure before treating it as a product defect.

### Requirement: Target Node-5 RG3 Salvage Evidence

The verification evidence tree SHALL record the salvaged node-`5` RG3
contention classification, bounded product fix, and clean-branch verification
record under
`evidence/records/target-node5-rg3-csp-runtime-contention-fix-v1/`.

#### Scenario: Evidence identifies both diagnosis provenance and clean rerun

- **WHEN** the repository salvages the service-managed node-`5` RG3 contention
  fix onto a clean branch
- **THEN** the evidence SHALL identify the source-branch diagnosis provenance
  used to classify the blocker
- **AND** it SHALL identify the clean-branch verification command and artifact
  root used to confirm the bounded fix without the dirty vendored timing patch

#### Scenario: Evidence keeps the claim bounded to RG3 closure

- **WHEN** reviewers inspect that evidence record
- **THEN** it SHALL summarize the reproduced RG3 slip behavior before and after
  the fix
- **AND** it SHALL explicitly keep any remaining RG1 or broader CSP client
  contention as separate follow-up work

### Requirement: Node-5 Observability Proof Oracles Stay Packet-Path Grounded
Maintained hosted and target node-`5` observability-governance evidence SHALL
prove representative bounded detailed readback and S-band close on the
packetized path itself, not only through passive observer quiescence.

#### Scenario: Hosted and target bounded detailed readback stays bounded on the packet path
- **WHEN** hosted or target node-`5` observability evidence records the
  representative authenticated `EPS_GET_STATUS -> EPS_POWER_OUT` detailed readback
- **THEN** the evidence SHALL show one auditable detailed readback on the
  maintained path
- **AND** it SHALL keep that detailed readback tied to a bounded
  command-specific capture or readback artifact on the maintained packet path
  instead of relying only on a long-running passive channel listener.

#### Scenario: Hosted and target switch-close quiet stays packet-path grounded
- **WHEN** the same hosted or target observability evidence records close on a
  primary switch away from S-band
- **THEN** the evidence SHALL show that the maintained downlink or gateway
  capture stops growing after the close condition
- **AND** it SHALL not accept passive-listener silence by itself as sufficient
  proof of packet-path quiet.

### Requirement: Target Secure-Auth Handshake Recovery May Use Source-Aware Progress
Maintained target secure-auth evidence SHALL allow handshake observation to
advance on the governed wire capture or native packet-log surface, provided the
evidence still records which source confirmed each step and preserves the same
target path identity.

#### Scenario: Evidence records source-aware handshake confirmation
- **WHEN** target secure-auth evidence records a challenge or auth-status step
  on a maintained path
- **THEN** it SHALL record whether the confirming surface was wire capture,
  native packet log, or target journal
- **AND** it SHALL keep the result tied to the same target secure-auth path
  rather than inventing a parallel proof family.

### Requirement: Integrated Route Evidence May Be Staged

The verification evidence tree SHALL allow one integrated route verdict to be
constructed from multiple repo-owned staged scripts or from a repo-owned
command playbook when one monolithic probe would reduce determinism or make the
observability surface ambiguous.

#### Scenario: Staged route evidence records aggregation explicitly
- **WHEN** one Chapter 5 route is proven by multiple staged scripts
- **THEN** the evidence SHALL identify each contributing script, its exact path
  boundary, the artifacts it produced, and the aggregation rule that yields the
  route verdict

#### Scenario: Command-playbook fallback remains reviewable
- **WHEN** a Chapter 5 segment is first proven through a repo-owned stepwise
  command playbook instead of a fully automated probe
- **THEN** the evidence SHALL record the exact commands, expected observations,
  artifact paths, and final verdict
- **AND** it SHALL NOT leave the proof surface only in chat history

### Requirement: Chapter 5 Route Evidence Keeps Mission Console Packet-Lab Separate

Chapter 5 evidence SHALL reuse the maintained Mission Console baseline-attach
pattern only as an adjacent prerequisite and SHALL keep Mission Console
`packet-lab negative evidence` separate from Chapter 5 route verdicts unless a
later change proves a direct dependency.

#### Scenario: Chapter 5 evidence does not treat packet-lab oracle as prerequisite
- **WHEN** a Chapter 5 hosted or target route probe is reviewed
- **THEN** the evidence SHALL state whether Mission Console baseline attach is a
  reused prerequisite
- **AND** it SHALL keep Mission Console packet-lab negative evidence out of the
  route verdict boundary unless that route explicitly tests that oracle

### Requirement: Non-Quiet UHF Primary Runtime Evidence Uses Stability Counts

The verification evidence tree SHALL record fresh target/lab evidence for the
current non-quiet UHF primary runtime using fixed repeated and interleaved
command stability counts.

#### Scenario: Evidence records repeated and interleaved command counts
- **WHEN** `uhf-primary-nonquiet-runtime-v1` evidence is recorded
- **THEN** it SHALL include one repeated single-command case and one interleaved
  dual-command case
- **AND** it SHALL record `10` attempts per subcase, the inter-command spacing,
  success counts, and the final per-subcase verdict.

#### Scenario: Evidence classifies failures instead of collapsing them
- **WHEN** any repetition fails or is degraded
- **THEN** the evidence SHALL classify the result as target runtime failure,
  ground observability failure, or environment/baseline failure
- **AND** it SHALL report those counts explicitly instead of only one overall
  summary line.

### Requirement: Autonomous UHF Failover Evidence Is Reviewable

The verification evidence tree SHALL record reviewable target/lab evidence for
detector-triggered autonomous UHF failover using a governed shared-service
unavailable-window helper.

#### Scenario: Evidence records the governed unavailable window and failover markers
- **WHEN** `target-autonomous-uhf-failover-v1` evidence is recorded
- **THEN** it SHALL identify how `subsystem-sband-csp.service` was stopped and
  restored, the detector and failover markers observed, the target artifact
  roots, and the final verdict.

#### Scenario: Evidence records post-failover re-auth and readback
- **WHEN** autonomous failover evidence records a PASS
- **THEN** it SHALL include UHF secure-auth re-bootstrap, bounded
  `GET_RESET_CAUSE`, bounded `GET_PERSISTENT_FAULT_HISTORY`, and the observed
  post-auth beacon suppress start
- **AND** it SHALL state whether live `event/tlm` and readback were visible on
  the UHF ground path.

### Requirement: Target Watchdog Evidence No Longer Depends On Quiet UHF Baseline

The verification evidence for the current target watchdog-reset proof SHALL use
the maintained secure-auth command-path family without requiring a probe-owned
quiet packet-egress overlay.

#### Scenario: Watchdog evidence records non-quiet post-reboot closure
- **WHEN** target watchdog-reset evidence is refreshed for this change
- **THEN** it SHALL record pre-trigger secure-auth readiness, the reboot edge,
  post-reboot secure-auth re-bootstrap, and final readback on the maintained
  non-quiet baseline
- **AND** it SHALL not require `DIAGNOSTIC_QUIET_PACKET_EGRESS=1` as a current
  acceptance dependency.

### Requirement: Payload persistent-session evidence distinguishes runtime lifecycle from historical single-capture warm-up evidence

The verification evidence tree SHALL record a distinct evidence package for the
current payload persistent-session lifecycle and SHALL keep it separate from the
older single-capture warm-up target proof.

#### Scenario: Persistent-session evidence records one-prepare multi-capture truth

- **WHEN** the repository records current persistent-session payload evidence
- **THEN** that evidence SHALL include the commands, observed state/readback,
  and final verdict for one `PAYLOAD_PREPARE` followed by at least
  `AUTO -> metadata -> DETERMINISTIC` on the same non-RAW session
- **AND** it SHALL state explicitly that repeated maintained captures did not
  require a second full cold-start warm-up lifecycle

#### Scenario: Persistent-session evidence records mismatch rejection

- **WHEN** the same evidence package checks non-RAW session-level mismatch
- **THEN** it SHALL record the command path and verdict showing that the runtime
  rejects the mismatch instead of silently re-preparing

### Requirement: Payload retirement evidence records current STILL removal boundary

The verification evidence tree SHALL record the retirement boundary for
`PAYLOAD_CAPTURE_STILL` so reviewers can distinguish current maintained payload
capture truth from historical compatibility evidence.

#### Scenario: Current evidence points only to AUTO and DETERMINISTIC

- **WHEN** reviewers inspect the current payload capture evidence after this
  change
- **THEN** the record SHALL identify `AUTO` and `DETERMINISTIC` as the only
  maintained normal still-capture policies
- **AND** it SHALL classify any retained `PAYLOAD_CAPTURE_STILL` records as
  historical evidence only

### Requirement: Route 1 Sequence Evidence SHALL Be Independently Reviewable

Route 1 sequence verification evidence SHALL be stored in a dedicated test
record. It SHALL record the exact checked-in sequence source, wrapper/probe
commands, build and revision provenance, hosted or target execution surface,
command and completion observations, final verdict, and explicit deferred
boundaries. Debugging lessons may be cited as diagnostic history but SHALL not
be the sole PASS authority.

#### Scenario: Hosted evidence records the full functional chain
- **WHEN** hosted Route 1 evidence is recorded
- **THEN** it SHALL record isolated runtime inputs, compile/upload/validate/run
  results, SoC and mode observations, fresh payload-completion evidence, and
  rejected stale or failure conditions
- **AND** it SHALL distinguish the hosted path from target filesystem and
  ground-observation claims.

#### Scenario: Target evidence records A/B/C and provenance
- **WHEN** target Route 1 evidence is recorded
- **THEN** it SHALL record A and B preflight results, C-owned functional
  evidence, local and remote provenance, target and ground observation
  sources, and A/B postflight results
- **AND** it SHALL state that C did not restart or stop shared baseline
  services.

### Requirement: Evidence Governance SHALL Permit Manifest-Backed Canonicalization

Repository evidence governance SHALL permit omission of byte-identical
derived decode outputs when a checked manifest identifies one retained
canonical artifact and every removed equivalent by repository-relative path
and SHA-256. Canonicalization SHALL NOT remove original transport bytes,
source or received product bytes, provenance, attempt history, verdict
oracles, or time-distinct observations. The manifest SHALL provide the source
and applicable received product hashes and a repository-owned reconstruction
command. When the governed observation claims DETERMINISTIC ground receipt,
its received FDP entry and hash SHALL be mandatory; a policy without a
ground-received product MAY record `null`.

#### Scenario: Exact decoded JSON copies are canonicalized

- **WHEN** multiple derived decoded JSON artifacts have the same SHA-256
- **THEN** the evidence bundle MAY retain one canonical `decode-primary`
  artifact and omit the byte-identical equivalents
- **AND** the manifest SHALL record the canonical path, content hash, every
  omitted equivalent path, raw source and received product hashes, and the
  reconstruction command.

#### Scenario: Claimed deterministic ground product remains mandatory

- **WHEN** the canonicalization manifest represents the Route 1
  DETERMINISTIC preview received by GDS
- **THEN** it SHALL identify and SHA-256-check both the source FDP and the
  ground-received FDP
- **AND** replacing the received entry with `null` or removing that raw product
  SHALL fail evidence validation.

#### Scenario: Distinct ground consumers remain independently reviewable

- **WHEN** GDS runtime and an independently connected StandardPipeline store
  byte-identical received FDP products
- **THEN** both received-product observations SHALL remain retained and
  SHA-256-checked
- **AND** byte identity SHALL NOT canonicalize away either ground consumer.

#### Scenario: Non-derived and provenance evidence stays retained

- **WHEN** an evidence bundle is canonicalized
- **THEN** original transport bytes, source and received FDP products, verdict
  summaries, attempt provenance, exact sequence source and compiled execution
  inputs, journals, and required observation logs SHALL remain present
- **AND** content identity alone SHALL NOT justify removing time-distinct
  snapshots or observations.

#### Scenario: Distinct ground receive surfaces remain hash-locked

- **WHEN** a frozen Route 1 bundle retains directional gateway captures,
  native-CLI receive bytes, pipeline receive bytes, and fallback-stage CLI
  receive bytes
- **THEN** each distinct raw transport observation SHALL be enumerated and
  SHA-256-checked
- **AND** deleting or altering any one capture SHALL fail evidence validation.

#### Scenario: Pipeline observations remain hash-locked

- **WHEN** a frozen Route 1 bundle retains StandardPipeline channel and event
  observations alongside its raw receive capture
- **THEN** both observation logs SHALL be enumerated and SHA-256-checked
- **AND** deleting or altering either log SHALL fail evidence validation.

#### Scenario: Frozen sequence execution inputs remain hash-locked

- **WHEN** a frozen Route 1 bundle claims a sequence-driven observation
- **THEN** its exact retained sequence source and compiled execution binary
  SHALL be enumerated and SHA-256-checked
- **AND** deleting or altering either input SHALL fail evidence validation.

#### Scenario: Governing evidence authority remains explicit

- **WHEN** a frozen Route 1 bundle identifies governing documents
- **THEN** its manifest and checker SHALL require each document to classify
  the provenance-incomplete 2026-07-12 proof as historical functional
  evidence, not current target authority
- **AND** each document SHALL explicitly classify the 2026-07-20 bundle as a
  non-authoritative functional observation and target requalification as
  pending rather than relying on date or link presence alone
- **AND** coordinated document and manifest edits SHALL fail when they replace
  an explicit non-authority phrase with an authority-promoting phrase that
  merely contains the character sequence `not`
- **AND** retaining the approved assertion while adding a contradictory
  authority-promoting statement SHALL also fail
- **AND** double-negated non-authority wording and a later positive authority
  clause, including one joined by a coordinating conjunction, SHALL fail
  independently instead of inheriting negation across clause boundaries
- **AND** contrast/subordination forms such as `while` or `although`, and
  positive predicates such as `remains authoritative`, SHALL receive the same
  independent treatment
- **AND** common numeric, English month-name, or Chinese spellings of the two
  governed dates SHALL receive the same authority-promotion checks.

#### Scenario: Frozen campaign README classification remains hash-locked

- **WHEN** the frozen Route 1 campaign README states the bundle's evidence
  classification
- **THEN** the README SHALL be enumerated and SHA-256-checked as a retained
  artifact
- **AND** the checker SHALL require an explicit non-authoritative
  classification and pending target requalification
- **AND** adding an authority-promoting statement SHALL fail validation even
  if the required non-authoritative substrings are still present.

### Requirement: Formal Evidence Attempts SHALL Preserve Failure Lineage

The formal artifact importer SHALL support attempt labels, retry lineage, and
failure classification so a failed or blocked attempt remains distinguishable
from the later governing PASS. A resumable formal campaign SHALL record one
campaign source branch/head/version identity and SHALL validate it before
retaining earlier attempts. On resume, the identity file SHALL also match the
branch/head/version already recorded in the existing campaign manifest before
those attempts are loaded. An attempt label SHALL be either empty or one safe
path component below its route/surface directory.

#### Scenario: Retry metadata is imported

- **WHEN** formal evidence is imported with `--attempt-label`, `--retry-of`,
  or `--failure-class`
- **THEN** the generated evidence metadata SHALL retain the supplied values
- **AND** a later PASS SHALL NOT erase or relabel the earlier attempt lineage
- **AND** every target attempt SHALL retain an attempt-specific revision
  provenance path and SHA-256 that remains independently verifiable after a
  retry.

#### Scenario: Attempt label cannot escape its surface

- **WHEN** the importer receives an absolute, traversal-containing, or
  multi-component attempt label
- **THEN** it SHALL reject the import before creating or copying artifacts
- **AND** no path outside the requested route/surface directory SHALL be
  written.

#### Scenario: Resume does not combine revisions

- **WHEN** a formal campaign resumes with retained attempts
- **THEN** its recorded campaign source branch, head, and project version
  SHALL match the current checkout before those attempts are loaded
- **AND** a mismatch or missing campaign identity SHALL block resume rather
  than combine evidence from different revisions.

#### Scenario: Fresh deployment prepares submodules before identity

- **WHEN** a fresh formal deployment starts from an otherwise clean checkout
  whose recursive submodules are uninitialized or revision-drifted
- **THEN** the runner SHALL initialize and align those submodules before
  recording campaign source identity
- **AND** this preparation SHALL NOT widen `SKIP_DEPLOY` or resume behavior.

#### Scenario: Campaign identity remains stable between surfaces

- **WHEN** hosted execution completes before a target attempt
- **THEN** the runner SHALL compare the current checkout with the recorded
  campaign identity again before target restart or provenance validation
- **AND** a mismatch SHALL block target execution rather than combine hosted
  and target evidence from different revisions.

#### Scenario: Resume identity matches retained campaign history

- **WHEN** a campaign resumes with existing attempt records
- **THEN** the branch/head/version in its campaign identity file SHALL match
  the identity already recorded in the retained campaign manifest before any
  attempt is loaded
- **AND** replacing either identity surface SHALL block resume rather than
  relabel retained attempts.

#### Scenario: Target workspace bytes remain bound to campaign source

- **WHEN** formal target evidence uses a workspace without `.git`
- **THEN** its provenance SHALL retain a deterministic manifest and SHA-256
  over every serialized Git-indexed path, normalized mode, and content
- **AND** those serialized bytes and modes SHALL be compared with the
  committed superproject/submodule trees before synchronization so index
  stat hints, skip-worktree flags, or file-mode configuration cannot relabel
  uncommitted inputs as campaign-HEAD content
- **AND** the target gate SHALL recompute that digest before C and reject a
  changed, missing, unsafe, duplicated, or omitted path
- **AND** the workspace marker SHALL bind a package-path-keyed build-time hash
  for every remote-build input copied into the target bundle so a later
  mutually consistent build/package/install hash chain cannot relabel any
  replaced executable, dictionary, or build metadata as campaign-HEAD
  evidence
- **AND** the live remote build-input audit and installed manifest entries
  SHALL match those marker-time hashes before target C.

#### Scenario: Resume does not rebind a retained target PASS

- **WHEN** a campaign resumes with an existing target PASS
- **THEN** that attempt's authoritative status SHALL remain bound to its
  retained target revision provenance by an attempt-recorded SHA-256
- **AND** the importer manifest itself SHALL be bound by an attempt-recorded
  SHA-256 and enumerate the exact retained artifact file set, sizes, and
  SHA-256 values
- **AND** resume SHALL validate the manifest metadata, artifact roots, and
  every enumerated byte before skipping hosted or target execution
- **AND** a current install, rebuild, or provenance refresh SHALL NOT replace
  the retained provenance unless a new target attempt is executed
- **AND** a missing, corrupt, changed, or unlisted retained artifact SHALL
  prevent authoritative campaign PASS.

### Requirement: Public Evidence Separates Summary From Raw Artifacts
Public test-record summaries, results, and provenance SHALL remain in Git under
`evidence/records/`, while raw `artifacts/` trees SHALL be distributed through
a checksummed release asset.

#### Scenario: Test record has raw artifacts
- **WHEN** a source test record includes an `artifacts/` subtree
- **THEN** its public `evidence/records/<id>/` directory SHALL contain an
  `ARTIFACTS.json` descriptor
- **AND** `evidence/catalog.json` SHALL bind the record and descriptor to the
  release asset
- **AND** source and public digests SHALL remain machine-verifiable

### Requirement: Evidence Redaction Is Reviewable
Text evidence sanitization SHALL use deterministic rules and SHALL record both
source and public digests.

#### Scenario: Personal environment text is replaced
- **WHEN** a source text artifact contains a personal path, account, private IP,
  or serial identifier
- **THEN** the public artifact SHALL use the declared role placeholder
- **AND** the redaction ledger SHALL record original and sanitized SHA-256

### Requirement: Reused Hardware Evidence Does Not Become A Fresh Tag Claim
Target evidence gathered before the public release commit SHALL retain its
original scope and SHALL NOT be promoted to fresh release verification.

#### Scenario: Release delta affects a target claim
- **WHEN** public curation changes a runtime, packaging, protocol, or oracle
  surface used by an old target proof
- **THEN** that proof SHALL be labeled historical for the public release
