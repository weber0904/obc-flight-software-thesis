# Hosted Per-Band Stock Ground Stacks Runbook

Status: current hosted operator runbook.
Last reconciled during `per-band-stock-ground-stacks-v1` and
`hosted-simulator-stale-reap-safety-v1`, plus
`sband-auth-gated-observability-v1` on 2026-06-05.

This runbook is the operator entrypoint for the maintained hosted near-term
simultaneous multi-band baseline:

```text
stock fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> shared hosted OBC / TopCcsds
stock fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> uhf_comm_csp_node(node 6) -> shared hosted OBC / TopCcsds
```

The current repo-owned answer is separate per-band stock stacks on one shared
hosted runtime. A combined wrapper may start and stop both stacks together, but
it is composition-only and is not a new orchestration policy owner.

## Scope

This runbook covers:

- one maintained launcher for the hosted S-band stock stack
- one maintained launcher for the hosted UHF stock stack
- one combined maintained wrapper that starts/stops both stacks together
- the reviewable manifest truth for ports, southbound endpoints, file stores,
  runtime roots, startup order, shutdown order, and owned logs
- the hosted-first repository-owned proof for this maintained baseline

This runbook does **not** cover:

- one stock GDS consuming heterogeneous upstream feeds
- one `ground_ttc_gateway` instance multiplexing simultaneous S-band and UHF
- higher-level simultaneous operator policy or runtime arbitration
- target/lab simultaneous S-band plus UHF proof
- RF closure
- UHF reliable-transfer redesign

## Preconditions

- repository built locally from `${REPO_ROOT}`
- `fprime-gds`, `fprime-cli`, and Python available from `fprime-venv/bin`
- current `OBC` dictionary present, typically:

```text
build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json
```

- current native hosted binaries present, typically under:

```text
build-artifacts/Darwin/bin
```

If build outputs are missing, run:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build
```

## 1. Choose The Maintained Entrypoint

Available entrypoints:

- S-band only:

  ```bash
  PER_BAND_STACK_ROOT=/tmp/per-band-sband \
  PER_BAND_RUNTIME_ROOT=/tmp/per-band-runtime \
  PER_BAND_AUTO_PORTS=1 \
  bash scripts/run_hosted_sband_stock_ground_stack.sh
  ```

- UHF only:

  ```bash
  PER_BAND_STACK_ROOT=/tmp/per-band-uhf \
  PER_BAND_RUNTIME_ROOT=/tmp/per-band-runtime \
  PER_BAND_AUTO_PORTS=1 \
  bash scripts/run_hosted_uhf_stock_ground_stack.sh
  ```

- Combined start/stop wrapper:

  ```bash
  PER_BAND_STACK_ROOT=/tmp/per-band-combined \
  PER_BAND_RUNTIME_ROOT=/tmp/per-band-runtime \
  PER_BAND_AUTO_PORTS=1 \
  bash scripts/run_hosted_per_band_stock_ground_stacks.sh
  ```

Recommended knobs:

- `PER_BAND_AUTO_PORTS=1`: allocate fresh local GDS/TTS/TCP/ZMQ ports
- `PER_BAND_STACK_ROOT=<path>`: keep per-run manifests, logs, captures, and
  GDS file stores under a reviewable owned root
- `PER_BAND_RUNTIME_ROOT=<path>`: provide an owned runtime namespace root.
  When the path lives outside `PER_BAND_STACK_ROOT`, each launcher owns
  `<path>/<mode>/` so per-band entrypoints cannot wipe one another's runtime
  state. In combined mode that owned subdirectory is the shared hosted
  `TopCcsds` runtime for both exposed operator surfaces.
- `STACK_HOLD_SECS=<n>`: hold the launcher open for a bounded number of seconds
  before automatic cleanup; omit it to keep the stack running until interrupted

## 2. Interpret The Maintained Operator Truth

Every launcher writes `<stack-root>/manifest.json` and prints a compact summary.

Review these manifest fields:

- `requestedRuntimeRoot`: optional caller-provided runtime namespace root
- `sharedHostedRuntime`: launcher-owned hosted `TopCcsds` runtime root; in
  combined mode it is shared by both exposed operator surfaces
- `internalOnlyRuntime.cspHubSubPort`, `cspHubPubPort`, `radioMockPort`, and
  `sbandCommTcpPort` when present
- `operatorSurfaces.sband.gdsPort` and `gdsTtsPort`
- `operatorSurfaces.uhf.gdsPort` and `gdsTtsPort`
- `operatorSurfaces.<band>.southbound`
- `operatorSurfaces.<band>.fileStorageDir`
- `operatorSurfaces.<band>.logRoot`
- `startupOrder`
- `shutdownOrder`
- `combinedWrapperTruth`
- `nonClaims`

Current operator meaning:

- S-band surface: nominal high-authority formal TT&C path
- S-band startup/live truth: quiet until accepted S-band secure auth, then
  packetized live `event/tlm` remains open only during that authenticated
  session
- UHF surface: bounded backup ingress until explicit switch
- Combined wrapper: coordinated start/stop for two distinct stock stacks only

Current observability tiers on the maintained hosted stock-stack baseline:

- always-on critical:
  command/auth closure plumbing plus the separate UHF beacon surface
- keep-live summary:
  packetized node-`5` S-band `event/tlm` stays curated after auth rather than
  reopening the former wholesale scheduled telemetry surface
- fresh bounded readback:
  component-owned `GET_*` / read-status commands now trigger fresh detailed
  bounded readback for `EPS`, `GPS`, `ADCS`, `RADIO`, and `STORAGE`
- onboard cached truth:
  mode/boot/reset/fault-history style readback remains cached by design
- reviewable proof / transport / policy observability:
  `GROUND_LINK_UP/DOWN`, `GROUND_LINK_TX_BYTES`, transport-error growth,
  `GROUND_LINK_HEALTH_S_BAND_*`, `QueueOverflow`,
  `CSP_OWNER_TIMEOUT` / `CSP_OWNER_TOTAL_TIMEOUTS`, and S-band
  `CommEgressMux` counters remain formal reviewable observability, but they
  are not the pass-time keep-live summary
- diagnostics-only / non-baseline live:
  residual runtime telemetry such as `SystemResources.*`, queue-depth
  counters, UART byte-stream counters, and timing-only support telemetry such
  as `OBCApp.storageHealthBridge.STORAGE_SCHED_TICKS` may still exist, but
  they are not part of the hosted operator baseline

## 3. Review Startup And Cleanup Ownership

Current startup order is:

1. prepare owned stack/runtime roots
2. start exposed stock GDS surfaces
3. start hosted CSP proxy and hosted subsystem simulators
4. start hosted S-band node `5` and hosted UHF node `6` southbound helpers as needed
5. start exposed `ground_ttc_gateway` bindings
6. start the shared hosted `OBC/TopCcsds` runtime

Current shutdown order is:

1. stop the shared hosted `OBC/TopCcsds` runtime
2. stop exposed `ground_ttc_gateway` bindings
3. stop COMM-node southbound helpers and hosted subsystem simulators
4. stop exposed stock GDS surfaces
5. reap owned stale listeners and helper processes

The wrapper owns cleanup for the processes it starts. It does not become a
runtime policy owner for dual-link arbitration.

For shared hosted EPS/ADCS simulator identities, stale cleanup is narrower than
full process ownership: the maintained launchers only reap orphaned leftovers
when the simulator command line does not otherwise prove ownership. Starting
one maintained launcher must not terminate another still-active hosted stack or
probe that is using different CSP hub ports.

## 4. Use The Hosted Proof

Repository-owned proof for this maintained baseline:

```bash
bash scripts/run_per_band_stock_ground_stacks_hosted_probe.sh
```

That proof verifies:

- S-band launcher start/stop and manifest output
- UHF launcher start/stop and manifest output
- combined wrapper start/stop and manifest output
- distinct per-band GDS/TTS ports, file stores, logs, and southbound endpoints
- owned listener cleanup after launcher exit, including internal CSP, radio,
  and S-band COMM listener ports
- launcher reruns do not terminate an unrelated active EPS/ADCS
  simulator-backed hosted stack on different CSP hub ports

Adjacent behavior remains under separate proof ownership:

- `bash scripts/run_ccsds_sband_hosted_adoption_probe.sh`
  - retained historical hosted wrapper only; not a current maintained
    secure-auth gate
- `bash scripts/run_uhf_ccsds_hosted_adoption_probe.sh`
  - retained historical hosted wrapper only; not a current maintained
    secure-auth gate
- `bash scripts/run_comm_session_and_downlink_qos_probe.sh`
  - retained historical hosted wrapper only; not a current maintained
    secure-auth gate
- `bash scripts/run_sband_observability_governance_hosted_probe.sh`
- `bash scripts/run_uhf_beacon_suppression_hosted_probe.sh`
  - retained historical hosted wrapper only; not a current maintained
    secure-auth gate
- `bash scripts/run_uhf_primary_packet_quiet_hosted_probe.sh`
  - retained historical hosted proof only; not a current maintained baseline
    gate

Do not cite those proof surfaces as if they were the maintained operator
entrypoint. They remain bounded evidence for adjacent S-band/UHF behavior.

## 5. Truthful Limits

- The maintained baseline is hosted-first only.
- The UHF stock stack does not itself claim target-bearing simultaneous proof.
- The maintained baseline does not claim one-GDS aggregation, one-gateway
  multiplexing, or concurrent dual-link runtime arbitration.
- Current UHF semantics on the maintained hosted baseline are:
  - node-`5` S-band live `event/tlm` is auth-gated rather than ambient
    startup chatter
  - content selection inside that auth-gated node-`5` stream is still a
    separate follow-up, not part of the current hosted stock-stack baseline
  - maintained `UHF primary` is non-quiet for live packet egress
  - beacon suppress remains accepted-session-driven
  - historical packet-quiet proofs remain archived compatibility evidence only
  - official file/data-product downlink remains formal on the maintained
    hosted UHF-primary path
- The hosted thin lifecycle owner added by
  [hosted-dual-link-orchestration-runbook.md](hosted-dual-link-orchestration-runbook.md)
  now layers above this maintained baseline without changing its meaning.
