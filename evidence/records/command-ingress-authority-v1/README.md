# command-ingress-authority-v1 Evidence

Date: 2026-05-09.

Branch: `feature/link-authority-vocabulary-v1`.

Base commit: `a12429f954878a908851cf48e85e748b95715d5e` (`feat(mode): add payload and TTC entry guard`).

Final commit SHA: recorded in the PR/closeout report. This evidence file is committed as part of the final change, so embedding the final commit SHA in this file would make the commit self-referential and unstable.

OpenSpec changes:

- `link-authority-vocabulary-v1`
- `command-ingress-authority-v1`

## Scope

This record covers OBC-side command ingress authority for routed `Fw.Com` command packets before `Svc::CommandDispatcher`.

It does not claim full link authority, full uplink authority, physical UHF serial provenance, file packet uplink authority, unknown packet authority, CSP authority, crypto authentication, session/sequence/replay protection, dynamic UHF primary failover, persistent authority config, reliable transfer, target hardware, RF behavior, or physical USB/RS485 validation.

The hosted profile proof uses the existing default hosted CCSDS S-band routed command path and changes the OBC authority profile between `sband-primary` and `uhf-backup`. The `uhf-backup` profile is configured OBC policy input; it is not inferred from `Fw.Com.context` or from gateway metadata.

## Build And Unit Verification

Commands run:

```text
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build -j 8
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native-ut --target OBC_Components_CommandIngressAuthority_ut_exe -j 8
PATH="$PWD/fprime-venv/bin:$PATH" ctest --test-dir build-fprime-automatic-native-ut -R command_authority_catalog_check --output-on-failure
build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-command-ingress-authority-v1-rerun
```

Result summary:

- fresh default build: PASS
- command authority catalog drift/classification check: PASS
- `OBC_Components_CommandIngressAuthority_ut_exe`: PASS, 14 tests
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

Component and policy coverage include allowed forwarding exactly once, denied no-forward with synthetic status exactly once, status/context preservation, restricted malformed/unknown fail-closed behavior, invalid config deny-by-default behavior, throttled rejection event behavior, unthrottled reject counters, UHF backup allowlist/rejection behavior, dev/internal full-authority profile behavior, and future `UHF + PRIMARY_AFTER_FAILOVER` vocabulary-only full-role behavior.

## Hosted Profile Probe

Command run:

```text
bash scripts/run_command_ingress_authority_probe.sh
```

Result:

```text
command_ingress_authority_probe: PASS
sband-root=/tmp/command-ingress-authority.qUMImX/sband-primary
uhf-root=/tmp/command-ingress-authority.qUMImX/uhf-backup
sband-authorized=MODE_SET IDLE reached mode state
uhf-allowlisted=MODE_GET dispatched
uhf-denied=MODE_SET IDLE rejected before CmdDispatcher and SYS_MODE remained SAFE
```

### S-Band Primary Profile

Profile: `sband-primary` (`SBAND + PRIMARY`).

Representative sequence:

- `OBCApp.modeManager.MODE_GET`
- `OBCApp.modeManager.MODE_SET IDLE`

Evidence excerpts:

```text
2026-05-09T15:24:29.379911: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 18
2026-05-09T15:24:29.379994: CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10030001 completed
2026-05-09T15:24:40.593391: OBCApp.modeManager.SYS_MODE_CHANGE : System mode changed to IDLE
2026-05-09T15:24:40.593495: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030000 dispatched to port 18
2026-05-09T15:24:40.593516: CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10030000 completed
```

Interpretation: an authorized high-authority mode command still reaches `CmdDispatcher` and the `ModeManager` handler through the new gate.

### UHF Backup Configured Profile

Profile: `uhf-backup` (`UHF + BACKUP` configured authority role).

Representative sequence:

- `OBCApp.modeManager.MODE_GET`
- `OBCApp.modeManager.MODE_SET IDLE`

Evidence excerpts:

```text
2026-05-09T15:24:51.378356: CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10030001 dispatched to port 18
2026-05-09T15:24:51.378422: CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10030001 completed
2026-05-09T15:25:02.956263: OBCApp.commandIngressAuthority.COMMAND_AUTHORITY_REJECTED : Command authority rejected opcode 0x10030000 role 2 class 2 reason 1 response VALIDATION_ERROR
```

Probe assertions:

- `MODE_GET` reaches `CmdDispatcher` under `uhf-backup`.
- `MODE_SET IDLE` emits `COMMAND_AUTHORITY_REJECTED`.
- `MODE_SET IDLE` does not emit `CmdDispatcher` dispatch/completion events in the `uhf-backup` scenario.
- `SYS_MODE_CHANGE` is absent in the `uhf-backup` scenario, so the denied high-authority mode command does not change OBC mode.

## Reused And New Verification Paths

Reused baseline:

- default hosted CCSDS S-band routed command path from `ccsds-sband-hosted-adoption-v1`

New path registered by this change:

- hosted command ingress authority profile path: default hosted CCSDS S-band routed `Fw.Com` command path with explicit `CommandIngressAuthority` profiles (`sband-primary` and `uhf-backup`)

## Deferred Work

- multi-link simultaneous provenance
- dynamic `UHF + PRIMARY_AFTER_FAILOVER` runtime authority
- persistent config store and reboot reload from stored authority config
- file packet and unknown packet uplink authority
- crypto authentication
- command session, sequence window, and replay protection
- full link authority enforcement
- physical UHF serial/node-6 authority enforcement proof
