## Context

The redo work changed the interpretation of the original problem.

What is now supported by current evidence:

- S-band TCP does not exhibit the same shared-UART half-duplex corruption class
  seen on the lab UHF serial path.
- `uhf-backup` is already a distinct policy role with bounded session-open and
  allowlisted read/status behavior; it is not beacon-only.
- active UHF command sessions can be stabilized by suppressing reverse chatter
  during the session window.
- stock `fprime-gds` is still a single communication-adapter operator surface;
  the current repo-owned gateway is a one-northbound / one-southbound relay,
  not a multi-band multiplexer.

What is intentionally not claimed by this design:

- no new custom dual-link GDS plugin
- no single-process stock-GDS simultaneous heterogeneous southbound integration
- no broad UHF reliable transfer or RF closure
- no claim that beacon traffic is harmful on its own outside the active UHF
  command-session window

## Goals

- make the new baseline explicit in formal change text
- preserve S-band as the unchanged full formal path
- preserve UHF backup as bounded allowlisted ingress instead of collapsing it
  into beacon-only behavior
- define UHF primary as a clean command-session path with packet-noise
  suppression and session-window beacon suppression
- keep official file/data-product downlink as a formal spacecraft capability
- freeze the near-term ground approach as separate per-band GDS/gateway stacks
  until a later orchestrated ground surface is worth building

## Non-Goals

- no attempt to complete a custom multi-band `fprime-gds` adapter in this slice
- no attempt to make one `ground_ttc_gateway` instance multiplex both S-band
  and UHF at once
- no attempt to prove final operator UX for simultaneous multi-band control
- no change to the existing high-authority versus allowlisted command catalog

## Baseline Policy

### S-band Primary

`sband-primary` remains the normal formal TT&C baseline.

It continues to own:

- command uplink
- command/session response
- live `event/tlm` visibility
- official file/data-product downlink

This change does not narrow S-band because the fresh evidence does not show the
same transport-level interference class on the S-band TCP path.

### UHF Backup

`uhf-backup` remains a bounded backup-ingress policy role.

It keeps:

- BeaconV1 duty
- bounded authenticated `SESSION_OPEN(seq0)`
- allowlisted low-risk read/status and other explicitly cataloged backup-safe
  commands

It does not gain:

- full high-authority mutation rights
- implicit ownership of the primary telemetry or file downlink role

This role must stay distinct from `uhf-primary-after-failover`.

### UHF Primary After Failover

`uhf-primary-after-failover` is the explicit-switch runtime role reached only
after S-band-default operation promotes UHF to the active primary link.

In this role, the current baseline is:

- formal UHF command/session traffic remains active
- live `event/tlm` packet egress is suppressed on the formal UHF path
- beacon and other reverse chatter are suppressed during the active UHF
  command-session window
- official file/data-product downlink remains a formal capability and must not
  be suppressed merely because packet noise is being suppressed

This gives UHF primary a clean command-session window without redefining
official file/data-product downlink as a non-mission capability.

## Quiet Semantics

Two quiet semantics remain distinct:

- `diagnostic quiet`: proof-oriented hard quiet that may suppress both packet
  and file egress
- `session quiet`: operational UHF command-session quiet that suppresses only
  live packet egress and leaves official file routing intact

The landed runtime change in this slice updates `session quiet` to match the
second rule.

## Ground Baseline

### Current Truth

Current ground truth is:

- one stock `fprime-gds` process owns one communication-adapter surface
- one `ground_ttc_gateway` instance relays one southbound path
- the current repo does not yet provide one integrated operator surface that
  simultaneously manages S-band and UHF routing within one stock GDS instance

### Near-Term Operational Baseline

Near-term simultaneous multi-band operations should use:

- one S-band-dedicated stock GDS plus gateway stack
- one UHF-dedicated stock GDS plus gateway stack

This is the smallest reviewable operational extension because it preserves the
existing stock-GDS and gateway model while giving the operator simultaneous
access to both bands.

### Deferred Alternative

A future change may build a higher-level orchestrated operator surface or a
custom communication adapter/plugin that internally manages asymmetric or
multi-band transport.

That work is intentionally deferred because it is larger than the actual
closure lever proven by this redo.

## Verification Implications

The key verification consequences of this baseline are:

- UHF primary cleanliness should be evaluated against command/session success,
  absence of checksum flood, and bounded flap reduction while session quiet is
  active
- UHF backup proofs should continue to demonstrate allowlisted backup-ingress
  authority, not beacon-only behavior
- file/data-product downlink must remain reviewable as a formal capability and
  must not be silently dropped from the active baseline wording

## Risks And Trade-offs

- **[Risk] UHF primary still has some post-session flap outside the active
  quiet window**
  - Mitigation: keep the baseline explicit that the clean command-session window
    is the proven closure step; any broader post-session smoothing can follow in
    a separate change.
- **[Risk] Separate per-band GDS stacks are less convenient than one integrated
  operator surface**
  - Mitigation: treat dual-GDS as the near-term baseline only, and leave a
    future orchestrated ground surface open as a later governed change.
- **[Risk] reviewers could misread file preservation as a proof that UHF
  file/downlink is fully closed in all nonquiet conditions**
  - Mitigation: keep the wording limited to preserving official file/data
    product downlink as a formal capability, not as a blanket UHF reliable
    transfer claim.
