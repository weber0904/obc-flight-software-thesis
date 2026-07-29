# command-auth-envelope-v1 Evidence

Date: 2026-05-11.

Branch: `feature/command-auth-envelope-v1`.

Base commit: `84dad5feeed2394383d980419d7b3475f1e388b2` (`docs(roadmap): sync session lifecycle merge closeout (#65)`).

Final commit SHA: recorded in the PR/closeout report. This evidence file is committed as part of the final change, so embedding the final commit SHA in this file would make the commit self-referential and unstable.

OpenSpec change: `command-auth-envelope-v1`.

## Scope

This record closes the first authenticated command ingress boundary on top of the existing routed `Fw.Com` command path:

- valid command envelope v1 traffic now carries authenticated `source_id`, `key_slot`, `session_id`, `sequence_number`, inner command payload, and `HMAC-SHA256` MAC material
- `CommandIngressAuthority` now enforces `parse -> auth -> authority -> lifecycle -> sequence -> dispatch` for valid enveloped traffic on the active hosted ingress path
- authenticated source identity is cross-checked against repo-controlled runtime config for the configured ingress source; envelope identity alone is not trusted
- bad MAC, malformed auth envelope, unknown key slot, and source mismatch all fail closed before authority, lifecycle, or sequence state mutation
- auth-pass but authority-denied traffic does not implicitly open a session, does not mutate active session metadata, and does not consume sequence state
- auth-pass but lifecycle-denied traffic does not consume sequence state
- `SESSION_OPEN(seq0)` remains the only lifecycle open / replace / resync surface after auth passes
- `uhf-backup` keeps authenticated read/status continuity without gaining high-authority mode-change rights
- legacy non-envelope commands remain supported, but remain explicitly outside the authenticated claim and do not inherit authenticated session semantics

This record does not claim full replay protection, persistent anti-replay state, persistent secure key storage, boot trust chain, simultaneous dual-link runtime proof, hosted proof for authority ingress port `1`, physical UHF provenance, RF behavior, Raspberry Pi target proof, or file/unknown uplink authority.

The hosted proof uses the existing default hosted CCSDS S-band routed command path and the currently wired authority ingress index `0` only.

## Build And Verification

Commands run:

```text
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test
bash scripts/run_command_auth_envelope_probe.sh
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-auth-envelope-v1
openspec validate command-auth-envelope-v1
openspec validate --specs
```

Result summary:

- `fprime-util generate -f`: PASS.
- `fprime-util build`: PASS.
- `fprime-util generate --ut -f`: PASS.
- `fprime-util build --ut`: PASS.
- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe`: PASS, `54/54` tests.
- `./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test`: PASS.
- `bash scripts/run_command_auth_envelope_probe.sh`: PASS.
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-auth-envelope-v1`: PASS.
  - `01_generate`: PASS.
  - `02_build`: PASS.
  - `03_generate_ut`: PASS.
  - `04_build_ut`: PASS.
  - `05_check_all`: PASS.
  - `06_check_repo_consistency`: PASS.
  - `07_check_component_test_baseline`: PASS.
  - `08_check_legacy_zmq_retired`: PASS.
  - `09_openspec_validate_specs`: PASS.
- `openspec validate command-auth-envelope-v1`: PASS.
- `openspec validate --specs`: PASS.

## Component And Helper Coverage

`CommandIngressAuthority` component coverage includes:

- valid authenticated `SESSION_OPEN(seq0)` acceptance
- valid authenticated increasing post-open sequence acceptance
- bad MAC rejection before authority/lifecycle/sequence mutation
- source mismatch rejection before session state mutation
- malformed envelope rejection before any authenticated acceptance evidence
- lifecycle mismatch rejection without sequence consumption
- duplicate/lower/wraparound sequence rejection after auth and lifecycle succeed
- `uhf-backup` authenticated read/status continuity while high-authority mode-change remains authority denied and does not consume session or sequence state
- reboot/recreation coverage proving in-memory lifecycle reset still requires fresh authenticated reopen

Helper coverage includes:

- authenticated envelope parse/serialize
- matching source/key/payload MAC acceptance
- RFC 4231 `HMAC-SHA256` known-answer vectors
- payload tamper rejection as `BAD_MAC`
- source mismatch rejection
- key-slot mismatch rejection
- missing or disabled auth config rejection
- authority-profile and auth-source mismatch fail-closed runtime-config rejection
- malformed MAC-length, truncation, and trailing-bytes rejection

## Hosted Authenticated Envelope Probe

Command run after the fresh build:

```text
bash scripts/run_command_auth_envelope_probe.sh
```

Result:

```text
command_auth_envelope_probe: PASS
sband-root=/tmp/command-auth-envelope.EU7WuW/sband-auth
reboot-root=/tmp/command-auth-envelope.EU7WuW/reboot-auth
uhf-root=/tmp/command-auth-envelope.EU7WuW/uhf-auth
auth-valid-open=authenticated SESSION_OPEN(seq0) accepted on hosted ingress port 0
auth-valid-sequence=authenticated post-open increasing sequence command accepted and dispatched
auth-bad-mac=bad MAC rejected fail-closed before session open or sequence mutation
auth-malformed-unknown-key=malformed envelope and unknown key slot both rejected fail-closed
auth-authority-boundary=auth-pass uhf authority-denied MODE_SET did not consume sequence state or expand privilege
auth-lifecycle-boundary=auth-pass stale old session failed at lifecycle and current session seq1 remained acceptable
auth-reboot-reopen=runtime restart cleared in-memory lifecycle state and required fresh authenticated reopen
ordering=parse -> auth -> authority -> lifecycle -> sequence -> dispatch on current hosted ingress port 0
scope=authenticated command ingress foundation only; no full replay protection, persistent secure storage, Pi/RF proof, or simultaneous dual-link claim
```

### S-Band Auth Failure And Acceptance Evidence

Profile: `sband-primary` (`SBAND + PRIMARY`) configured on ingress port `0`, with runtime-config-backed auth tuple `source_id=1`, `key_slot=1`, and shared HMAC key.

Malformed auth envelope rejection:

```text
2026-05-11T21:36:03.885355: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_REJECTED : Command envelope rejected ingress 0 reason 9 response FORMAT_ERROR
```

Unknown key slot and bad MAC rejection before authenticated acceptance:

```text
2026-05-11T21:36:05.369093: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_AUTH_REJECTED : Command envelope auth rejected ingress 0 identity 1 role 1 source 1 key slot 99 session 8001 sequence 1 inner opcode 0x10030001 reason 3 response VALIDATION_ERROR
2026-05-11T21:36:06.858232: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_AUTH_REJECTED : Command envelope auth rejected ingress 0 identity 1 role 1 source 1 key slot 1 session 8002 sequence 1 inner opcode 0x10030001 reason 4 response VALIDATION_ERROR
```

Authenticated `SESSION_OPEN(seq0)` and increasing sequence dispatch:

```text
2026-05-11T21:36:09.396278: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 8002 replaced 0
2026-05-11T21:36:10.882119: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030000 dispatched to port 19
```

Replacement open and stale old-session lifecycle rejection:

```text
2026-05-11T21:36:12.419753: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 8003 replaced 1
2026-05-11T21:36:13.901478: OBCApp.commandIngressAuthority.COMMAND_SESSION_REJECTED : Command session rejected ingress 0 identity 1 role 1 session 8002 sequence 2 inner opcode 0x10030001 reason 2 response VALIDATION_ERROR
2026-05-11T21:36:15.376667: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

Interpretation: malformed auth fails at parse, bad key-slot and bad MAC fail at auth, valid `SESSION_OPEN(seq0)` is the only accepted lifecycle open, and stale old-session traffic still fails closed at lifecycle before sequence or dispatch.

### Reboot-Reopen Evidence

Profile: `sband-primary` on ingress port `0`, reusing the same runtime roots before and after restart.

Before restart:

```text
2026-05-11T21:36:26.352462: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 8101 replaced 0
2026-05-11T21:36:27.870257: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

After restart:

```text
2026-05-11T21:36:31.260170: OBCApp.commandIngressAuthority.COMMAND_SESSION_REJECTED : Command session rejected ingress 0 identity 1 role 1 session 8101 sequence 2 inner opcode 0x10030001 reason 1 response VALIDATION_ERROR
2026-05-11T21:36:32.775729: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 8102 replaced 0
2026-05-11T21:36:34.266477: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

Interpretation: reboot still clears the in-memory lifecycle/sequence state. This change authenticates the reopen contract, but does not add reboot-persistent anti-replay or secure persistence.

### UHF Backup Authenticated Continuity Boundary

Profile: `uhf-backup` (`UHF + BACKUP`) configured on ingress port `0`, with runtime-config-backed auth tuple `source_id=2`, `key_slot=2`, and distinct shared HMAC key.

Authenticated open and allowed read/status traffic:

```text
2026-05-11T21:36:45.012998: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 2 role 2 session 8201 replaced 0
2026-05-11T21:36:46.530640: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

Auth-pass but authority-denied mode-change does not consume sequence state:

```text
2026-05-11T21:36:48.057493: OBCApp.commandIngressAuthority.COMMAND_AUTHORITY_REJECTED : Command authority rejected opcode 0x10030000 ingress 0 identity 2 role 2 class 2 reason 1 response VALIDATION_ERROR
2026-05-11T21:36:49.569256: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

Telemetry continuity evidence:

```text
2026-05-11T21:36:52.002373,(2(0)-1778506432:2373),OBCApp.modeSafetyController.MODE_SAFETY_LAST_CURRENT_MODE,268668930,SAFE
2026-05-11T21:36:52.249367,(2(0)-1778506432:249367),OBCApp.modeSafetyController.MODE_SAFETY_LAST_CURRENT_MODE,268668930,SAFE
```

Interpretation: `uhf-backup` keeps authenticated low-authority read/status continuity, denied high-authority traffic does not mutate lifecycle/sequence acceptance, and the addition of auth does not widen runtime authority.

## Reused And Updated Verification Paths

Reused baseline:

- default hosted CCSDS S-band routed command path from `ccsds-sband-hosted-adoption-v1`
- hosted command ingress authority profile path from `command-ingress-authority-v1`
- hosted envelope metadata, session sequence, and session lifecycle proof boundaries from the prior command-path records

Updated by this change:

- hosted command ingress authority / envelope / session path now proves authenticated envelope verification on the current routed ingress port `0`
- the hosted path order is now `parse -> auth -> authority -> lifecycle -> sequence -> dispatch`
- malformed/auth-failed traffic is not counted as authenticated session observation or accepted lifecycle/sequence progress

## Deferred Work

- full replay protection, nonce design, or reboot-persistent anti-replay state
- persistent secure key storage or remote key catalog loading
- boot trust chain or authenticated key provenance
- simultaneous dual-link runtime proof
- hosted proof for authority ingress port `1`
- file packet or unknown packet uplink authority
- physical UHF provenance, RF behavior, or Raspberry Pi target proof
