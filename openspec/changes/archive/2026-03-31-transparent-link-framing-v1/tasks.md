## 1. Scope And Framing Definition

- [x] 1.1 Define the minimal transparent link frame format and capture how it stays distinct from the current TCP/GDS and `mock-text` baselines.
- [x] 1.2 Identify the current transport and runtime limitations that prevent binary-safe framed exchange.

## 2. Framing Implementation

- [x] 2.1 Implement a reusable transparent link framing/deframing utility with escaping, payload-length handling, and CRC-32 validation.
- [x] 2.2 Extend the byte-stream path and runtime entrypoints so framed payload exchange can run without breaking the current line-based flows.
- [x] 2.3 Extend the host-side transparent peer with a governed framed mode that can deframe incoming payloads and return framed responses.

## 3. Probe And Documentation

- [x] 3.1 Add governed host/target probe coverage for framed transparent payloads over the existing Raspberry Pi to host UART/RS485 path.
- [x] 3.2 Add a dedicated evidence record for the framed transparent path and describe its scope boundaries.
- [x] 3.3 Update repository-facing docs to explain when to use raw transparent mode versus framed transparent mode.

## 4. Verification And Closeout

- [x] 4.1 Run regression checks to prove the current TCP/GDS and `mock-text` baselines still pass.
- [x] 4.2 Run the governed framed transparent UART probe and record the observed result.
- [x] 4.3 Validate the change with OpenSpec and prepare it for archive.
