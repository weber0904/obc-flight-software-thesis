## Why

Mission Console currently renders operator-facing decimal values with
inconsistent precision and cannot replay the stock-GDS native command bytes
recorded by the maintained gateway capture. This follow-up keeps the existing
hosted operator surface and command authority intact while making display and
capture replay behavior predictable.

## What Changes

- Normalize finite decimal display values to at most two fractional digits and
  normalize negative zero, including array elements and trend labels.
- Preserve integer values, enums, identifiers, hexadecimal text, and other
  non-decimal strings exactly as supplied.
- Extend the existing Packet Lab `replay-captured-raw` case to recognize valid
  secure-command-v2 packets within stock-GDS native capture bytes while
  retaining the existing synthetic descriptor-envelope path.
- Reject malformed native candidates by checking the secure packet magic,
  version, reserved/header/MAC lengths, inner command type, bounds, and trailer;
  ignore unrelated noise without introducing a new command or HTTP endpoint.
- Add focused unit and hosted-probe assertions plus a bounded test record for
  this hosted operator-surface correction.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `mission-console`: Define consistent operator-facing decimal formatting and
  native-capture replay behavior for the existing Packet Lab case.

## Impact

- Affected code is limited to the Mission Console browser renderer, Packet Lab
  capture parser, hosted probe client, and Mission Console tests.
- No flight component, secure-command wire format, target/lab baseline, RF
  path, HTTP route, or F' command is added or changed.
- The existing manual secure-operations path remains the sole command
  authority; native capture parsing only selects previously recorded bytes for
  the already-governed replay case.
