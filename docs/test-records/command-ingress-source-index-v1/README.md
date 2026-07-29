# command-ingress-source-index-v1 Evidence

Date: 2026-05-09.

Branch: `feature/command-ingress-source-index-v1`.

Base commit: `8eea9750a372dd54ac932f4fa68a4c360dc71511` (`feat(comm): add command ingress authority gate`).

Final commit SHA: recorded in the PR/closeout report. This evidence file is committed as part of the final change, so embedding the final commit SHA in this file would make the commit self-referential and unstable.

OpenSpec changes:

- `command-ingress-source-index-v1`
- `command-session-sequence-foundation-v1`

## Scope

This record covers two related foundations in one PR:

- topology-configured `CommandIngressAuthority` source mapping by ingress port index
- inactive command session/sequence helper semantics

It does not claim trusted source, per-packet provenance, physical UHF provenance, simultaneous dual-link runtime proof, active session enforcement, replay protection, authentication, nonce/MAC/signature behavior, reliable transfer, command envelope changes, file/unknown uplink authority, RF behavior, target/Pi behavior, or full link/uplink authority.

The hosted proof uses the existing default hosted CCSDS S-band routed `Fw.Com` command path and the currently wired authority ingress index `0`. Port `1` behavior is component-level proof only.

## Build And Verification

Commands run:

```text
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native-ut --target OBC_Components_CommandIngressAuthority_ut_exe -j 8
build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
PATH="$PWD/fprime-venv/bin:$PATH" ctest --test-dir build-fprime-automatic-native-ut -R command_authority_catalog_check --output-on-failure
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native --target OBC -j 8
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-ingress-source-index-v1
bash scripts/run_command_ingress_authority_probe.sh
openspec validate command-ingress-source-index-v1
openspec validate command-session-sequence-foundation-v1
openspec validate --specs
```

Result summary:

- `OBC_Components_CommandIngressAuthority_ut_exe`: PASS, 23 tests
- `command_authority_catalog_check`: PASS
- default hosted `OBC` rebuild: PASS
- full repository verification gate: PASS
  - `01_generate`: PASS
  - `02_build`: PASS
  - `03_generate_ut`: PASS
  - `04_build_ut`: PASS
  - `05_check_all`: PASS
  - `06_check_repo_consistency`: PASS
  - `07_check_component_test_baseline`: PASS
  - `08_check_legacy_zmq_retired`: PASS
  - `09_openspec_validate_specs`: PASS
- `openspec validate command-ingress-source-index-v1`: PASS
- `openspec validate command-session-sequence-foundation-v1`: PASS
- `openspec validate --specs`: PASS

## Component And Helper Coverage

`CommandIngressAuthority` component coverage includes:

- port `0` configured `sband-primary` allows a high-authority command
- port `0` configured `uhf-backup` denies a high-authority command
- port `1` unconfigured fails closed with synthetic `EXECUTION_ERROR`
- port `1` explicitly configured follows its own source profile
- legacy `configure(config)` clears previous per-port mappings and configures only port `0`
- `Fw.Com.context` spoofing does not change configured source identity
- forwarded command and dispatcher status preserve port/context semantics
- denied synthetic response uses decoded opcode or `0xFFFFFFFF` for malformed packets
- rejection event includes ingress port, link identity, role, class, reason, and response
- counters continue incrementing while rejection events throttle

`CommandSessionSequence` helper coverage includes:

- first sequence per key accepted
- increasing sequence accepted
- duplicate sequence rejected
- lower sequence rejected
- same sequence under a different ingress port accepted independently
- same sequence under a different session ID accepted independently
- same link/session under a different role accepted as a separate authority epoch
- `resetSession(key)` allows a new baseline
- wraparound without reset rejected

The session/sequence helper is not wired into the active `Fw.Com` command path. It provides only a future enforcement primitive; it does not provide replay protection.

## Hosted Source-Index Probe

Command run after the fresh verification build:

```text
bash scripts/run_command_ingress_authority_probe.sh
```

Result:

```text
command_ingress_authority_probe: PASS
sband-root=/tmp/command-ingress-authority.ZFzPx0/sband-primary
uhf-root=/tmp/command-ingress-authority.ZFzPx0/uhf-backup
sband-authorized=MODE_SET IDLE reached mode state
uhf-allowlisted=MODE_GET dispatched
uhf-denied=MODE_SET IDLE rejected before CmdDispatcher and SYS_MODE remained SAFE
```

### S-Band Primary Profile

Profile: `sband-primary` (`SBAND + PRIMARY`) configured on ingress port `0`.

Representative sequence:

- `OBCApp.modeManager.MODE_GET`
- `OBCApp.modeManager.MODE_SET IDLE`

Evidence excerpts:

```text
2026-05-09T17:26:34.683785: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 18
2026-05-09T17:26:34.683835: CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10030001 completed
2026-05-09T17:26:45.905355: OBCApp.modeManager.SYS_MODE_CHANGE : System mode changed to IDLE
2026-05-09T17:26:45.905449: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030000 dispatched to port 18
2026-05-09T17:26:45.905476: CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10030000 completed
```

Interpretation: the authorized mode command still reaches `CmdDispatcher` and `ModeManager` through the authority gate.

### UHF Backup Configured Profile

Profile: `uhf-backup` (`UHF + BACKUP`) configured on ingress port `0`.

Representative sequence:

- `OBCApp.modeManager.MODE_GET`
- `OBCApp.modeManager.MODE_SET IDLE`

Evidence excerpts:

```text
2026-05-09T17:26:56.867052: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 18
2026-05-09T17:26:56.867118: CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10030001 completed
2026-05-09T17:27:10.112310: OBCApp.commandIngressAuthority.COMMAND_AUTHORITY_REJECTED : Command authority rejected opcode 0x10030000 ingress 0 identity 2 role 2 class 2 reason 1 response VALIDATION_ERROR
```

Probe assertions:

- `MODE_GET` reaches `CmdDispatcher` under `uhf-backup`.
- `MODE_SET IDLE` emits `COMMAND_AUTHORITY_REJECTED` with ingress port `0`, link identity `2` (`UHF`), role `2` (`BACKUP`), class `2` (`MODE_CHANGE`), reason `1` (`POLICY_DENIED`), and response `VALIDATION_ERROR`.
- `MODE_SET IDLE` does not emit `CmdDispatcher` dispatch/completion events in the `uhf-backup` scenario.
- `SYS_MODE_CHANGE` is absent in the `uhf-backup` scenario, so the denied high-authority mode command does not change OBC mode.

## Reused And Updated Verification Paths

Reused baseline:

- default hosted CCSDS S-band routed command path from `ccsds-sband-hosted-adoption-v1`
- hosted command ingress authority profile path from `command-ingress-authority-v1`

Updated by this change:

- hosted command ingress authority profile path now includes the dictionary-visible `COMMAND_AUTHORITY_REJECTED` schema with configured ingress port and link identity fields.

## Deferred Work

- hosted proof for authority ingress port `1`
- simultaneous S-band/UHF routed command ingress proof
- trusted source or per-packet provenance
- physical UHF provenance, RF behavior, target/Pi behavior, or physical serial behavior
- active command session or sequence enforcement
- command envelope carrying session ID or sequence number
- replay protection, authentication, nonce/MAC/signature behavior, or persistent session state
- file packet and unknown packet uplink authority
- full link authority or full uplink authority
