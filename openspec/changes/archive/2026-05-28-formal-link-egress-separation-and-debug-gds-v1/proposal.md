## Why

The original scope assumed that the node-`6` nonquiet closure would require a
full formal/debug split with a second debug-only GDS side channel. Fresh redo
evidence changed that priority.

This redo established three narrower truths:

- `fprime-gds` keepalive payload was a real formal-uplink noise source and must
  stay off on the governed UHF path.
- the dominant residual corruption on target/lab UHF was shared-UART reverse
  chatter, not proof that live `event/tlm` intrinsically belongs off the formal
  path on every link.
- the first useful closure step is operational-policy narrowing on UHF, not an
  immediate ground-system rewrite.

The baseline now needs to be rewritten around those facts. The repository still
needs clean, operator-reviewable wording for:

- what remains on S-band
- what UHF backup is actually allowed to do
- what UHF primary does during an active command session
- how near-term ground operations should handle simultaneous S-band and UHF
  access before a custom dual-link ground integration exists

## What Changes

- Freeze `sband-primary` as the unchanged full formal path for command,
  command-response, live `event/tlm`, and official file/data-product downlink.
- Freeze `uhf-backup` as bounded backup ingress with beacon plus allowlisted
  low-risk command/read-status continuity, not as beacon-only.
- Freeze `uhf-primary-after-failover` as a clean command-session path that
  suppresses live `event/tlm` on the formal UHF path and suppresses beacon
  chatter during the active UHF command-session window.
- Preserve official file/data-product downlink as a formal spacecraft
  capability; UHF session quiet must not silently redefine file downlink as
  disposable background noise.
- Keep the current gateway truth explicit: one `ground_ttc_gateway` instance
  owns one southbound path. Near-term simultaneous S-band/UHF operations are
  expected to use separate stock-GDS/gateway stacks per band rather than a new
  custom GDS plugin in this slice.

## Capabilities

### Modified Capabilities

- `comm-subsystem`: refine the active UHF primary and UHF backup policy
  baseline, and record UHF session-quiet packet suppression as separate from
  official file/downlink routing.
- `ground-ttc-gateway`: freeze the current single-southbound gateway boundary
  and the near-term dual-GDS operator baseline.
- `live-beacon-broadcast`: align beacon behavior with active UHF command-session
  suppression on the current baseline.
- `interface-contract-index`: record the corrected current-role semantics and
  the file-versus-packet quiet boundary.

## Impact

- Affected code: `CommEgressMux` session-quiet routing semantics and its unit
  coverage.
- Affected docs/specs: OpenSpec change artifacts plus current baseline
  architecture/interface documents.
- Affected operations: target/lab UHF primary remains a clean command-session
  path, while near-term simultaneous multi-band ground use is expected to be
  solved by dedicated per-band GDS/gateway stacks rather than one stock GDS
  process with two heterogeneous southbound links.
