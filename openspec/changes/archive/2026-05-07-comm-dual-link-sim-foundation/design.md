## Context

`comm_csp_node` currently provides the generic gateway-backed omitted-RF COMM path as CSP node `4`. That identity appears in existing hosted, physical serial, SocketCAN, file/downlink, and service-managed lab evidence. Reusing node `4` as either S-band or UHF would make those records ambiguous.

The existing implementation already has the right shared foundation: `CommNodeServer` owns libcsp and serial I/O, while `CommSimModel` owns bounded link behavior. This change only adds explicit process identities and evidence boundaries for later S-band and UHF work.

## Goals / Non-Goals

**Goals:**

- keep `comm_csp_node` as generic compatibility COMM node `4`
- add `sband_comm_csp_node` as S-band COMM node `5`
- add `uhf_comm_csp_node` as UHF COMM node `6`
- keep services `30`, `31`, and `32` unchanged and reused per node
- make executable, logs, launcher/probe output, and evidence identify the link explicitly
- prove hosted coexistence of generic, S-band, and UHF COMM simulator identities on the governed internal CSP substrate

**Non-Goals:**

- no complete S-band simulated-TCP-to-GDS proof
- no UHF UART/RS485/USB/macOS backup or beacon proof
- no CCSDS adoption, APID mapping, or `ComCcsds` migration
- no RF behavior, real radio behavior, reliable transfer, packet-loss recovery, or Pi hardware evidence
- no changes to `ComFprime`, existing GDS gateway wire behavior, node-4 service evidence, or COMM service packet layouts

## Decisions

### Decision: Use thin executable wrappers over shared COMM implementation

Each executable calls shared COMM node main support with a compile-time identity:

- `comm_csp_node`: link identity `generic`, default node `4`, interface `COMMCSP`
- `sband_comm_csp_node`: link identity `sband`, default node `5`, interface `SBANDCSP`
- `uhf_comm_csp_node`: link identity `uhf`, default node `6`, interface `UHFCSP`

The existing `--node-id` and `--interface-name` options remain available for diagnostic overrides, but the executable defaults define the governed identity.

### Decision: Keep one service contract per link node

S-band and UHF reuse the existing COMM service ports `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`. Node ID distinguishes the link, not a new service range or wire layout.

### Decision: Prove foundation through identity/coexistence, not full transport paths

The hosted probe starts all three COMM identities with PTY serial stand-ins, starts hosted OBC on the same internal CSP substrate with ground link disabled, verifies OBC CSP reachability to nodes `4`, `5`, and `6`, and uses a focused helper to exercise services `30-32` on each node. This proves identity and service compatibility only.

## Risks / Trade-offs

- **[Risk] New wrappers could accidentally change node-4 compatibility** -> Mitigation: keep `DEFAULT_COMM_NODE_ID = 4`, leave existing launchers/service templates unchanged, and rerun existing COMM model/groundlink tests.
- **[Risk] Probe evidence could be mistaken for full S-band or UHF path proof** -> Mitigation: record the new registry entry as hosted simulator identity/coexistence only and explicitly exclude S-band GDS, UHF UART, RF, CCSDS, reliable transfer, file/downlink, and Pi hardware.
- **[Risk] Three libcsp nodes in one hosted probe can hide logging ambiguity** -> Mitigation: use distinct executable names, interface names, PTY labels, log files, and probe output for generic, S-band, and UHF.

## Migration Plan

1. Add shared COMM node app support and thin executable entrypoints.
2. Register new simulator executables and focused live service probe helper in CMake.
3. Add the hosted dual-link foundation probe.
4. Update docs, specs, and evidence.
5. Run fresh full local verification, rerun the focused probe after that build, validate OpenSpec, archive, and prepare a review-ready branch.

Rollback strategy: remove the new wrapper targets/probe/docs and keep the existing `comm_csp_node` implementation unchanged.

## Open Questions

- none
