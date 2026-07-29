## Why

The current S-band node `5` and UHF node `6` baselines prove hosted COMM gateway behavior using stock `ComFprime` framing, but they do not establish CCSDS framing evidence. F' v4.1.0 includes `ComCcsds`, APID assignment, and CCSDS TC/TM framing support, so the project needs a bounded comparison before choosing whether to adopt CCSDS on the ground link path.

## What Changes

- Add a bounded `ccsds-ground-link-spike` comparative analysis with hosted proof.
- Prototype a spike-only CCSDS OBC path for S-band node `5` while leaving the default `OBC` `ComFprime` topology and existing S-band/UHF baselines unchanged.
- Validate command, event, telemetry, bounded housekeeping archive file/downlink, and gateway raw-byte compatibility through the hosted S-band node `5` path.
- Record an explicit recommendation: adopt CCSDS now, defer CCSDS adoption with blockers, or keep `ComFprime` while preserving CCSDS-aligned mission/session semantics.
- Keep UHF node `6` as analysis-only for this change.
- Exclude topology-wide migration, RF behavior, reliable transfer, target hardware, Raspberry Pi deployment, command authority, and failover policy.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: define the CCSDS spike boundary, the S-band node `5` proof scope, and the invariant that default `ComFprime` baselines remain unchanged.
- `ground-ttc-gateway`: define CCSDS GDS framing compatibility through transparent raw-byte gateway relay.
- `verification-evidence`: require CCSDS spike evidence to be independent from existing `ComFprime` records and to include framing, APID/sequence observations, file byte-match, and recommendation.
- `verification-path-registry`: allow a reusable CCSDS hosted path registration only if the hosted proof passes; failed spikes record blockers without registering a path.

## Impact

- Adds a spike-only CCSDS topology/executable or equivalent build target.
- Adds a repository-owned CCSDS hosted probe script and a bounded evidence record.
- May add non-breaking GDS framing selection support to existing stack helpers if needed.
- Does not change COMM services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, `32 LINK_STATUS`, S-band node `5`, UHF node `6`, or existing COMM packet layouts.
