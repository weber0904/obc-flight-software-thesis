# dual-pi-hostname-and-target-role-alignment-v1

## Summary

This evidence record covers a governance/configuration slice that aligns the repository with the near-term two-Pi topology:

- `macOS` as the ground host
- `obc` / `obc.local` as the OBC target host
- `subsystem-sim` / `subsystem.local` as the subsystem simulator host

No runtime business logic, F' public contract, CSP service contract, or topology business behavior changed in this slice. The evidence therefore focuses on shell/config resolution, documentation alignment, and OpenSpec/repo governance checks.

## Scope

Covered:

- canonical `OBC_SSH_TARGET` resolution for current OBC-target scripts
- backward-compatible `RPI_SSH_TARGET` alias handling
- formal `SUBSYSTEM_SIM_SSH_TARGET` shared configuration surface for future subsystem-host workflows
- checked-in wording updates for near-term host-role naming

Not covered:

- dual-Pi remote simulator orchestration over SSH
- new runtime probes
- real subsystem hardware integration
- physical carrier migration (`CAN` / `UART` / `RS485`)

## Validation

### 1. Shell syntax

Validated all modified shell scripts with `bash -n`:

- `scripts/_common.sh`
- `scripts/sync_rpi_workspace.sh`
- `scripts/bootstrap_rpi_workspace.sh`
- `scripts/package_rpi_bundle.sh`
- `scripts/install_rpi_bundle.sh`
- `scripts/run_rpi_stack.sh`
- `scripts/run_rpi_uart_stack.sh`
- `scripts/run_rpi_remote_csp_stack.sh`
- `scripts/run_rpi_installed_stack.sh`
- `scripts/install_rpi_autostart.sh`
- `scripts/rpi_autostart_status.sh`
- `scripts/run_rpi_autostart_probe.sh`
- `scripts/run_rpi_boot_probe.sh`
- `scripts/run_rpi_installed_probe.sh`
- `scripts/run_rpi_remote_csp_gds_probe.sh`
- `scripts/run_rpi_csp_comm_baseline_probe.sh`
- `scripts/run_rpi_endurosat_transparent_probe.sh`
- `scripts/run_rpi_endurosat_framed_probe.sh`
- `scripts/run_rpi_endurosat_framed_robustness_probe.sh`
- `scripts/run_rpi_uart_probe.sh`

Result: PASS

### 2. Shared helper resolution

Command:

```bash
bash -lc 'source scripts/_common.sh; unset OBC_SSH_TARGET RPI_SSH_TARGET SUBSYSTEM_SIM_SSH_TARGET; printf "default_obc=%s\n" "$(obc_resolve_obc_ssh_target)"; printf "default_subsystem=%s\n" "$(obc_resolve_subsystem_sim_ssh_target)"; RPI_SSH_TARGET=legacy@compat.local; printf "alias_obc=%s\n" "$(obc_resolve_obc_ssh_target)"; OBC_SSH_TARGET=explicit@override.local; printf "explicit_obc=%s\n" "$(obc_resolve_obc_ssh_target)"; SUBSYSTEM_SIM_SSH_TARGET=explicit@subsystem.local; printf "explicit_subsystem=%s\n" "$(obc_resolve_subsystem_sim_ssh_target)"'
```

Observed output:

```text
default_obc=operator@obc.local
default_subsystem=operator@subsystem.local
alias_obc=legacy@compat.local
explicit_obc=explicit@override.local
explicit_subsystem=explicit@subsystem.local
```

Interpretation:

- default fallback resolves to `operator@obc.local`
- legacy `RPI_SSH_TARGET` still overrides the default for OBC-target scripts
- explicit `OBC_SSH_TARGET` takes precedence over the legacy alias
- `SUBSYSTEM_SIM_SSH_TARGET` is available as a formal future-facing configuration surface

### 3. Repository governance checks

Command:

```bash
python3 scripts/check_repo_consistency.py
```

Observed output:

```text
PASS: repo consistency checks passed
- main specs checked: 18
- archived changes checked: 49
- matrix source: docs/baseline-reconciliation-matrix.json
```

Command:

```bash
python3 scripts/check_agent_entrypoint.py
```

Observed output:

```text
PASS: agent onboarding entrypoint checks passed
- checked file: AGENTS.md
- required references: 7
```

### 4. OpenSpec validation

Command:

```bash
openspec validate dual-pi-hostname-and-target-role-alignment-v1
```

Observed output:

```text
Change 'dual-pi-hostname-and-target-role-alignment-v1' is valid
```

Command:

```bash
openspec validate --specs
```

Observed output:

```text
- Validating...
✓ spec/adcs-subsystem
✓ spec/architecture-review
✓ spec/boot-update
✓ spec/codex-skills
✓ spec/comm-subsystem
✓ spec/core-system-contracts
✓ spec/delivery-workflow
✓ spec/eps-subsystem
✓ spec/gps-subsystem
✓ spec/housekeeping-archive
✓ spec/mission-autonomy
✓ spec/platform-baseline
✓ spec/project-reporting
✓ spec/resource-storage
✓ spec/scenario-driven-validation
✓ spec/storage-health
✓ spec/verification-evidence
✓ spec/verification-path-registry
Totals: 18 passed, 0 failed (18 items)
```

## Conclusion

This slice proves that the repository has moved from one implicit Raspberry Pi target hostname to an explicit role-based host configuration surface while preserving backward compatibility for current OBC-target workflows.

It does not prove a new runtime path. It prepares the repository for later dual-Pi workflow changes without changing the current runtime or verification-path boundaries.
