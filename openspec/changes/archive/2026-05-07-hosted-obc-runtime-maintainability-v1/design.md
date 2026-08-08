## Context

The default hosted `OBC` executable and `OBC_CcsdsGroundLinkSpike` both expose the same operator shell surface for hosted diagnosis and probes. Their current `Main*.cpp` files duplicate most launch parsing, runtime state refresh, status/help formatting, command parsing, and command dispatch. The real differences are topology ownership (`OBCApp` versus `OBCAppCcsds`), the imported F' comm subtopology, and a small CCSDS-specific startup banner.

This change is a maintainability refactor. Existing validation paths remain the behavior baseline: direct hosted runtime/operator commands, stock `ComFprime` S-band/UHF paths, and the separate CCSDS spike proof path.

## Goals / Non-Goals

**Goals:**

- Move common hosted runtime parsing and command dispatch into a shared `OBC_Runtime` helper module.
- Keep deployment-specific topology and protocol differences in small adapter code.
- Preserve existing command names, arguments, visible output strings used by probes, F' public symbols, topology defaults, and validation-path boundaries.
- Add focused helper tests for command routing, parser failures, help text, and launch argument validation.

**Non-Goals:**

- No default `OBC` migration from `ComFprime` to `ComCcsds`.
- No new CCSDS, S-band, UHF, direct-GDS, target/Pi, RF, reliable-transfer, command-authority, session, authentication, failover, or pass-scheduler claims.
- No HK data product field changes and no housekeeping archive format changes.
- No real F' component interface changes under `OBC/Components/`.

## Decisions

### Shared runtime module with deployment adapters

Add `OBC/Runtime` as a project-local helper module registered as `OBC_Runtime`. It owns common runtime structs, parser helpers, help/status formatting, command routing, and the interactive/headless loop. Each deployment main provides an adapter that maps the helper's runtime-service interface to the correct topology globals.

Alternative considered: keep two source files and mechanically reduce `if`/`else` blocks in each. Rejected because it would still require every future operator-shell command to be edited twice.

Alternative considered: make the runtime helper a C++ template over a topology namespace. Rejected because it would make focused helper tests harder to keep independent from generated topology globals.

### Table-driven top-level dispatch, handler-based subcommands

Use a small top-level command table for `help`, `quit`/`exit`, `status`, `mode`, `csp`, `eps`, `adcs`, `gps`, `storage`, `comm`, `radio`, `uart`, and `boot`. Keep subcommand handlers explicit because they call distinct component/runtime services and must preserve existing visible error handling.

Alternative considered: one fully data-driven schema for every subcommand and argument. Rejected because the current commands are stateful component operations, and a broad schema would add abstraction without improving this narrow refactor.

### Behavior preservation over cleanup

Keep current command syntax and output fragments stable even where stricter validation might be attractive. Only adopt already-proven stricter numeric launch-argument parsing where it does not change valid invocations and where malformed argument behavior is covered by helper tests.

Alternative considered: normalize all malformed operator command arguments during this refactor. Rejected because that would turn maintainability work into behavior change and expand probe/evidence scope.

## Risks / Trade-offs

- [Risk] Refactoring startup and command handling could break probe-dependent output markers. Mitigation: preserve banner/status strings and run focused hosted probes after a fresh build.
- [Risk] A shared helper could accidentally collapse default `ComFprime` and CCSDS spike boundaries. Mitigation: keep topology/protocol selection adapter-owned and record evidence as reused existing paths only.
- [Risk] Helper tests could overfit fake services rather than runtime behavior. Mitigation: combine focused L1 helper tests with the existing repository-owned hosted probes for behavior preservation.
- [Risk] New shared module dependencies could make build registration fragile. Mitigation: register `OBC_Runtime` as a normal project module and keep deployment dependencies explicit.
