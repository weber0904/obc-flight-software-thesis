## 1. Display Precision

- [x] 1.1 Add a shared display-only formatter for finite non-integer numbers and explicit decimal strings, with at most two fractional digits and negative-zero normalization
- [x] 1.2 Apply the formatter to structured scalar values, array elements, trend latest values, and trend-axis labels while preserving identity-bearing and non-decimal values
- [x] 1.3 Add direct JavaScript coverage for rounding, negative zero, arrays, integers, hexadecimal text, enums, identifiers, and non-decimal strings

## 2. Capture Replay

- [x] 2.1 Add the stock-GDS native secure-packet fallback scanner while keeping complete synthetic descriptor envelopes authoritative
- [x] 2.2 Validate native candidate magic, version, reserved/header/MAC fields, inner command kind, trailer, and computed bounds while continuing past noise and malformed candidates
- [x] 2.3 Add capture parser and replay-selection tests for native envelopes, noise/trailer bytes, malformed candidates, synthetic precedence, and no-valid-packet failure

## 3. Hosted Operator Proof

- [x] 3.1 Extend the existing hosted Mission Console probe client to exercise `replay-captured-raw` and assert capture-replay provenance
- [x] 3.2 Update the Mission Console test record with the hosted-only display/replay scope, verification commands, and explicit target/RF non-claims

## 4. Verification And Closeout

- [x] 4.1 Run JavaScript syntax/direct tests and the complete Mission Console Python unit suite
- [x] 4.2 Run a fresh full `scripts/run_verification_ci.sh` gate and the isolated Mission Console hosted probe without a target probe
- [x] 4.3 Validate the OpenSpec change and main specs, sync/archive the change, update reconciliation artifacts, and rerun repository consistency checks
- [x] 4.4 Confirm sensitive-information, whitespace, documentation-governance, and clean-worktree gates before local-ready handoff
