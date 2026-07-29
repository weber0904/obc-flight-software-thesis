# uplink-authority-and-key-hardening-v1 Evidence

Date:
- `2026-05-31`

OpenSpec change:
- `uplink-authority-and-key-hardening-v1`

## Scope

This record proves the hosted-baseline follow-on hardening slice above
`challenge-handshake-secure-command-v1`. The change closes three bounded
surfaces without broadening into target proof, encryption, or generic
arbitrary-file governance:

- one tracked hosted keystore contract now owns comm-managed secure-auth root
  keys and retained comm-managed legacy v1 auth defaults
- comm-managed unknown uplink is formally handshake-only on APID `0x00FE` and
  fail-closed for malformed or unsupported packets
- `.sequence-staging/<leaf>` file upload on the active hosted CCSDS path now
  requires both active secure auth and current runtime file-role allow

This record does **not** prove:

- target/lab or RF behavior
- encryption
- persistent secure key storage or hardware-backed key storage
- generic arbitrary file-uplink authority beyond `.sequence-staging/<leaf>`
- generic unknown-uplink authority beyond handshake APID `0x00FE`
- retirement of legacy v1 command envelopes

## Implemented Entry Points

- [scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh](../../../scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh)
- [scripts/uplink_authority_and_key_hardening_hosted_probe.py](../../../scripts/uplink_authority_and_key_hardening_hosted_probe.py)
- [scripts/security_server_sim.py](../../../scripts/security_server_sim.py)
- [scripts/secure_link_auth_lib.py](../../../scripts/secure_link_auth_lib.py)
- [config/security/command-auth.ini](../../../config/security/command-auth.example.ini)

The hosted runtime and repo-owned helper stack now share this tracked keystore
through one fixed repo path; hosted `--command-auth*` and
`--command-auth-keystore` overrides are not part of this verified path.

## Hosted Verdict

Verdict: `PASS` for the hosted staged-file and unknown-uplink authority closure
path on the active comm-managed baseline.

Repository-owned proof entrypoint:

```bash
bash scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-31` |
| formal verdict | `uplink-authority-and-key-hardening-hosted` |
| security-server log | `/tmp/uplink-authority-and-key-hardening-hosted.J3hPIn/security-server.log` |
| stack root | `/tmp/uplink-authority-and-key-hardening-hosted.J3hPIn/combined-stack` |
| runtime root | `/tmp/uplink-authority-and-key-hardening-hosted.J3hPIn/runtime-root/combined` |
| S-band capture | `/tmp/uplink-authority-and-key-hardening-hosted.J3hPIn/combined-stack/captures/sband-southbound-to-gds.bin` |
| UHF capture | `/tmp/uplink-authority-and-key-hardening-hosted.J3hPIn/combined-stack/captures/uhf-southbound-to-gds.bin` |
| OBC log | `/tmp/uplink-authority-and-key-hardening-hosted.J3hPIn/combined-stack/logs/obc.log` |
| S-band events log | `/tmp/uplink-authority-and-key-hardening-hosted.J3hPIn/combined-stack/sband-ground/logs/events.log` |
| UHF events log | `/tmp/uplink-authority-and-key-hardening-hosted.J3hPIn/combined-stack/uhf-ground/logs/events.log` |

Observed PASS markers:

```text
uplink-authority-and-key-hardening-hosted-probe: PASS
formal-verdict=uplink-authority-and-key-hardening-hosted
case-malformed-handshake-fail-closed=PASS
case-sband-secure-auth-staged-upload-and-sequence-validate=PASS
case-uhf-backup-staged-upload-denied=PASS
case-uhf-failover-primary-reauth-staged-upload-and-sequence-validate=PASS
case-legacy-v1-keystore-backed-non-regression=PASS
```

What this passing proof means:

- hosted OBC and repo-owned ground helper tooling now share the same tracked
  keystore asset instead of receiving hosted command-auth keys through runtime
  injection flags or env vars
- malformed or unsupported APID `0x00FE` traffic is rejected without creating
  auth state
- S-band secure auth plus staged file upload succeeds and the admitted official
  sequence wrapper path remains reviewable
- `uhf-backup` secure auth does not reopen staged upload authority
- explicit-switched `uhf-primary-after-failover` requires re-auth before
  staged upload succeeds
- retained comm-managed legacy v1 compatibility still works under the same
  tracked keystore contract

Proof boundary preserved by this record:

- hosted-only
- current comm-managed per-band stock-stack surfaces only
- bounded `.sequence-staging/<leaf>` governance only
- no target proof, encryption, or generic arbitrary-file authority claim

## Reused Adjacent Evidence

This record reuses but does not replace these adjacent hosted proof families:

- [evidence/records/challenge-handshake-secure-command-v1/README.md](../challenge-handshake-secure-command-v1/README.md)
- [evidence/records/official-sequencing-system-resources-v1/README.md](../official-sequencing-system-resources-v1/README.md)
- [evidence/records/per-band-stock-ground-stacks-v1/README.md](../per-band-stock-ground-stacks-v1/README.md)

## Verification

Focused local verification used for this change:

| Step | Command | Result |
|---|---|---|
| SecureLinkAuthorizer UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_SecureLinkAuthorizer_ut_exe` | PASS |
| FileIngressAuthority UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_FileIngressAuthority_ut_exe` | PASS |
| HostedRuntime UT | `./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test` | PASS |
| CommController UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe` | PASS |
| fresh local verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| hosted staged-file / unknown-uplink probe | `bash scripts/run_uplink_authority_and_key_hardening_hosted_probe.sh` | PASS |
| OpenSpec change validate | `openspec validate uplink-authority-and-key-hardening-v1` | PASS |
| OpenSpec specs validate | `openspec validate --specs` | PASS |

## Path Registration Impact

This record creates a new reusable hosted proof boundary for:

- one tracked keystore contract across hosted OBC and repo-owned ground helper
  tooling
- APID `0x00FE` handshake-only unknown uplink reject-without-mutation
- `.sequence-staging/<leaf>` staged upload success on S-band under active
  secure auth
- `uhf-backup` staged upload denial even with secure auth
- failover-primary re-auth requirement before staged UHF upload success

It does not widen the meaning of:

- generic arbitrary file-uplink governance
- target/lab secure-auth or file-ingress behavior
- secure-command bootstrap itself, which remains separately governed by
  `challenge-handshake-secure-command-v1`
