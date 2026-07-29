## 1. OpenSpec And Contract Definition

- [x] 1.1 Create the `radio-metrics-v1` proposal, design, and delta specs for
  `comm-subsystem` and `interface-contract-index`
- [x] 1.2 Freeze the intended field set, ownership boundaries, units,
  freshness, and unavailable semantics in the OpenSpec artifacts before code
  changes diverge

## 2. Runtime Contract And Component Updates

- [x] 2.1 Extend the OBC-owned raw ground-link observation contract and update
  backend implementations to populate the full field set
- [x] 2.2 Add the cached radio observation runtime contract, freshness aging,
  and unavailable-result semantics in `RadioController`
- [x] 2.3 Add band-aware raw ground-link observation getters and cached radio
  observation getters through the runtime service layer and hosted status
  readback

## 3. Verification Surfaces

- [x] 3.1 Update `GroundLinkDriver`, `GroundLinkHealthProvider`,
  `CommController`, `RadioController`, and `HostedRuntime` tests for the new
  observability contract
- [x] 3.2 Add `scripts/run_radio_metrics_v1_hosted_probe.sh` for hosted node-`5`,
  node-`6`, and direct-TCP contract proof
- [x] 3.3 Add `scripts/run_radio_metrics_v1_target_probe.sh` as the focused
  service-managed target node-`5` contract proof wrapper

## 4. Docs, Evidence, And Validation

- [x] 4.1 Update the main docs and interface index with the frozen COMM
  observability contract and explicit adjacent non-claims
- [x] 4.2 Add `evidence/records/radio-metrics-v1/README.md` with the exact
  hosted and target paths exercised and the exact metrics fields frozen
- [x] 4.3 Run fresh local verification, focused hosted and target probes,
  `openspec validate radio-metrics-v1`, and `openspec validate --specs`
