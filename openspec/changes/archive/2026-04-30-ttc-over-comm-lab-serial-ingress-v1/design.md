## Context

The repository now has three separate COMM-adjacent baselines:

- hosted PTY-backed gateway TT&C proves `fprime-cli -> GDS -> ground_ttc_gateway -> PTY -> comm_csp_node -> CSP -> OBC` including bounded commands, events, and telemetry;
- subsystem UART preflight proves macOS-initiated physical request/reply byte exchange with `subsystem.local:/dev/serial0`;
- lab serial acquisition proves `subsystem.local` can transmit bounded framed records to a passive macOS receiver after conservative acquisition.

The previous guarded physical TT&C attempt failed before proving either OBC command readback or downlink. This change must not collapse that failure into a downlink-only conclusion, so the probe needs staged assertions that isolate physical ingress before attempting full TT&C.

## Goals / Non-Goals

**Goals:**

- Prove or precisely stop at the physical lab serial ingress boundary from macOS/GDS/gateway to `subsystem.local` COMM.
- Keep the minimum formal pass boundary at bounded uplink command ingress to OBC readback.
- Attempt events/channels downlink only after uplink ingress succeeds.
- Preserve existing hosted PTY regression coverage and COMM CSP service contract.

**Non-Goals:**

- No RF, real radio, file/downlink, target OBC migration, ScenarioBridge, `ground_pass_open`, `link_available`, Docker/cross-compile, or COMM shared CAN FD.
- No custom F' GDS plugin.
- No new COMM CSP service ports or wire layouts.

## Decisions

- Build one staged repository probe instead of rerunning a full TT&C script as a single black box.
  - Stage 0 can run prerequisite probes, but strict no-preamble UART preflight is optional diagnostic because the current lab link has shown acquisition/settling sensitivity.
  - Stage 1 launches real physical serial ingress and verifies OBC readback after bounded commands.
  - Stage 2 attempts `fprime-cli events` and `fprime-cli channels`.
- Keep Stage 1 as the minimum formal pass.
  - This directly tests the path that should be feasible from the macOS-initiated UART preflight.
  - If Stage 2 fails, the evidence stays at uplink ingress and records downlink diagnostics.
- Add an opt-in serial TX preamble to `ground_ttc_gateway` for physical lab validation.
  - Default gateway behavior remains unchanged unless `--serial-tx-preamble-lines` is configured.
  - The physical probe records the preamble count and delay so reviewers can separate link acquisition from useful GDS traffic.
- Use bounded command retries in Stage 1.
  - The current physical run can show link transitions and dropped frames before the bounded commands converge.
  - Stage 1 still requires final `eps ... pdu=7` and `adcs mode=POINTING` readback before passing.
- Reuse existing binaries and launch patterns.
  - `ground_ttc_gateway` remains a TCP-to-serial byte pump.
  - remote `comm_csp_node` remains native-built on `subsystem.local`.
  - hosted OBC remains local with `GROUND_LINK_MODE=comm-csp` and COMM node `4`.
- Use explicit endpoint defaults for this lab:
  - macOS: `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
  - subsystem: `/dev/serial0`
  - baudrate: `115200`

## Risks / Trade-offs

- [Risk] Full TT&C downlink still fails because subsystem-origin serial acquisition is not stock F' frame-clean.
  - Mitigation: Stage 2 cannot upgrade the verdict unless events/channels are observed; Stage 1 remains useful evidence.
- [Risk] Strict no-preamble UART preflight fails even though gateway traffic can converge after acquisition.
  - Mitigation: keep strict preflight as optional diagnostic, use an explicit gateway preamble in the physical probe, and record that clean first-byte behavior is not proven.
- [Risk] OBC readback remains unchanged even though macOS writes bytes.
  - Mitigation: Stage 1 stops there and preserves logs for gateway, COMM node, OBC, and GDS.
- [Risk] Serial endpoints or subsystem native workspace drift.
  - Mitigation: the probe can rerun physical prerequisite checks and records endpoint paths, baudrate, and target launch details.
- [Risk] Port collisions create false negatives.
  - Mitigation: allocate alternate local GDS/CSP/radio ports and record them in evidence.
