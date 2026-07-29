# interface-contract-index Specification

## Purpose
Define the governed repository contract for the checked-in non-normative
current interface index and the fact-status rules it follows.
## Requirements
### Requirement: Current Interface Contract Index
The repository SHALL provide a checked-in `docs/interfaces.md` file as a
non-normative current interface contract index for the active baseline, and the
document SHALL state that code, topology, archived evidence, the verification
path registry, and current main specs outrank that index when conflicts exist.

#### Scenario: Reviewers have one checked-in current-contract entrypoint
- **WHEN** a reviewer or future agent needs the current command, transport,
  timing, storage, or sequencing interface boundary
- **THEN** the repository SHALL provide `docs/interfaces.md` as one checked-in
  starting point instead of requiring them to reconstruct the answer from
  multiple roadmap notes alone

#### Scenario: Interface index does not override higher-priority truth
- **WHEN** `docs/interfaces.md` disagrees with code, topology, archived
  evidence, the verification-path registry, or current main specs
- **THEN** the repository SHALL treat the higher-priority source as current
  truth
- **AND** the index SHALL be updated in a later governed change instead of
  silently redefining the baseline

### Requirement: Interface Index Uses Explicit Fact Status
`docs/interfaces.md` SHALL classify its current-fact entries as `verified`,
`configured-hosted`, or `TBD`, and it SHALL NOT present unproven target timing
or MTU values as frozen current-baseline facts.

#### Scenario: Hosted-only timing and framing facts stay hosted-scoped
- **WHEN** the index records the current hosted CCSDS frame size or hosted
  rate-group timing profile
- **THEN** it SHALL mark those entries as `configured-hosted` or otherwise make
  the hosted scope explicit
- **AND** it SHALL NOT imply that those values are already frozen target-flight
  truth

#### Scenario: Partially proven target timing facts may be promoted
- **WHEN** the repository has governed evidence for specific target
  service-managed timing facts such as base tick, divisors, nominal rates, or
  zero-slip behavior on a declared workload window
- **THEN** the corresponding interface-index entries MAY be marked `verified`
- **AND** the same section SHALL keep any still-unproven WCET, jitter, or
  broader timing boundary explicit as residual gaps instead of silently
  inferring final target truth

#### Scenario: Unknown target values remain intentionally open
- **WHEN** the repository still lacks governed evidence for a target flightlike
  MTU, WCET bound, jitter limit, or broader timing boundary
- **THEN** the corresponding interface-index entry SHALL remain `TBD` or SHALL
  be called out as an explicit residual gap
- **AND** the document SHALL NOT substitute a guessed or Raspberry-Pi-only
  number as universal target truth

### Requirement: Interface Index Covers Current Baseline Boundaries

The interface contract index SHALL record the hosted-baseline keystore-backed
auth contract, handshake-only unknown uplink, and staged file-uplink authority
boundary in addition to the existing secure-command and UHF runtime facts.

#### Scenario: Reviewers can audit the tracked keystore contract in one place
- **WHEN** a reviewer inspects the command-security or COMM sections of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that comm-managed auth defaults come from
  one tracked keystore asset
- **AND** they SHALL be able to see that the asset carries `module_serial`
  plus per-service key material as the preferred secure baseline, while any
  retained S-band or UHF legacy `source_id/key_slot/key` tuple is explicitly
  compatibility-only
- **AND** they SHALL be able to see that old hosted runtime
  `--command-auth-*` injection and `--command-auth-keystore` override are no
  longer part of the active baseline.

#### Scenario: Reviewers can audit unknown uplink and staged file authority separately
- **WHEN** a reviewer inspects the file-ingress and unknown-uplink sections of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that APID `0x00FE` handshake traffic is
  the only admitted current unknown-uplink family on the comm-managed route
- **AND** they SHALL be able to see that `.sequence-staging/<leaf>` upload now
  requires both active secure auth and current allowed runtime file role
- **AND** they SHALL be able to see that `uhf-backup` remains deny-only for
  staged upload while explicit-switched `uhf-primary-after-failover` may admit
  staged upload only after re-auth.

### Requirement: Interface Index Records Frozen Future Target-Bearing Dual-Link Boundary

`docs/interfaces.md` SHALL summarize the frozen future target-bearing
simultaneous dual-link boundary without restating it as a currently proven path.

#### Scenario: Interface index records the future path family and verdict split
- **WHEN** reviewers inspect the current COMM boundary notes in
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that the frozen future target-bearing
  family uses:
  - default node-`5` primary truth
  - non-quiet node-`6` `uhf-backup` concurrent adjunct
  - explicit switched `uhf-primary-after-failover` UHF command truth
  - quiet node-`6` adjunct rescue
- **AND** they SHALL be able to see that `target-claim` and
  `operator-observability` are separate formal verdicts

#### Scenario: Interface index preserves current non-claim status
- **WHEN** the same summary describes the frozen future boundary
- **THEN** it SHALL state that the boundary is clarified future work rather
  than a currently proven simultaneous target path
- **AND** it SHALL keep non-claims explicit for one-GDS aggregation, one-gateway
  multiplexing, simultaneous full-authority commands on both links, and
  mandatory file/downlink continuity in the main PASS

### Requirement: Interface Index Records The First Implementation-Bearing Target Dual-Link Branch

`docs/interfaces.md` SHALL summarize the first implementation-bearing
target-bearing dual-link proof as an exact proven branch, not only as a frozen
future boundary.

#### Scenario: Reader can see the exact proven branch
- **WHEN** readers inspect `docs/interfaces.md` after the proof lands
- **THEN** they SHALL be able to see:
  - the default node-`5` primary truth
  - the non-quiet node-`6` `uhf-backup` adjunct
  - the explicit switched `uhf-primary-after-failover` non-quiet truth
  - whether quiet rescue was used
  - whether the official run landed as `target-claim=PASS` with
    `operator-observability=PASS` or `DEGRADED`

#### Scenario: Interface index does not overstate unrun branches
- **WHEN** the official run proves only one successful outcome branch
- **THEN** `docs/interfaces.md` SHALL describe only that exact branch as
  proven
- **AND** it SHALL keep any unrun rescue or degraded branch as a non-claim or
  future possibility rather than current proof

### Requirement: Interface Index Records Current COMM Observability Semantics

`docs/interfaces.md` SHALL describe the new secure auth/session observability
surface with owner, freshness, timeout, and invalidation semantics.

#### Scenario: Reviewers can audit secure auth observability
- **WHEN** a reviewer inspects the COMM observability section of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see the active secure service ID, suppress
  owner role, last accepted secure sequence, and inactivity timeout semantics
- **AND** they SHALL be able to see that UHF backup and failover-primary share
  the same secure service while remaining distinct runtime roles.

### Requirement: Interface contract index records the reliable-transfer boundary

The interface contract index SHALL record the owner and retry boundary for
the current bounded reliable slice.

#### Scenario: Owner boundary is reviewed

- **WHEN** the bounded reliable slice after `uhf-reliable-transfer-v1` is
  described
- **THEN** the index SHALL state that `CommController` remains the policy owner
- **AND** it SHALL state that a bounded helper owns in-transfer send/ACK/retry
  execution
- **AND** it SHALL state that `DpCatalog` remains the file-selection owner

#### Scenario: Retry boundary is reviewed

- **WHEN** the change documents retries
- **THEN** it SHALL distinguish ground whole-command retry from reliable
  transfer timeout/resend semantics
- **AND** it SHALL state that the reliable-transfer slice does not reopen
  command authority, preferred secure-session boundary, or gateway role policy

#### Scenario: Reliable-transfer segment size stays path-local

- **WHEN** the interface index records the `160`-byte reliable-transfer
  segment ceiling
- **THEN** it SHALL identify that value as belonging only to the bounded
  reliable helper path on the exact allowed S-band node-`5` and
  explicit-switched UHF node-`6` slices
- **AND** it SHALL distinguish that helper ceiling from the stock current UHF
  `243`-byte `Fw::FilePacket::DATA` ceiling
- **AND** it SHALL NOT present that segment size as a repo-wide transport MTU

### Requirement: Interface Index Separates Numeric Transport Derivation From Path Proof

The interface contract index SHALL describe each frozen current transport
ceiling with both its numeric derivation source and the narrower path-scope
proof class that makes the ceiling relevant to the active baseline.

#### Scenario: Reviewers can audit a derived ceiling without mistaking it for direct runtime proof

- **WHEN** `docs/interfaces.md` records a current command or file/downlink
  ceiling
- **THEN** the row SHALL identify the checked-in constants or serializer
  formula that produced the number
- **AND** it SHALL separately identify the governed path whose contract uses
  that ceiling

#### Scenario: Hosted framing facts stay distinct from admitted payload ceilings

- **WHEN** the interface index records CCSDS frame size, SCID, or VCID facts
- **THEN** it SHALL keep hosted/configured framing facts distinct from the
  admitted current command or file/downlink payload ceilings
- **AND** it SHALL NOT present a hosted frame-size value as the same proof
  class as a source-derived inner-payload ceiling

### Requirement: Interface Index Records Current APID Governance And Change Control

The interface contract index SHALL record the current `ComCfg.Apid`
reservation policy, distinguish active path-proven operational flows from
reserved or invalid classes, and state how future APID claims are governed.

#### Scenario: Reviewers can audit the current APID split in one place

- **WHEN** a reviewer inspects the APID section of `docs/interfaces.md`
- **THEN** they SHALL be able to see which APIDs are active path-proven flows
- **AND** they SHALL be able to see which values are reserved current-code
  classes, special reserved values, or invalid

#### Scenario: Future APID expansion does not collapse into code drift

- **WHEN** the index documents the current APID map
- **THEN** it SHALL state that new active APID claims require a governed change
  that updates code, current docs, formal specs, and evidence together

### Requirement: Interface Index Distinguishes Hosted And Target Secure Auth Status

The interface contract index SHALL distinguish hosted secure-auth proof from
the target secure-auth proof added by `target-secure-auth-proof-v1`.

#### Scenario: Interfaces show target observability after target proof
- **WHEN** target secure-auth proof evidence is accepted
- **THEN** `docs/interfaces.md` SHALL update secure-auth and secure-command
  observability wording so target/lab status no longer appears unproven for
  the exact S-band and bounded UHF cases exercised by the proof.

#### Scenario: Interfaces keep UHF role boundaries explicit
- **WHEN** `docs/interfaces.md` describes UHF secure auth after this change
- **THEN** it SHALL keep `uhf-backup` and
  `uhf-primary-after-failover` separate
- **AND** it SHALL state that UHF primary staged-upload success remains outside
  the target proof claim.

### Requirement: Interface Index Records Current Content Tiers In Addition To Access Gating

`docs/interfaces.md` SHALL describe the current node-`5` observability tiers
so reviewers can distinguish curated live summary, fresh GET-driven bounded
readback, onboard cached truth, formal reviewable proof / transport / policy
observability, and remaining diagnostics-only residual surfaces.

#### Scenario: Reviewers can audit keep-live truth versus reviewable and diagnostics-only residuals
- **WHEN** a reviewer inspects the observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL identify which selected family surfaces remain in
  current live summary
- **AND** it SHALL identify which transport, queue, owner, egress, and policy
  surfaces remain formal reviewable observability without becoming keep-live
  summary
- **AND** it SHALL keep the remaining residual live surfaces diagnostics-only
  rather than current operator baseline truth.

#### Scenario: External status commands distinguish cached truth from fresh readback
- **WHEN** the same index describes `GET_*` or read/status command surfaces
- **THEN** it SHALL distinguish cached onboard truth from fresh external
  observation/readback
- **AND** it SHALL make the retirement of `STORAGE_SCAN_NOW` and the fresh
  semantics of `STORAGE_GET_STATUS` explicit.

### Requirement: Interface Index Records The Exact Node-5 Residual Inventory

`docs/interfaces.md` SHALL record the current node-`5` residual live
inventory with explicit owner/component mapping and final governance bucket for
each current surface under review.

#### Scenario: Reviewers can audit the resource and residual surface mapping
- **WHEN** a reviewer inspects the node-`5` observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL identify the reviewed `SystemResources`,
  `GroundLinkDriver`, `GroundLinkHealthProvider`, `ComCcsds` / `OBCComCcsds`,
  `CspRuntimeOwner`, `CommEgressMux`, `UartDriver`, and `CommController`
  surfaces
- **AND** it SHALL state the final bucket, replacement surface if any, and the
  owner/component for each reviewed item.

#### Scenario: Reviewers can see current exposure and planned action separately
- **WHEN** the same inventory is used during an active same-change rebuild
- **THEN** it SHALL distinguish current observed exposure from final governance
  bucket
- **AND** it SHALL state the implementation action for each reviewed item,
  including docs-only reclassification, proof/oracle repair, runtime behavior
  repair, or explicit same-change deferral.

### Requirement: Interface Index Records The Current Observability Tier Boundary

`docs/interfaces.md` SHALL summarize the current observability tier boundary so
reviewers can distinguish always-on critical surfaces, auth-gated S-band live
packet visibility, and bounded `GET_*` summary readback.

#### Scenario: Reviewers can audit the live observability tiers in one place
- **WHEN** a reviewer inspects the COMM observability section of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that:
  - UHF beacon remains a no-ACK always-on critical surface
  - packetized S-band live `event/tlm` is quiet until accepted S-band secure
    auth opens the current live packet session
  - bounded `GET_*` or read/status commands remain the on-demand summary tier
- **AND** they SHALL be able to see that these tiers are current operator truth,
  not future generic telemetry redesign.

#### Scenario: Interface index keeps live packet visibility distinct from summary readback
- **WHEN** the same section describes readback behavior
- **THEN** it SHALL distinguish packetized live observability from bounded
  component-owned summary events/telemetry emitted by `GET_*` or status
  commands
- **AND** it SHALL NOT present those summary readbacks as proof that broad live
  packet chatter remains always-on by default.

### Requirement: Interface Index Records The Manual Operator Surface Contract

`docs/interfaces.md` SHALL summarize the maintained manual operator manifest
and session-state contract for the hosted and target dual-GDS surfaces.

#### Scenario: Reviewers can audit the manual helper contract in one place
- **WHEN** a reviewer inspects the COMM/manual-ops section of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see the required manifest fields, the hosted
  versus target surface split, and the stored auth/session-state fields
- **AND** they SHALL be able to see the invalidation triggers for manual auth
  state without having to infer them from helper code alone.

### Requirement: Interface Index Records Current Event-Quieting Boundaries

`docs/interfaces.md` SHALL describe the current live event surface after the
event-noise reduction changes so reviewers can distinguish operator-facing
events from retained local/debug-only visibility.

#### Scenario: Current docs distinguish mask-change events from periodic reduction
- **WHEN** reviewers inspect the reduced-state observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL state that `STATE_MONITOR_UPDATED` is emitted on
  reduced-state mask changes rather than on every successful scheduled
  reduction.

#### Scenario: Current docs distinguish packetized operator events from local debug events
- **WHEN** reviewers inspect the COMM or data-product observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL state that success `CSP_PING_RESULT` is not part
  of the default packetized ground live event surface
- **AND** it SHALL state that `HK_TREND_PRODUCT_WRITTEN` remains operator-facing
  while `DpWriter.FileWritten` is local/debug-only visibility by default
- **AND** it SHALL describe `HK_TREND_PRODUCT_WRITTEN` as chunk-oriented rather
  than one-sample-per-file success

### Requirement: Interface Index Records Chunked Official HK Product Controls

`docs/interfaces.md` SHALL describe the current operator-facing control and
identity surfaces for chunked official HK `.fdp` products so reviewers can see
how freshness and file-size policy are controlled without reading helper code.

#### Scenario: Current docs record chunked HK mission-history identity
- **WHEN** reviewers inspect the official state/history section of
  `docs/interfaces.md`
- **THEN** the document SHALL state that the active official HK `.fdp` path
  uses `HkTrendRecord` record id `0` as an array V6 sample record
- **AND** it SHALL state that each emitted `.fdp` also carries
  `HkTrendChunkMeta` record id `1`

#### Scenario: Current docs record operator freshness and size controls
- **WHEN** reviewers inspect the HK producer interface description in
  `docs/interfaces.md`
- **THEN** the document SHALL state that `HK_TREND_FLUSH` can finalize a
  pending non-empty chunk on demand
- **AND** it SHALL state that `HK_TREND_GET_STATUS` reports pending sample/file
  state
- **AND** it SHALL state that `HK_TREND_TARGET_FILE_BYTES` is the persistent
  target-size control bounded by the current reliable-transfer ceiling

### Requirement: Interface Index Records Mission Console Readback Categories

`docs/interfaces.md` SHALL record the Mission Console distinction between
surface/lifecycle truth, keep-live summary, operator-facing transition events,
and explicit detailed readback.

#### Scenario: Reviewers can audit Mission Console data tiers
- **WHEN** reviewers inspect the Mission Console-related interface sections
- **THEN** they SHALL be able to see which current surfaces belong to dashboard
  summary, transition/event review, and explicit detailed readback
- **AND** the wording SHALL keep bounded `GET_*` or status-driven readback
  distinct from broad ambient live packet chatter.

### Requirement: Interface Index Records Packet-Lab Diagnostic Boundaries

`docs/interfaces.md` SHALL describe the Mission Console packet-lab surface as a
bounded diagnostic/demo feature with parsed packet summaries and explicit
non-claims.

#### Scenario: Packet-lab docs stay bounded and reviewable
- **WHEN** reviewers inspect the packet-lab-related interface wording
- **THEN** the docs SHALL identify the supported replay/tamper cases, the
  bounded field-summary intent, and the expected evidence model for failure
  observation
- **AND** they SHALL NOT describe the surface as a generic fuzzing framework,
  flight operator plane, or alternate secure authority.

### Requirement: Interface Index Records Public Keystore Provisioning
`docs/interfaces.md` SHALL distinguish the tracked example keystore, ignored
hosted runtime copy, packaging-time target input, installed fixed path, and
prohibition on runtime command-auth injection.

#### Scenario: Reviewer inspects command-auth configuration
- **WHEN** the public interface index describes secure command provisioning
- **THEN** it SHALL state that example keys are non-deployable
- **AND** it SHALL record `OBC_PACKAGE_KEYSTORE_PATH` as packaging-only
- **AND** it SHALL preserve the fixed installed runtime path and manifest digest

