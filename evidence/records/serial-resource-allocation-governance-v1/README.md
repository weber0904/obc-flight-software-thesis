# serial-resource-allocation-governance-v1 Evidence

## Scope

This record captures the governance decision that Raspberry Pi `/dev/serial0` is reserved for the external comm/radio UART path and is not available for GPS live UART bring-up in the current baseline.

## Environment

- Date: `2026-04-10`
- Workspace: `$REPO_ROOT`
- Branch: `feature/libcsp-internal-network-base`
- Host / target mode: documentation and governance update

## Implemented Artifacts

- Main specs updated:
  - `comm-subsystem`: `/dev/serial0` ownership belongs to external comm/radio.
  - `gps-subsystem`: live UART requires a non-conflicting serial allocation.
  - `verification-evidence`: constrained evidence must name serial ownership conflicts.
  - `verification-path-registry`: comm UART evidence cannot be reused as GPS live UART evidence.
- README, scripts README, verification matrix, path registry, narrative docs, and reporting package updated.

## Decision

- Current allocation:
  - `/dev/serial0`: external comm/radio path.
  - GPS live UART: not allocated in the current baseline.
- Preferred future GPS live path:
  - USB-UART with stable `/dev/serial/by-id/...` naming, or
  - separately governed secondary UART overlay / pin mux plan.
- Rejected for the current baseline:
  - sharing or time-multiplexing GPS and comm on `/dev/serial0`.

## Verification Commands

| Step | Command | Result |
|---|---|---|
| repo consistency | `python3 scripts/check_repo_consistency.py` | PASS; 17 main specs and 43 archived changes checked |
| verification inventory | `python3 scripts/report_verification_inventory.py --json` | PASS |
| OpenSpec change validation | `openspec validate serial-resource-allocation-governance-v1` | PASS |
| OpenSpec specs validation | `openspec validate --specs` | PASS; 17/17 specs valid |
| shared gate reuse | `bash scripts/run_verification_ci.sh build-artifacts/libcsp-base-mainline-release-v1-final` | PASS; governance docs included in the same release-readiness gate |

## Verification Summary

- Verdict: PASS for serial resource governance.
- This change intentionally does not implement GPS live UART.

## Remaining Gaps

- `Blocked-HW`: GPS live UART remains blocked until a non-conflicting serial allocation is approved and implemented.
