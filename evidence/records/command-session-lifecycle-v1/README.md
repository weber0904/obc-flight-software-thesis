# command-session-lifecycle-v1 Evidence

Date: 2026-05-11.

Branch: `feature/command-session-lifecycle-v1`.

Base commit: `e31d7d353735e98fd49d197e41c8060dbe9f6720` (`feat(obc): add EPS timeout FDIR safe fallback (#62)`).

Final commit SHA: recorded in the PR/closeout report. This evidence file is committed as part of the final change, so embedding the final commit SHA in this file would make the commit self-referential and unstable.

OpenSpec change: `command-session-lifecycle-v1`.

## Scope

This record closes the current hosted command/session runtime gap on top of the existing routed `Fw.Com` command path:

- `SESSION_OPEN` is the only v1 session open / replace / resync surface
- `SESSION_OPEN` is accepted only as a valid command envelope v1 inner opcode with `sequence_number = 0`
- accepted `SESSION_OPEN` is keyed by ingress port, link identity, and link role, and atomically replaces the active session for that source epoch
- non-lifecycle enveloped commands require an already-open matching session before sequence evaluation or dispatch
- strict-monotonic sequence enforcement still applies after the session match succeeds
- reboot or runtime restart clears in-memory session state; a fresh `SESSION_OPEN` is required before later sequence traffic resumes
- legacy non-envelope commands remain supported, but legacy direct `SESSION_OPEN` fails closed before `CmdDispatcher`
- `uhf-backup` can open a session and continue enveloped read/status traffic without gaining high-authority mode-change rights

This record does not claim authenticated source identity, crypto/MAC/signature behavior, full replay protection, persistent secure session state, reliable transfer, simultaneous dual-link runtime proof, hosted proof for authority ingress port `1`, physical UHF provenance, RF behavior, target/Pi behavior, or file/unknown uplink authority.

The hosted proof uses the existing default hosted CCSDS S-band routed command path and the currently wired authority ingress index `0` only.

## Build And Verification

Commands run:

```text
PATH="$PWD/fprime-venv/bin:$PATH" ./fprime-venv/bin/fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" ./fprime-venv/bin/fprime-util build
PATH="$PWD/fprime-venv/bin:$PATH" ./fprime-venv/bin/fprime-util generate --ut -f
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native-ut --target OBC_Components_CommandIngressAuthority_ut_exe -j 8
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
PATH="$PWD/fprime-venv/bin:$PATH" ctest --test-dir build-fprime-automatic-native-ut -R '^(OBC_Components_CommandIngressAuthority_ut_exe|command_authority_catalog_check)$' --output-on-failure
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-session-lifecycle-v1
bash scripts/run_command_ingress_authority_probe.sh
bash scripts/run_command_envelope_metadata_probe.sh
bash scripts/run_command_session_sequence_probe.sh
bash scripts/run_command_session_lifecycle_probe.sh
openspec validate command-session-lifecycle-v1
openspec validate --specs
```

Result summary:

- `fprime-util generate -f`: PASS.
- `fprime-util build`: PASS.
- `fprime-util generate --ut -f`: PASS.
- `cmake --build build-fprime-automatic-native-ut --target OBC_Components_CommandIngressAuthority_ut_exe -j 8`: PASS.
- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe`: PASS, `48/48` tests.
- `ctest --test-dir build-fprime-automatic-native-ut -R '^(OBC_Components_CommandIngressAuthority_ut_exe|command_authority_catalog_check)$' --output-on-failure`: PASS.
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-session-lifecycle-v1`: PASS.
  - `01_generate`: PASS
  - `02_build`: PASS
  - `03_generate_ut`: PASS
  - `04_build_ut`: PASS
  - `05_check_all`: PASS
  - `06_check_repo_consistency`: PASS
  - `07_check_component_test_baseline`: PASS
  - `08_check_legacy_zmq_retired`: PASS
  - `09_openspec_validate_specs`: PASS
- `bash scripts/run_command_ingress_authority_probe.sh`: PASS.
- `bash scripts/run_command_envelope_metadata_probe.sh`: PASS after updating the probe to send `SESSION_OPEN(seq0)` before legacy session traffic assumptions.
- `bash scripts/run_command_session_sequence_probe.sh`: PASS after updating the probe to send `SESSION_OPEN(seq0)` before sequence-gated traffic.
- `bash scripts/run_command_session_lifecycle_probe.sh`: PASS.
- `openspec validate command-session-lifecycle-v1`: PASS.
- `openspec validate --specs`: PASS.

## Component Coverage

`CommandIngressAuthority` component coverage includes:

- unopened enveloped command rejection before `CmdDispatcher`
- accepted `SESSION_OPEN(seq0)` opening a new session and seeding baseline sequence state
- increasing sequence after open forwarding exactly once
- duplicate, lower, and wraparound sequence rejection after open
- same-source fresh `SESSION_OPEN(new session_id)` replacing the active session and invalidating stale old-session traffic
- same-session reopen rejection
- legacy direct `SESSION_OPEN` rejection before `CmdDispatcher`
- `uhf-backup` `SESSION_OPEN` acceptance while high-authority enveloped mode-change remains authority denied and does not consume session/sequence state
- reset/recreation coverage proving the reboot-memory-clear rule for active session state

`CommandSessionSequence` helper coverage remains in place for direct strict-monotonic window behavior; this change reuses that helper rather than replacing it.

## Hosted Session Lifecycle Probe

Command run after the fresh build:

```text
bash scripts/run_command_session_lifecycle_probe.sh
```

Result:

```text
command_session_lifecycle_probe: PASS
sband-root=/tmp/command-session-lifecycle.NBD9l1/sband-lifecycle
reboot-root=/tmp/command-session-lifecycle.NBD9l1/sband-reboot
uhf-root=/tmp/command-session-lifecycle.NBD9l1/uhf-backup
legacy-boundary=legacy MODE_GET remained available while direct SESSION_OPEN failed closed
sband-lifecycle=unopened traffic rejected, SESSION_OPEN(seq0) accepted, seq1 dispatched, duplicate rejected, replacement SESSION_OPEN resynced acceptance, stale old session failed closed
reboot-recovery=OBC restart on the same runtime roots cleared active session state and required a fresh SESSION_OPEN before seq1 traffic resumed
uhf-lifecycle=uhf-backup SESSION_OPEN accepted, read/status seq1 and seq2 dispatched, authority-denied MODE_SET did not consume seq2
ordering=parse -> observe metadata -> authority -> lifecycle -> sequence -> dispatch on current hosted ingress port 0
scope=hosted evidence covers current routed ingress port 0 only; no auth, crypto, replay, simultaneous dual-link, RF, Pi, or file/unknown uplink claims
```

### S-Band Session Open, Replace, And Resync Evidence

Profile: `sband-primary` (`SBAND + PRIMARY`) configured on ingress port `0`.

Fail-closed lifecycle boundary before open:

```text
2026-05-11T18:49:36.828895: OBCApp.commandIngressAuthority.COMMAND_SESSION_REJECTED : Command session rejected ingress 0 identity 1 role 1 session 0 sequence 0 inner opcode 0x10045000 reason 5 response VALIDATION_ERROR
2026-05-11T18:49:38.436870: OBCApp.commandIngressAuthority.COMMAND_SESSION_REJECTED : Command session rejected ingress 0 identity 1 role 1 session 5005 sequence 1 inner opcode 0x10030000 reason 1 response VALIDATION_ERROR
```

Accepted open and first accepted sequence:

```text
2026-05-11T18:49:42.100729: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 1 role 1 session 5005 sequence 0 inner opcode 0x10045000
2026-05-11T18:49:42.100796: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 5005 replaced 0
2026-05-11T18:49:43.658749: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 1 role 1 session 5005 sequence 1 inner opcode 0x10030000
2026-05-11T18:49:43.658849: OBCApp.modeManager.SYS_MODE_CHANGE : System mode changed to IDLE
2026-05-11T18:49:43.658897: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030000 dispatched to port 19
```

Duplicate and same-session reopen rejection:

```text
2026-05-11T18:49:45.245464: OBCApp.commandIngressAuthority.COMMAND_SEQUENCE_REJECTED : Command sequence rejected ingress 0 identity 1 role 1 session 5005 sequence 1 inner opcode 0x10030000 reason 1 response VALIDATION_ERROR
2026-05-11T18:49:40.535400: OBCApp.commandIngressAuthority.COMMAND_SESSION_REJECTED : Command session rejected ingress 0 identity 1 role 1 session 5005 sequence 1 inner opcode 0x10045000 reason 4 response VALIDATION_ERROR
```

Replacement open and stale old-session rejection:

```text
2026-05-11T18:49:46.822330: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 5006 replaced 1
2026-05-11T18:49:48.379298: OBCApp.commandIngressAuthority.COMMAND_SESSION_REJECTED : Command session rejected ingress 0 identity 1 role 1 session 5005 sequence 2 inner opcode 0x10030000 reason 2 response VALIDATION_ERROR
2026-05-11T18:49:49.942595: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

Interpretation: valid envelope traffic is parsed and observed first, authority runs on the inner opcode, `SESSION_OPEN(seq0)` explicitly opens or replaces the session for that source epoch, stale/mismatched sessions fail closed, and strict-monotonic sequence still applies after the session match succeeds.

### Command Response And Lifecycle Telemetry Evidence

Accepted command-response evidence from component coverage:

- `testSessionOpenThenNextCommandForwardsInnerCommand` proves accepted `SESSION_OPEN(seq0)` synthesizes exactly one `Fw::CmdResponse::OK` and does not forward the lifecycle inner opcode downstream.
- `testBadOpenSequenceRejects`, `testSameSessionReopenRejects`, `testLegacyPacketSessionOpenRejectsBeforeDispatch`, and `testRecreatedComponentRequiresFreshSessionOpen` prove rejected lifecycle transitions synthesize one `VALIDATION_ERROR`.

Hosted probe command-path evidence:

```text
$ $REPO_ROOT/fprime-venv/bin/fprime-cli command-send --dictionary $REPO_ROOT/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json --no-zmq --tts-port 50720 OBCApp.modeManager.MODE_GET
returncode=0
$ $REPO_ROOT/fprime-venv/bin/fprime-cli command-send --dictionary $REPO_ROOT/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json --no-zmq --tts-port 50720 OBCApp.commandIngressAuthority.SESSION_OPEN
returncode=0
```

Runtime response evidence for rejected lifecycle traffic:

```text
2026-05-11T18:49:36.828895: OBCApp.commandIngressAuthority.COMMAND_SESSION_REJECTED : Command session rejected ingress 0 identity 1 role 1 session 0 sequence 0 inner opcode 0x10045000 reason 5 response VALIDATION_ERROR
2026-05-11T18:49:38.436870: OBCApp.commandIngressAuthority.COMMAND_SESSION_REJECTED : Command session rejected ingress 0 identity 1 role 1 session 5005 sequence 1 inner opcode 0x10030000 reason 1 response VALIDATION_ERROR
2026-05-11T18:50:30.390138: OBCApp.commandIngressAuthority.COMMAND_AUTHORITY_REJECTED : Command authority rejected opcode 0x10030000 ingress 0 identity 2 role 2 class 2 reason 1 response VALIDATION_ERROR
```

Lifecycle telemetry excerpt after accepted `uhf-backup` open:

```text
2026-05-11T18:50:18.765744,(2(0)-1778496618:765744),OBCApp.commandIngressAuthority.SESSION_OPEN_TOTAL,268718108,1
2026-05-11T18:50:18.765746,(2(0)-1778496618:765746),OBCApp.commandIngressAuthority.SESSION_ACTIVE,268718110,1
2026-05-11T18:50:18.765747,(2(0)-1778496618:765747),OBCApp.commandIngressAuthority.SESSION_ACTIVE_IDENTITY,268718112,2
2026-05-11T18:50:18.765749,(2(0)-1778496618:765749),OBCApp.commandIngressAuthority.SESSION_ACTIVE_ROLE,268718113,2
2026-05-11T18:50:18.765751,(2(0)-1778496618:765751),OBCApp.commandIngressAuthority.SESSION_ACTIVE_ID,268718114,7007
```

Interpretation: this evidence set now records command responses in two places: component coverage proves accepted `SESSION_OPEN` returns `Fw::CmdResponse::OK`, and hosted runtime excerpts show rejected lifecycle or authority paths returning `VALIDATION_ERROR`. Together with the dedicated lifecycle events and telemetry above, the record stays bounded while also showing that rejected lifecycle packets had no paired downstream `OpCodeDispatched`.

### Reboot Or Runtime Restart Recovery Evidence

Profile: `sband-primary` (`SBAND + PRIMARY`) configured on ingress port `0`, using the same runtime root before and after restart.

Before restart:

```text
2026-05-11T18:50:00.284326: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 6006 replaced 0
2026-05-11T18:50:01.873559: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

After restart with stale session state:

```text
2026-05-11T18:50:05.343699: OBCApp.commandIngressAuthority.COMMAND_SESSION_REJECTED : Command session rejected ingress 0 identity 1 role 1 session 6006 sequence 2 inner opcode 0x10030001 reason 1 response VALIDATION_ERROR
2026-05-11T18:50:06.931626: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 6007 replaced 0
2026-05-11T18:50:08.517757: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

Interpretation: reboot clears the in-memory lifecycle state completely. A stale post-restart sequence is not accepted as an implicit reopen; the operator recovery path is a fresh `SESSION_OPEN(new session_id, seq0)`.

### UHF Backup Lifecycle Boundary

Profile: `uhf-backup` (`UHF + BACKUP`) configured on ingress port `0`.

Accepted open and allowed read/status:

```text
2026-05-11T18:50:18.765753: OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 2 role 2 session 7007 replaced 0
2026-05-11T18:50:20.337106: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

Authority-denied high-authority command does not consume lifecycle/sequence state:

```text
2026-05-11T18:50:30.390083: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 2 role 2 session 7007 sequence 2 inner opcode 0x10030000
2026-05-11T18:50:30.390138: OBCApp.commandIngressAuthority.COMMAND_AUTHORITY_REJECTED : Command authority rejected opcode 0x10030000 ingress 0 identity 2 role 2 class 2 reason 1 response VALIDATION_ERROR
2026-05-11T18:50:31.978579: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 2 role 2 session 7007 sequence 2 inner opcode 0x10030001
2026-05-11T18:50:31.978617: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
```

Interpretation: `uhf-backup` can use lifecycle open and keep enveloped read/status operable, but the existing low-authority boundary remains intact. The denied mode-change is rejected at authority and does not consume sequence `2`.

## Reused And Updated Verification Paths

Reused baseline:

- default hosted CCSDS S-band routed command path from `ccsds-sband-hosted-adoption-v1`
- hosted command ingress authority profile path from `command-ingress-authority-v1`
- topology-configured source-index authority path from `command-ingress-source-index-v1`
- command envelope metadata path from `command-envelope-metadata-v1`
- command session sequence path from `command-session-sequence-v1`

Updated by this change:

- the hosted command ingress authority profile path now includes explicit session-open/session-replace/session-reboot-recovery behavior for valid command envelope v1 packets on the current routed ingress port `0`
- the hosted command envelope metadata and session sequence probes were rerun as regressions with the new required `SESSION_OPEN(seq0)` precondition

## Deferred Work

- authenticated command envelope, source ID, MAC/signature, nonce, or crypto
- full replay protection claims
- persistent secure session state or reboot-persistent replay tracking
- simultaneous S-band/UHF routed ingress proof
- hosted proof for authority ingress port `1`
- file packet or unknown packet uplink authority
- physical UHF provenance, RF behavior, target/Pi behavior, or reliable transfer
