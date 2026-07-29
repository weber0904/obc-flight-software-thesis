# command-envelope-metadata-v1 Evidence

Date: 2026-05-09.

Branch: `feature/command-envelope-metadata-v1`.

Base commit: `652b4f7b0785ae01d31d8659e3bce92e891d8a48` (`feat(comm): add command ingress source index foundation (#57)`).

Final commit SHA: recorded in the PR/closeout report. This evidence file is committed as part of the final change, so embedding the final commit SHA in this file would make the commit self-referential and unstable.

OpenSpec change: `command-envelope-metadata-v1`.

## Scope

This record covers project-owned mission command envelope metadata carriage over the existing routed `Fw.Com` command path:

- legacy non-envelope commands remain supported
- valid envelope commands are unwrapped by `CommandIngressAuthority`
- the inner F Prime command is evaluated by the existing authority policy
- `session_id` and `sequence_number` are observed through event/telemetry only
- malformed envelope candidates fail closed before `CmdDispatcher`

This record does not claim trusted source, per-packet provenance, active command session enforcement, replay protection, authentication, crypto/MAC/signature behavior, physical UHF provenance, simultaneous dual-link runtime proof, reliable transfer, file/unknown uplink authority, RF behavior, target/Pi behavior, or full link/uplink authority.

The hosted proof uses the existing default hosted CCSDS S-band routed command path and the currently wired authority ingress index `0` only.

## Build And Verification

Commands run:

```text
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut
$PWD/fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R '^OBC_Components_CommandIngressAuthority_ut_exe$' --output-on-failure
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util check
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-envelope-metadata-v1
bash scripts/run_command_ingress_authority_probe.sh
bash scripts/run_command_envelope_metadata_probe.sh
openspec validate command-envelope-metadata-v1
openspec validate --specs
python3 scripts/check_repo_consistency.py
```

Result summary:

- `OBC_Components_CommandIngressAuthority_ut_exe`: PASS after fixing raw inner-command serialization to omit F Prime buffer length prefixes and preserving safely decoded inner opcodes for malformed-envelope synthetic responses.
- `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util check`: PASS, 47/47 tests.
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-envelope-metadata-v1`: PASS.
  - `01_generate`: PASS
  - `02_build`: PASS
  - `03_generate_ut`: PASS
  - `04_build_ut`: PASS
  - `05_check_all`: PASS
  - `06_check_repo_consistency`: PASS
  - `07_check_component_test_baseline`: PASS
  - `08_check_legacy_zmq_retired`: PASS
  - `09_openspec_validate_specs`: PASS
- `scripts/run_command_ingress_authority_probe.sh`: PASS after the fresh local gate.
- `scripts/run_command_envelope_metadata_probe.sh`: PASS after the fresh local gate.
- `openspec validate command-envelope-metadata-v1`: PASS.
- `openspec validate --specs`: PASS.
- `python3 scripts/check_repo_consistency.py`: PASS.

## Component And Helper Coverage

`CommandEnvelopeMetadata` helper coverage includes:

- valid envelope parsing
- bad magic rejection
- unsupported version rejection
- nonzero flags rejection
- wrong header length rejection
- nonzero reserved rejection
- truncated inner command rejection
- duplicate/lower sequence values parsed as metadata without active enforcement

`CommandIngressAuthority` component coverage includes:

- legacy command forwarding remains unchanged
- valid envelope unwrap forwards exactly one inner command
- envelope observation event includes ingress port, configured link identity/role, session ID, sequence number, and inner opcode
- `session_id` and `sequence_number` do not affect authority decisions
- UHF backup enveloped mode-change command is rejected by the existing policy before `CmdDispatcher`
- malformed envelope emits exactly one synthetic `FORMAT_ERROR` response and does not forward
- `Fw.Com.context` remains command-status correlation and does not affect configured source identity or metadata

## Hosted Envelope Probe

Command run after the fresh build:

```text
bash scripts/run_command_envelope_metadata_probe.sh
```

Result:

```text
command_envelope_metadata_probe: PASS
sband-root=/tmp/command-envelope-metadata.bmDpg9/sband-primary
uhf-root=/tmp/command-envelope-metadata.bmDpg9/uhf-backup
legacy=MODE_GET through fprime-cli remained supported
sband-envelope=MODE_SET IDLE unwrapped on hosted ingress port 0 and changed SYS_MODE to IDLE
uhf-envelope=MODE_GET unwrapped and dispatched under uhf-backup profile
uhf-denied-envelope=MODE_SET IDLE observed metadata then rejected by authority before CmdDispatcher
scope=hosted evidence covers current routed ingress port 0 only
```

### Legacy Command Evidence

The probe first sends a normal non-envelope command through `fprime-cli`:

```text
$ fprime-cli command-send ... OBCApp.modeManager.MODE_GET
returncode=0
```

Event evidence:

```text
2026-05-09T19:10:03.405077: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 18
2026-05-09T19:10:03.405146: CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10030001 completed
```

Interpretation: the legacy `fprime-cli -> GDS -> CCSDS -> OBC` command path remains supported.

### S-Band Enveloped Command Evidence

Profile: `sband-primary` (`SBAND + PRIMARY`) configured on ingress port `0`.

The repo-owned injector sends an outer `OBC_COMMAND_ENVELOPE_V1_OPCODE` command with `session_id=1001`, `sequence_number=1`, and inner `OBCApp.modeManager.MODE_SET IDLE`.

Event evidence:

```text
2026-05-09T19:10:14.014432: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 1 role 1 session 1001 sequence 1 inner opcode 0x10030000
2026-05-09T19:10:14.014552: OBCApp.modeManager.SYS_MODE_CHANGE : System mode changed to IDLE
2026-05-09T19:10:14.014616: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030000 dispatched to port 18
2026-05-09T19:10:14.014631: CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10030000 completed
```

Telemetry evidence:

```text
2026-05-09T19:10:14.014608: OBCApp.modeManager.SYS_MODE,268632064,IDLE
```

Interpretation: the envelope was unwrapped before authority evaluation, metadata was observed, the inner command reached `CmdDispatcher`, and `SYS_MODE` reached `IDLE`.

### UHF Backup Enveloped Denial Evidence

Profile: `uhf-backup` (`UHF + BACKUP`) configured on ingress port `0`.

The probe sends:

- enveloped `OBCApp.modeManager.MODE_GET` with `session_id=2002`, `sequence_number=1`
- enveloped `OBCApp.modeManager.MODE_SET IDLE` with `session_id=2002`, `sequence_number=2`

Allowed read/status evidence:

```text
2026-05-09T19:10:58.051449: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 2 role 2 session 2002 sequence 1 inner opcode 0x10030001
2026-05-09T19:10:58.051567: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 18
2026-05-09T19:10:58.051618: CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10030001 completed
```

Denied mode-change evidence:

```text
2026-05-09T19:11:08.501933: OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 2 role 2 session 2002 sequence 2 inner opcode 0x10030000
2026-05-09T19:11:08.502030: OBCApp.commandIngressAuthority.COMMAND_AUTHORITY_REJECTED : Command authority rejected opcode 0x10030000 ingress 0 identity 2 role 2 class 2 reason 1 response VALIDATION_ERROR
```

Telemetry evidence:

```text
2026-05-09T19:10:58.051427: OBCApp.commandIngressAuthority.ENVELOPE_OBSERVED_TOTAL,268718092,1
2026-05-09T19:10:58.051428: OBCApp.commandIngressAuthority.ENVELOPE_LAST_SESSION_ID,268718094,2002
2026-05-09T19:10:58.051429: OBCApp.commandIngressAuthority.ENVELOPE_LAST_SEQUENCE_NUMBER,268718095,1
2026-05-09T19:10:58.051430: OBCApp.commandIngressAuthority.ENVELOPE_LAST_INNER_OPCODE,268718096,268632065
2026-05-09T19:10:57.959941: OBCApp.modeSafetyController.MODE_SAFETY_LAST_CURRENT_MODE,268668930,SAFE
```

Probe assertions:

- UHF backup enveloped `MODE_GET` is observed and dispatched.
- UHF backup enveloped `MODE_SET IDLE` is observed, then rejected by the existing authority policy with `VALIDATION_ERROR`.
- Denied enveloped `MODE_SET IDLE` does not emit downstream `CmdDispatcher` dispatch/completion for opcode `0x10030000`.
- Denied enveloped `MODE_SET IDLE` does not emit `SYS_MODE_CHANGE`; mode remains `SAFE` for this hosted run.

## Reused And Updated Verification Paths

Reused baseline:

- default hosted CCSDS S-band routed command path from `ccsds-sband-hosted-adoption-v1`
- hosted command ingress authority profile path from `command-ingress-authority-v1`
- topology-configured source-index authority path from `command-ingress-source-index-v1`

Updated by this change:

- hosted command ingress authority profile path now includes envelope metadata carriage over outer `FW_PACKET_COMMAND + OBC_COMMAND_ENVELOPE_V1_OPCODE`, pre-dispatch unwrap, metadata observation, and inner-command authority evaluation.

## Deferred Work

- active command session/sequence enforcement
- command replay/duplicate rejection
- authenticated command envelope, nonce, MAC/signature, or crypto
- persistent session state or reboot recovery
- per-packet trusted source/provenance
- source ID inside envelope v1
- physical UHF provenance, RF behavior, target/Pi behavior, or simultaneous dual-link runtime proof
- file packet or unknown packet uplink authority
- full link authority or full uplink authority
