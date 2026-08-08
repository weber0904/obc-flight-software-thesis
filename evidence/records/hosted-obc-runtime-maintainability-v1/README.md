# hosted-obc-runtime-maintainability-v1 Evidence

## Scope

This record covers a refactor-only hosted OBC runtime maintainability change.

The implementation moved duplicated default `OBC` and `OBC_CcsdsGroundLinkSpike` launch parsing, status/help formatting, command dispatch, UART framing helpers, and interactive/headless loop behavior into the shared `OBC_Runtime` helper module. The two executable mains now act as topology adapters over the existing `OBCApp` and `OBCAppCcsds` services.

This record does not register a new validation path. It reuses the existing path registry entries for:

- mode-model hosted behavior: `evidence/verification-path-registry.md` entry 35, governed by `evidence/records/mode-model-v2-v1/README.md`
- hosted S-band TCP TT&C and housekeeping archive file/downlink: entries 37 and 38, governed by `evidence/records/sband-tcp-ground-link-v1/README.md`
- hosted UHF serial backup TT&C and BeaconV1 side channel: entries 39 and 40, governed by `evidence/records/uhf-uart-backup-link-v1/README.md`
- hosted CCSDS S-band spike command/event/telemetry and file/downlink: entry 41, governed by `evidence/records/ccsds-ground-link-spike-v1/README.md`

## Non-Claims

This refactor does not claim:

- default `OBC` migration from `ComFprime` to CCSDS
- new CCSDS compatibility beyond the existing spike executable evidence
- HK data product field changes
- command authority, sessions, authentication, failover, RF, reliable transfer, target/Pi deployment, or hardware UART/RS485 behavior
- any new direct `GDS -> TCP -> OBC`, S-band-through-COMM, UHF serial backup, or CCSDS evidence boundary

## Verification Summary

| Check | Command | Result |
| --- | --- | --- |
| native generate | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f` | PASS |
| native build | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build` | PASS |
| UT generate | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate --ut -f` | PASS |
| UT build | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut` | PASS |
| focused helper test | `build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test` | PASS, `hosted_runtime_unit_test: PASS` |
| full local gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-hosted-obc-runtime-maintainability-v1-reviewfix` | PASS through `09_openspec_validate_specs` after review follow-up |
| mode-model hosted probe | `bash scripts/run_mode_model_v2_hosted_probe.sh` | PASS, log `/tmp/mode-model-v2-hosted.zmvoPM/obc-mode-smoke.log` |
| S-band TCP ground-link probe | `bash scripts/run_sband_tcp_ground_link_probe.sh` | PASS, logs `/tmp/obc-sband-tcp-ground-link.dGm2Re` |
| UHF UART backup-link probe | `bash scripts/run_uhf_uart_backup_link_probe.sh` | PASS, logs `/tmp/obc-uhf-uart-backup.136TUi` |
| CCSDS ground-link spike probe | `bash scripts/run_ccsds_ground_link_spike_probe.sh` | PASS, logs `/tmp/obc-ccsds-ground-link-spike.ThQzCW` |
| active change validation | `openspec validate hosted-obc-runtime-maintainability-v1` | PASS |
| full spec validation | `openspec validate --specs` | PASS, 23 specs passed |

## Focused Helper Coverage

`hosted_runtime_unit_test` covers the shared runtime helper without binding to either concrete topology:

- command table routes representative `mode`, `csp`, `eps`, `adcs`, `gps`, `storage`, `comm`, `radio`, `uart`, and `boot` commands to fake service callbacks
- `quit` and `exit` stop the loop
- unknown top-level commands print `unknown command`
- invalid mode, ADCS mode, GPS source, bool, and hex payload inputs preserve visible error wording
- help output includes the current operator command list
- launch parsing accepts existing valid arguments and rejects malformed numeric/runtime configuration values deterministically

## Behavior-Preservation Markers

The focused hosted probes confirmed the refactor preserved visible startup/status markers used by existing evidence:

- default runtime: `OBC runtime started. Type 'help' for commands.`
- shared S-band/COMM status markers: `Ground link via COMM CSP node: 5`, `groundLink mode=comm-csp commNode=5`
- UHF backup markers: `UHF beacon CSP sink: node=6 port=33`, `Ground link via COMM CSP node: 6`
- CCSDS spike runtime: `OBC_CcsdsGroundLinkSpike`, `OBCAppCcsds`, `framing=space-packet-space-data-link`, `scid=0x44`, `vcid=1`, `frame-size=1024`

The probe verdicts reused existing evidence boundaries and demonstrate that the runtime refactor did not alter command names, operator-visible status output, F' command/event/channel flow, telemetry/file-downlink paths, or spike/default topology separation.
