## Context

The repository currently proves that a Raspberry Pi OBC can exchange raw transparent payloads with a host-side serial peer over UART/RS485 using the legacy EnduroSat-style data path. That path is useful as a transport baseline, but it still treats the link as an unstructured byte stream. For research and later ground-integration work, the repository now needs a governed framing layer that:

- is binary-safe
- defines packet boundaries explicitly
- includes integrity checking
- can be deframed by the host peer without depending on a real transceiver

At the same time, the project should not collapse its existing layers. The direct TCP path to `fprime-gds` remains the simplest and most stable baseline for the F' ground stack. The controller-oriented `mock-text` radio path also remains the main regression baseline for the current comm architecture. The new framed transparent path is therefore an additional layer, not a replacement.

## Goals / Non-Goals

**Goals**

- Add a minimal framed transparent-UART link format above the existing serial byte-stream transport.
- Keep the framing binary-safe so payload bytes may include values such as `0x00`, `0x7D`, or `0x7E`.
- Let the host-side peer deframe and validate payloads, then return a framed response over the same UART/RS485 path.
- Keep the direct TCP/GDS path and current `mock-text` comm baseline unchanged.

**Non-Goals**

- Replace the direct `fprime-gds` TCP path
- Implement RF modulation/demodulation or physical-layer behavior
- Implement the EnduroSat legacy ESPS control/configuration plane
- Claim full ground-station integration through the transparent link

## Decisions

### Decision: Use a repository-owned transparent link frame format

This change will introduce a project-local frame format instead of pretending that the transparent transceiver already provides one. This keeps the repository honest about where framing responsibility lives in transparent mode and creates a controlled step between raw bytes and any later ground-side integration.

### Decision: Preserve the TCP/GDS baseline as the canonical ground-stack path

The existing direct TCP path to `fprime-gds` remains the simplest route for integrated F' commanding and telemetry. The new framed transparent path will not replace it. This protects the current baseline and supports the desired layer-by-layer development flow.

### Decision: Use a trailing-delimiter, escaped frame body with CRC-32

The frame format will use:

- a version byte
- a flags byte
- a 16-bit big-endian payload length
- raw payload bytes
- a 32-bit CRC-32 over the header and payload
- byte stuffing for delimiter and escape bytes
- a trailing frame delimiter byte

This keeps the first framing slice simple while still supporting binary-safe payloads and deterministic frame boundaries.

### Decision: Add a framed operator/probe path instead of replacing `uart raw`

The existing `uart raw` command will remain available for plain-text and raw byte-stream validation. A separate framed operator/probe path will be added for the new link format. This keeps the new work reviewable and avoids confusing the raw and framed baselines.

### Decision: Keep the host peer scope modest

The host-side peer will be extended only enough to deframe, validate, and reframe payloads for lab validation. It will not claim to model RF behavior, real ground protocols, or vendor-specific control semantics.

## Risks / Trade-offs

- [Risk] The new frame format could be mistaken for a flight-final protocol.  
  Mitigation: document it clearly as a repository-owned transparent link layer, not the vendor RF protocol or final ground-station protocol.

- [Risk] Adding binary-safe framing may tempt the project to merge the transparent path with `fprime-gds` too early.  
  Mitigation: keep the TCP/GDS path unchanged and state explicitly that ground-gateway work remains future scope.

- [Risk] Transport changes needed for framed byte exchange could regress the current text-oriented baselines.  
  Mitigation: preserve the current line-based exchange behavior and add dedicated integration coverage for both line-based and framed paths.

## Migration Plan

1. Add a reusable framing/deframing utility and the transport hooks it needs.
2. Extend the host-side serial peer with a framed transparent mode.
3. Add a governed runtime/probe path that can send framed binary payloads over the Raspberry Pi UART/RS485 link.
4. Capture evidence and keep the framed path explicitly distinct from the direct TCP/GDS baseline.
