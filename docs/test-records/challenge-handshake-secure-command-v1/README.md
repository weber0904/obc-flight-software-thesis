# challenge-handshake-secure-command-v1 Evidence

Date:
- `2026-05-31`

OpenSpec change:
- `challenge-handshake-secure-command-v1`

## Scope

This record proves the new hosted secure authorization and post-auth secure
command path on the active `TopCcsds` baseline. The new path is parallel to
legacy command envelope v1 and does not require wire-level `SESSION_OPEN` once
authorization succeeds.

Framework/submodule delta carried by this change:

- This change includes a narrow checked-out `lib/fprime` delta in:
  - `lib/fprime/Svc/Subtopologies/ComCcsds/ComCcsds.fpp`
  - `lib/fprime/Svc/Subtopologies/ComCcsds/ComCcsdsConfig/ComCcsdsConfig.fpp`
- The delta adds a formal handshake queue/input so `FW_PACKET_HAND` downlink
  packets travel through the governed CCSDS framer/driver path instead of
  reusing event/tlm side-band output.
- The parent repo now carries that framework delta through the pinned
  `lib/fprime` submodule commit and `.gitmodules` fork URL, not as an
  uncommitted local-only submodule working tree.
- Treat this as an explicit review item in any later GitHub PR for this
  change; it is a deliberate integration patch, not an accidental framework
  edit.

Path under test:

```text
stock fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> shared hosted TopCcsds
stock fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> PTY-backed uhf_comm_csp_node(node 6) -> shared hosted TopCcsds
```

Newly proven in this change:

- `SecureLinkAuthorizer` owns handshake APID `0x00FE` on both comm-managed
  hosted ingress paths.
- `ReqAuth(ServiceID) -> Challenge -> Response -> Authenticated` is now a
  formal hosted secure-command bootstrap flow.
- `Authenticated` success synthesizes the repo-internal opened-session state in
  `CommandIngressAuthority`; the new path does not send wire-level
  `SESSION_OPEN`.
- S-band `ServiceID = 1` secure auth plus secure command v2
  `EPS_GET_STATUS(seq=1)` succeeds on the default hosted path.
- Default hosted pre-switch physical node-`6` `uhf-backup` secure auth plus
  read/status continuity now succeeds without first switching the active role
  to UHF primary.
- `uhf-backup` secure auth does not widen authority: high-authority
  `MODE_SET` remains rejected until the runtime role changes.
- Switching to `uhf-primary-after-failover` revokes the existing UHF secure
  auth/session state and requires a new UHF auth cycle before higher-authority
  commands are accepted.
- The secure auth/session timeout clears after `180 s` of inactivity and later
  secure commands are rejected until a new auth cycle completes.
- Legacy command envelope v1 `SESSION_OPEN` plus bounded post-open command flow
  remains non-regressed on the same hosted baseline.

This record does **not** prove:

- target/lab or RF behavior
- encryption
- `uhf-backup` reliable transfer
- generic file or unknown-packet uplink authority
- hardware-backed key storage, persistent secure key storage, or full secure
  boot
- full replay protection outside challenge-auth bootstrap plus strict
  per-session sequence
- removal of legacy command envelope v1

## Implemented Probe Entry Points

- [scripts/run_challenge_handshake_secure_command_hosted_probe.sh]($REPO_ROOT/scripts/run_challenge_handshake_secure_command_hosted_probe.sh)
- [scripts/challenge_handshake_secure_command_hosted_probe.py]($REPO_ROOT/scripts/challenge_handshake_secure_command_hosted_probe.py)
- [scripts/security_server_sim.py]($REPO_ROOT/scripts/security_server_sim.py)
- [scripts/secure_link_auth_lib.py]($REPO_ROOT/scripts/secure_link_auth_lib.py)

## Hosted Verdict

Verdict: `PASS` for the hosted handshake-secure-command path, including the
default pre-switch physical node-`6` `uhf-backup` secure-auth slice.

Repository-owned proof entrypoint:

```bash
bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-31` |
| formal verdict | `challenge-handshake-secure-command-hosted` |
| logs | `/tmp/challenge-auth-clean` |
| security-server log | `/tmp/challenge-auth-clean/security-server.log` |
| stack root | `/tmp/challenge-auth-clean/combined-stack` |
| runtime root | `/tmp/challenge-auth-clean/runtime-root/combined` |
| S-band capture | `/tmp/challenge-auth-clean/combined-stack/captures/sband-southbound-to-gds.bin` |
| UHF capture | `/tmp/challenge-auth-clean/combined-stack/captures/uhf-southbound-to-gds.bin` |
| OBC log | `/tmp/challenge-auth-clean/combined-stack/logs/obc.log` |
| S-band events log | `/tmp/challenge-auth-clean/combined-stack/sband-ground/logs/events.log` |
| UHF events log | `/tmp/challenge-auth-clean/combined-stack/uhf-ground/logs/events.log` |

Observed PASS markers:

```text
challenge-handshake-secure-command-hosted-probe: PASS
formal-verdict=challenge-handshake-secure-command-hosted
secure-auth-service-1=sband
secure-auth-service-2=uhf
handshake-apid=0x00FE
secure-command-apid=0x0000
case-sband-secure-auth-eps-status=PASS
case-uhf-backup-secure-auth-read-continuity-and-high-authority-deny=PASS
case-uhf-primary-reauth-required-and-high-authority-after-reauth=PASS
case-secure-auth-inactivity-timeout-clears-session=PASS
case-legacy-v1-session-open-non-regression=PASS
```

What this passing proof means:

- S-band auth bootstrap is reviewable as discrete handshake packets on APID
  `0x00FE`, not as event/tlm side-band data.
- UHF secure auth now proves a stricter path than the earlier switched-UHF
  adoption record: the hosted default role state remains on S-band while node
  `6` accepts the secure handshake and bounded backup-role read/status traffic.
- The current hosted UHF runtime still keeps role separation intact:
  `uhf-backup` and `uhf-primary-after-failover` share `ServiceID = 2`, but the
  switch to failover-primary revokes the old auth/session and requires re-auth.
- The passing proof depends on formal handshake downlink queueing and the
  aggregator timeout plumbing on both bands; without that flush path, the small
  hosted handshake packets on node `6` can stall before reaching GDS.

Proof boundary preserved by this record:

- the proof is hosted-only
- the proof uses the maintained per-band stock GDS plus gateway surfaces
- the proof includes command acceptance and rejection behavior only after
  authentication completes
- the proof does not widen file/downlink, reliable-transfer, RF, or target
  claims

## Reused Adjacent Evidence

This change intentionally keeps adjacent legacy and hosted COMM paths separate.
The new secure-auth proof reuses the maintained hosted operator baseline but
does not replace these separate evidence families:

- [docs/test-records/per-band-stock-ground-stacks-v1/README.md](../per-band-stock-ground-stacks-v1/README.md)
- [docs/test-records/uhf-beacon-suppression-runtime-v1/README.md](../uhf-beacon-suppression-runtime-v1/README.md)
- [docs/test-records/uhf-primary-packet-quiet-decoupling-v1/README.md](../uhf-primary-packet-quiet-decoupling-v1/README.md)
- [docs/test-records/uhf-ccsds-hosted-adoption-v1/README.md](../uhf-ccsds-hosted-adoption-v1/README.md)
- [docs/test-records/command-auth-envelope-v1/README.md](../command-auth-envelope-v1/README.md)
- [docs/test-records/command-session-lifecycle-v1/README.md](../command-session-lifecycle-v1/README.md)

## Verification

Focused local verification used for this change:

| Step | Command | Result |
|---|---|---|
| SecureLinkAuthorizer UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_SecureLinkAuthorizer_ut_exe` | PASS |
| CommandIngressAuthority UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe` | PASS |
| CommController UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe` | PASS |
| hosted secure-auth probe | `bash scripts/run_challenge_handshake_secure_command_hosted_probe.sh` | PASS |
| OpenSpec change validate | `openspec validate challenge-handshake-secure-command-v1` | PASS |
| OpenSpec specs validate | `openspec validate --specs` | PASS |

## Path Registration Impact

This record creates a new reusable hosted proof boundary for:

- handshake APID `0x00FE` reviewable secure auth bootstrap
- secure command v2 on APID `0x0000`
- default hosted pre-switch physical node-`6` `uhf-backup` secure-auth and
  read/status continuity
- UHF failover-primary re-auth requirement before higher-authority command
  acceptance
- secure-auth inactivity-timeout clearing

It does not widen the meaning of:

- legacy command envelope v1 `SESSION_OPEN`
- hosted switched-UHF reliable transfer
- hosted UHF packet quiet or beacon suppress/runtime
- target/lab UHF behavior
