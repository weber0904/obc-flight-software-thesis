## Context

The repository has already separated four adjacent COMM proof boundaries:

- hosted PTY-backed gateway TT&C proves bounded commands, command events, and telemetry through `COMM` using a virtual serial stand-in;
- subsystem UART preflight proves macOS-initiated physical request/reply byte exchange with `subsystem.local`;
- subsystem-origin acquisition proves passive macOS reception of bounded subsystem-origin marker frames after acquisition preamble and spacing;
- physical lab serial ingress proves bounded command ingress through the real serial link into hosted OBC readback, but does not register full TT&C downlink.

This change builds on the last boundary. It must prove downlink over the same physical lab serial COMM path before the repository claims full physical lab serial TT&C.

## Goals / Non-Goals

**Goals:**

- Provide a focused probe whose formal PASS means bounded physical lab serial TT&C, not only physical uplink ingress.
- Keep the existing Stage 1 command readback as a prerequisite so downlink is proven on a live path that can still command OBC behavior.
- Require ground-side event and telemetry visibility through stock `fprime-cli` listeners.
- Preserve hosted PTY gateway regression as a separate baseline check.
- Keep the evidence and registry wording narrow enough to avoid borrowing proof from adjacent paths.

**Non-Goals:**

- No file/downlink behavior over COMM.
- No RF, real radio, target OBC migration, COMM SocketCAN participation, or combined physical internal carrier proof.
- No custom `fprime-gds` plugin and no changes to stock F' framing.
- No no-preamble or first-byte-clean physical UART claim.
- No new COMM CSP service ports or wire layouts.

## Decisions

- Add a new probe entrypoint instead of changing the meaning of the existing ingress script.
  - `run_comm_lab_serial_ingress_probe.sh` remains useful for the Stage 1 uplink boundary.
  - The new downlink probe requires the same Stage 1 readback and then treats Stage 2 downlink as mandatory.
- Reuse the current launch topology.
  - macOS runs headless `fprime-gds`, `fprime-cli`, `ground_ttc_gateway`, hosted OBC, and the local CSP hub.
  - `subsystem.local` runs native-built `comm_csp_node` as node `4` on `/dev/serial0`.
  - COMM keeps services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`.
- Keep acquisition controls explicit.
  - The physical gateway may use the same opt-in serial TX preamble settings as the ingress proof.
  - The evidence records preamble count, delay, command attempts, ports, endpoints, and log directory.
- Make downlink assertions concrete.
  - Events PASS requires at least the bounded command dispatch/completion observations for the EPS and ADCS commands.
  - Telemetry PASS requires `fprime-cli channels` to observe `GROUND_LINK_TX_BYTES`.
  - The final probe output reports `formal-verdict=bounded-ttc` only when Stage 1 and Stage 2 both pass.

## Risks / Trade-offs

- [Risk] Physical serial timing remains marginal and command/event frames arrive inconsistently.
  - Mitigation: keep bounded retries and explicit acquisition preamble; do not claim no-preamble behavior.
- [Risk] The probe can see command readback but miss `fprime-cli` listener output.
  - Mitigation: keep event/channel listener logs as first-class evidence and fail the formal downlink verdict when they are incomplete.
- [Risk] A downlink-only probe could pass without proving that the same path still carries useful uplink.
  - Mitigation: Stage 1 command readback remains mandatory.
- [Risk] Reusing the older ingress script could obscure which evidence upgraded the verdict.
  - Mitigation: add a distinct downlink probe and evidence directory.
