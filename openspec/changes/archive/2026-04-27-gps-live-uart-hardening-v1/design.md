## Context

`gps-live-uart-source-v1` 已經把 `obc.local:/dev/serial0` 建成第一版 governed live GPS hardware path，並且驗證了真實 NMEA sentence ingest、cached state update、以及 fake/replay compatibility。後續 review 指出的問題集中在 low-level robustness：serial source 對 blank lines 的處理、`open()` 與 baudrate parsing 的防禦性、PTY test setup 的 fail-fast 行為，以及 target probe temporary artifact cleanup。

這次 change 不重開 GPS 設計，也不改 public contract。它只補 live UART source 的低層硬化，避免把 ignorable UART noise 或 test/probe hygiene 問題帶進後續 `CAN FD` 與 omitted-RF TT&C 主線。

## Goals / Non-Goals

**Goals:**
- Harden the live UART GPS source so ignorable blank serial lines do not surface as `NO_SOURCE_DATA`.
- Tighten runtime parsing and serial open behavior without changing the existing `fake` / `replay` / `live-uart` source contract.
- Improve PTY-backed GPS source tests so setup failures short-circuit cleanly and portability is explicit.
- Ensure the governed Raspberry Pi live GPS probe removes its temporary local artifacts on exit.

**Non-Goals:**
- No new GPS source modes, sentence types, or command/tlm/event families.
- No change to the direct `GPS -> OBC` architecture baseline.
- No changes to external `comm`, CSP, CAN FD, or omitted-RF TT&C work.
- No attempt to require live-sky fix, PPS, or navigation-quality validation.

## Decisions

### Keep the hardening inside the existing GPS serial-source boundary
The serial source remains a dedicated `IGpsSentenceSource` backend. The fix stays in `simulators/gps/GpsSource.cpp` and `GpsBridge.cpp` rather than routing GPS through `comm` helpers or introducing a new transport abstraction. This preserves the direct-sensor architecture already formalized in `gps-live-uart-source-v1`.

### Treat blank UART-delimited lines as ignorable noise
When the live serial source receives a newline that normalizes to an empty line, it will discard it and continue polling within the same bounded call instead of returning `false`. This prevents benign whitespace or repeated line endings from being surfaced as `NO_SOURCE_DATA`, while keeping true timeout/no-data behavior unchanged.

Alternative considered:
- Return `false` on empty lines and let `GpsBridge` treat that as a degraded transport signal.
Why rejected:
- It conflates ignorable framing noise with actual source starvation and produces misleading operator output.

### Make serial open more defensive but preserve bounded polling semantics
The live UART source will open the device with `O_NONBLOCK` in addition to `O_RDWR | O_NOCTTY`, then rely on `poll()` plus bounded reads for normal operation. This avoids blocking unexpectedly during open/bring-up while preserving the current bounded scheduled-poll model.

Alternative considered:
- Keep blocking `open()` because the current probe already passed on hardware.
Why rejected:
- Review feedback is correct that defensive `open()` is safer for edge-case serial drivers and does not materially complicate the implementation.

### Reject impossible baudrate env values before narrowing
`parseBaudrateEnv` will reject values that exceed `std::uint32_t` before narrowing from `unsigned long`. This is defensive correctness only; the governed supported baudrate set remains enforced later by the serial source.

### Make PTY tests fail fast and portable
The PTY-backed unit test harness will:
- include the correct PTY declarations explicitly
- stop constructing the PTY pair after the first failed setup step

This avoids cascading test noise and makes failures point to the actual setup fault.

### Clean temporary probe artifacts automatically
`run_rpi_gps_live_probe.sh` will add a local cleanup trap so `mktemp -d` output does not accumulate under `/tmp`.

## Risks / Trade-offs

- [Spec scope stays narrow] → The change only formalizes the blank-line handling aspect at requirement level; the rest remains implementation hardening. This keeps the change small, but not every review comment becomes a separate formal requirement.
- [Non-blocking open may expose previously hidden device errors earlier] → That is acceptable because the governed live-uart contract already allows activation failure when the device cannot be opened or configured.
- [Looping past blank lines could hide repeated noise bursts] → The source still uses the same bounded poll/read timeout, so persistent noise without valid sentences still resolves to timeout/no-data rather than an unbounded wait.
