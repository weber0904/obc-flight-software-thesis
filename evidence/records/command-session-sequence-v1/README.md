# command-session-sequence-v1 Evidence

Date: 2026-05-09.

Branch: `feature/command-session-sequence-v1`.

Base commit: `ce3085676997a8585049ca16dd54d5dc9fe29193`.

Final commit SHA: recorded in the PR/closeout report. This evidence file is committed as part of the final change, so embedding the final commit SHA in this file would make the commit self-referential and unstable.

OpenSpec change: `command-session-sequence-v1`.

## Scope

This record covers active sequence enforcement for valid command envelope v1 packets over the existing routed `Fw.Com` command path:

- legacy non-envelope commands remain supported and are not sequence gated
- valid envelope commands are observed and authority-checked before sequence evaluation
- authority-denied envelopes do not update sequence state
- duplicate, lower, and wraparound sequence numbers are rejected before `CmdDispatcher`
- accepted sequence numbers forward the inner F Prime command exactly once

This record does not claim crypto authentication, full replay protection, reliable transfer, trusted source, physical UHF provenance, simultaneous dual-link runtime proof, file/unknown uplink authority, persistent session state, session-open/reset/resync, or reboot-persistent sequence windows.

The hosted proof uses the existing default hosted CCSDS S-band routed command path and the currently wired authority ingress index `0` only.

## Build And Verification

Commands run:

```text
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut
$PWD/fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R '^OBC_Components_CommandIngressAuthority_ut_exe$' --output-on-failure
$PWD/fprime-venv/bin/ctest --test-dir build-fprime-automatic-native-ut -R '^command_authority_catalog_check$' --output-on-failure
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-session-sequence-v1
bash scripts/run_command_ingress_authority_probe.sh
bash scripts/run_command_envelope_metadata_probe.sh
bash scripts/run_command_session_sequence_probe.sh
openspec validate command-session-sequence-v1
openspec validate --specs
python3 scripts/check_repo_consistency.py
```

Result summary:

- `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build`: PASS.
- `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut`: PASS.
- `OBC_Components_CommandIngressAuthority_ut_exe`: PASS.
- `command_authority_catalog_check`: PASS.
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-session-sequence-v1`: PASS.
  - `01_generate`: PASS
  - `02_build`: PASS
  - `03_generate_ut`: PASS
  - `04_build_ut`: PASS
  - `05_check_all`: PASS
  - `06_check_repo_consistency`: PASS
  - `07_check_component_test_baseline`: PASS
  - `08_check_legacy_zmq_retired`: PASS
  - `09_openspec_validate_specs`: PASS
- `scripts/run_command_ingress_authority_probe.sh`: PASS.
- `scripts/run_command_envelope_metadata_probe.sh`: PASS.
- `scripts/run_command_session_sequence_probe.sh`: PASS after tightening probe parsing to read actual channel values and waiting for event counts instead of fixed sleeps.
- `openspec validate command-session-sequence-v1`: PASS.
- `openspec validate --specs`: PASS.
- `python3 scripts/check_repo_consistency.py`: PASS.

## Component Coverage

`CommandIngressAuthority` component coverage includes:

- legacy non-envelope command behavior remains unchanged
- first enveloped authority-allowed command for a session forwards exactly once
- increasing enveloped command for the same session forwards exactly once
- duplicate sequence rejects before `CmdDispatcher`
- lower sequence rejects before `CmdDispatcher`
- wraparound after `0xFFFFFFFF` rejects without reset
- same sequence under different ingress port, session ID, or role is independent
- UHF backup authority-denied enveloped mode-change does not update sequence state
- after an authority-denied sequence `N`, an authority-allowed command with sequence `N` can still be accepted
- sequence window full maps to `EXECUTION_ERROR`
- sequence rejection emits `COMMAND_SEQUENCE_REJECTED`, updates bounded telemetry, produces exactly one synthetic status, and forwards zero commands
- sequence counters continue incrementing while the rejection event throttles

## Hosted Session Sequence Probe

Command run after the fresh build:

```text
bash scripts/run_command_session_sequence_probe.sh
```

Result:

```text
command_session_sequence_probe: PASS
sband-root=/tmp/command-session-sequence.L4VMsd/sband-primary
uhf-root=/tmp/command-session-sequence.L4VMsd/uhf-backup
legacy=MODE_GET through fprime-cli remained supported
sband-sequence=seq1 accepted, duplicate/lower seq1 rejected, seq2 accepted on hosted ingress port 0
uhf-sequence=read/status seq1 accepted, duplicate rejected, seq2 accepted after authority-denied mode-change
authority-order=authority-denied mode-change did not consume sequence state
scope=hosted evidence covers current routed ingress port 0 only
```

### S-Band Sequence Evidence

Profile: `sband-primary` (`SBAND + PRIMARY`) configured on ingress port `0`.

Accepted initial envelope:

```text
OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 1 role 1 session 3003 sequence 1 inner opcode 0x10030000
OBCApp.modeManager.SYS_MODE_CHANGE : System mode changed to IDLE
CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030000 dispatched to port 18
```

Duplicate/lower rejection evidence:

```text
OBCApp.commandIngressAuthority.COMMAND_SEQUENCE_REJECTED : Command sequence rejected ingress 0 identity 1 role 1 session 3003 sequence 1 inner opcode 0x10030000 reason 1 response VALIDATION_ERROR
```

Increasing sequence evidence:

```text
OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 1 role 1 session 3003 sequence 2 inner opcode 0x10030000
CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030000 dispatched to port 18
```

Interpretation: authority-allowed enveloped commands are sequence-gated after metadata observation and authority allow; duplicate/lower sequence values do not reach `CmdDispatcher`.

### UHF Backup Authority-Then-Sequence Evidence

Profile: `uhf-backup` (`UHF + BACKUP`) configured on ingress port `0`.

Allowed read/status sequence `1`:

```text
OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 2 role 2 session 4004 sequence 1 inner opcode 0x10030001
CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 18
```

Duplicate read/status rejection:

```text
OBCApp.commandIngressAuthority.COMMAND_SEQUENCE_REJECTED : Command sequence rejected ingress 0 identity 2 role 2 session 4004 sequence 1 inner opcode 0x10030001 reason 1 response VALIDATION_ERROR
```

Authority-denied high-authority command:

```text
OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 2 role 2 session 4004 sequence 2 inner opcode 0x10030000
OBCApp.commandIngressAuthority.COMMAND_AUTHORITY_REJECTED : Command authority rejected opcode 0x10030000 ingress 0 identity 2 role 2 class 2 reason 1 response VALIDATION_ERROR
```

Allowed read/status using the same sequence number after the authority-denied command:

```text
OBCApp.commandIngressAuthority.COMMAND_ENVELOPE_OBSERVED : Command envelope observed ingress 0 identity 2 role 2 session 4004 sequence 2 inner opcode 0x10030001
CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 18
```

Interpretation: the runtime order is authority-then-sequence. The authority-denied mode-change does not consume sequence `2`; an authority-allowed read/status command with sequence `2` is accepted afterward.

## Reused And Updated Verification Paths

Reused baseline:

- default hosted CCSDS S-band routed command path from `ccsds-sband-hosted-adoption-v1`
- hosted command ingress authority profile path from `command-ingress-authority-v1`
- topology-configured source-index authority path from `command-ingress-source-index-v1`
- command envelope metadata path from `command-envelope-metadata-v1`

Updated by this change:

- hosted command ingress authority profile path now includes active duplicate/lower sequence rejection for valid command envelope v1 packets after authority allow.

## Deferred Work

- command session-open/reset/resync flow
- persistent sequence state or reboot recovery
- authenticated command envelope, nonce, MAC/signature, or crypto
- full replay protection or reliable transfer
- per-packet trusted source/provenance
- physical UHF provenance, RF behavior, target/Pi behavior, or simultaneous dual-link runtime proof
- file packet or unknown packet uplink authority
- full link authority or full uplink authority
