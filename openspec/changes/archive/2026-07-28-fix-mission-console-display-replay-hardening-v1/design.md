## Context

Mission Console is a hosted operator surface layered over the maintained
manual dual-GDS runtime. Its browser renderer currently applies different
precision rules to structured fields and trend values. Packet Lab already
supports `replay-captured-raw`, but its capture parser recognizes only the
synthetic command-descriptor envelope emitted by repository helpers; the
maintained gateway capture can instead contain stock-GDS native envelope bytes
followed by the secure F' command packet.

The change crosses the browser renderer, gateway parser, hosted probe, tests,
and governed evidence. It must not alter the secure-command protocol,
flight-side validation, manual command authority, or target/RF claims.

## Goals / Non-Goals

**Goals:**

- Apply one bounded formatting rule to finite decimal numbers shown in
  structured fields, arrays, trends, and trend-axis labels.
- Preserve values whose syntax or type represents integer state, enums,
  identifiers, hexadecimal data, or other non-decimal text.
- Recover a complete secure-command-v2 packet from a noisy stock-GDS native
  capture while rejecting malformed lookalikes.
- Preserve synthetic descriptor parsing as the preferred, backward-compatible
  capture representation.
- Cover both success and rejection boundaries with focused automated tests and
  a hosted probe assertion.

**Non-Goals:**

- Changing backend telemetry values, API JSON, trend calculations, or raw
  evidence.
- Adding an endpoint, Packet Lab case, F' command, command authority, or secure
  wire format.
- Claiming target, lab, RF, or flight-side requalification.
- Replacing stock GDS or interpreting its entire transport protocol.

## Decisions

### Centralize display-only decimal formatting

The browser will use one `formatDisplayNumber` helper. JavaScript numbers are
formatted only when they are finite and non-integer. String values are
formatted only when they match an explicit decimal syntax containing a decimal
point; integer strings, scientific notation without a decimal point,
hexadecimal strings, enums, and identifiers remain unchanged. Rounding uses
two fractional digits at most, removes redundant trailing zeroes, and converts
rounded negative zero to `0`.

This is preferred over coercing every value with `Number(...)`, which would
silently rewrite identifiers, integer strings, hex text, and enum-like values.
It is also preferred over backend mutation because precision is an
operator-display concern and raw API evidence must remain intact.

### Keep synthetic capture parsing authoritative when present

`parse_transport_packets` will first run the existing synthetic
`COMMAND_DESCRIPTOR + length + payload` parser. If it finds any complete
synthetic packets, it returns that set unchanged. Native scanning runs only
when no synthetic packet was recovered.

This preserves existing offsets, exclusion bookkeeping, and test behavior and
prevents incidental native-looking byte sequences inside a synthetic capture
from changing replay selection.

### Scan native captures for a complete secure-command-v2 packet

The fallback scanner advances one byte at a time through noise. A candidate
must begin with the F' command packet kind and secure-command-v2 opcode and
must have enough bytes for the fixed header. It is accepted only when:

- secure magic and version match the maintained protocol;
- reserved and trailer fields are zero;
- header and MAC lengths match the maintained constants;
- the inner payload is at least an F' command header and declares the F'
  command packet kind;
- the computed packet boundary remains within the capture.

Malformed candidates are skipped and scanning continues. The accepted
`TransportPacket` retains its absolute capture offset so existing exclusion
and capture-marker logic continues to work.

This bounded recognition is preferred over attempting to decode a
stock-GDS-specific outer envelope, because the capture can include arbitrary
noise or changing outer metadata while the secure F' packet contract is stable
and already repository-owned.

### Test the browser helper directly without changing runtime packaging

The formatter and structured renderer will be exported only when a CommonJS
test environment is present. A small Node test will load the existing browser
script with minimal DOM stubs and assert rounding, negative zero, arrays, and
preservation cases. Browser execution remains a normal non-module script.

The Python Mission Console suite will invoke this Node test so the established
suite command exercises both gateway and display contracts.

## Risks / Trade-offs

- [Risk] A random byte sequence could resemble a secure packet. → Require all
  stable secure header fields, a valid inner command kind, and an in-bounds
  computed length before accepting it.
- [Risk] Native fallback could reorder or shadow synthetic packets. → Run it
  only when the synthetic parser returns no packets.
- [Risk] Display formatting could rewrite identifiers that happen to be
  numeric. → Format numeric primitives only when non-integer and strings only
  when they contain explicit decimal syntax; keep raw JSON unchanged.
- [Risk] JavaScript floating-point rounding remains binary floating-point
  rounding. → Limit the claim to display precision, assert representative
  boundaries, and do not mutate evidence or protocol values.
- [Trade-off] Scientific-notation strings without a decimal point stay
  unchanged even when mathematically fractional. This favors preservation of
  non-decimal textual representations over aggressive cosmetic coercion.

## Migration Plan

No data migration is required. Deploy by replacing the Mission Console static
asset and gateway code together. Rollback is the branch commit revert; capture
files, snapshot data, and API schemas remain compatible.

## Open Questions

None. The change is deliberately bounded to the existing hosted
operator-surface contract.
