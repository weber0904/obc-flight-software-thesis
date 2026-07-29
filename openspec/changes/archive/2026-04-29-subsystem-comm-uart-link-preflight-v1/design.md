## Context

The previous governed hardware UART evidence exercised an OBC-side serial allocation that is now historical after `/dev/serial0` on `obc.local` was reallocated to GPS. The next COMM direction moves the lab ingress toward `subsystem.local`, but the repository has not yet proven that the new macOS-to-subsystem serial wiring can carry even a bounded macOS-initiated request/reply exchange.

The formal platform baseline already provides a `subsystem.local` synced-workspace native build flow. This change intentionally keeps that flow instead of introducing cross-compilation or installed subsystem bundles.

## Goals / Non-Goals

**Goals:**

- prove a bounded macOS-initiated physical UART request/reply exchange between macOS and `subsystem.local`
- reuse the existing `mock-text` serial peer behavior for a small sanity check
- require explicit host and subsystem serial device paths
- capture reviewable logs and evidence before starting TT&C lab ingress work
- keep the proof distinct from historical OBC-side serial comm and from gateway-backed TT&C

**Non-Goals:**

- do not add Docker/Linux CI parity or cross-compilation
- do not install a subsystem simulator bundle
- do not run the gateway-backed TT&C path over this serial link
- do not claim clean subsystem-origin cold-first downlink, RF, real radio, file/downlink, target OBC, or COMM shared CAN FD behavior

## Decisions

### Decision: Use a dedicated serial probe utility instead of the OBC runtime

The default governed probe uses the lab-ingress direction: `subsystem.local` runs the native-built `radio_mock_server` peer on the configured serial device, and macOS runs a small `serial_link_probe` requester that opens the host serial device and issues `STATUS`, `ENABLE 1`, and `STATUS` requests through the existing `SerialByteStreamTransport`.

The script keeps `PROBE_DIRECTION=subsystem-to-mac` as a diagnostic mode, but the registered proof boundary is the `mac-to-subsystem` direction because that is the next `ground_ttc_gateway -> subsystem.local` ingress direction. Later diagnostics found that clean subsystem-origin cold-first traffic into a passive macOS receiver was not proven by this change.

Rationale:

- keeps this change focused on physical serial reachability
- avoids mixing TT&C, GDS, OBC ground-link, or CSP behavior into the preflight verdict
- reuses the already governed `mock-text` protocol semantics without introducing another wire format
- matches the next lab serial ingress step more closely than a subsystem-initiated first transmission

### Decision: Keep endpoint paths explicit

The host probe script will require `HOST_SERIAL_DEVICE` and `SUBSYSTEM_SIM_COMM_DEVICE`. It will not hard-code `/dev/cu.*`, `/dev/serial0`, or any specific adapter identity.

Rationale:

- serial names can change after reconnects
- the same probe should work with different lab adapters
- evidence must show exactly which physical endpoints were used

### Decision: Preserve native `subsystem.local` build flow

The preflight assumes the existing `sync_subsystem_sim_workspace.sh` and `bootstrap_subsystem_sim_workspace.sh` flow has produced native Linux simulator binaries on `subsystem.local`. The probe script may optionally run that prep, but the formal evidence will list those prep commands separately when used.

Rationale:

- avoids changing the platform build model during COMM bring-up
- keeps cross-compilation as a later platform change
- keeps this proof bounded to the new UART wiring

## Risks / Trade-offs

- **[Risk] A successful `mock-text` exchange could be mistaken for TT&C or unsolicited downlink proof** -> Mitigation: evidence and registry wording state that this proves only macOS-initiated physical serial request/reply reachability.
- **[Risk] Serial device names drift** -> Mitigation: require explicit device path inputs and record them in evidence.
- **[Risk] Native subsystem build may be stale** -> Mitigation: verification instructions include the existing subsystem sync/bootstrap prep before the hardware probe.

## Migration Plan

1. Add the serial probe executable and CMake registration.
2. Add the host-orchestrated subsystem UART probe script.
3. Update OpenSpec delta specs and reviewer-facing baseline docs.
4. Run fresh local verification, subsystem prep, the focused UART probe, and OpenSpec validation.
5. Record evidence and stop before TT&C ingress unless the UART preflight passes.

## Open Questions

- none for this slice
