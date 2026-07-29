## ADDED Requirements

### Requirement: Mission Console SHALL Normalize Operator-Facing Decimal Precision

Mission Console SHALL render finite decimal measurements with no more than two
fractional digits and SHALL normalize a rounded negative zero to `0` without
mutating the underlying API value or raw evidence.

#### Scenario: Structured fields and arrays use bounded decimal precision
- **WHEN** Mission Console renders decimal number primitives, explicit decimal
  strings, or decimal elements within a structured array
- **THEN** each decimal display value SHALL be rounded to at most two
  fractional digits
- **AND** redundant trailing zeroes SHALL not be displayed.

#### Scenario: Trend labels use the same bounded decimal precision
- **WHEN** Mission Console renders a trend latest-value label or trend-axis
  tick
- **THEN** it SHALL use the same at-most-two-decimal formatting rule as other
  structured operator fields.

#### Scenario: Negative zero is normalized for display
- **WHEN** a positive or negative decimal rounds to zero at two fractional
  digits
- **THEN** Mission Console SHALL display `0`
- **AND** it SHALL NOT display `-0`.

#### Scenario: Non-decimal and identity-bearing values remain unchanged
- **WHEN** Mission Console renders an integer number, integer string, enum,
  identifier, hexadecimal text, scientific-notation text without a decimal
  point, or other non-decimal string
- **THEN** it SHALL preserve the original displayed value
- **AND** it SHALL NOT coerce that value through the decimal formatter.

### Requirement: Mission Console SHALL Replay Valid Secure Packets From Maintained Capture Forms

The existing Packet Lab `replay-captured-raw` case SHALL accept both the
repository synthetic command-descriptor envelope and stock-GDS native capture
bytes while preserving the maintained manual secure-command authority.

#### Scenario: Existing synthetic capture remains authoritative
- **WHEN** a capture contains one or more complete synthetic
  command-descriptor envelopes
- **THEN** Packet Lab SHALL use the existing synthetic packet parsing behavior
- **AND** it SHALL NOT additionally select native-looking byte sequences from
  that capture.

#### Scenario: Valid secure packet is recovered from native capture noise
- **WHEN** no synthetic packet is present and a stock-GDS native capture
  contains unrelated prefix or suffix bytes around a complete secure-command-v2
  packet
- **THEN** Packet Lab SHALL recover the complete secure packet and its absolute
  capture offset for the existing replay workflow.

#### Scenario: Malformed native candidates are ignored
- **WHEN** a native capture contains candidates with an invalid secure magic,
  version, reserved field, header length, MAC length, inner command kind,
  trailer, or computed packet boundary
- **THEN** Packet Lab SHALL ignore those candidates and continue scanning for a
  later valid secure-command-v2 packet.

#### Scenario: No valid packet preserves the bounded failure
- **WHEN** neither a complete synthetic packet nor a valid native secure packet
  exists in the capture
- **THEN** `replay-captured-raw` SHALL fail with the existing no-reusable-packet
  error behavior
- **AND** it SHALL NOT send arbitrary capture bytes.

#### Scenario: Replay extension does not create a second command surface
- **WHEN** native capture replay is used
- **THEN** Mission Console SHALL continue to send the selected packet through
  the existing Packet Lab and manual secure-operations path
- **AND** it SHALL NOT add an HTTP endpoint, F' command, secure-command format,
  or target/RF verification claim.
