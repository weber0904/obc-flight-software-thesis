## Context

The repository already has governed proofs for adjacent communication paths,
but they are fragmented:

- hosted direct `OBC -> GDS` and bounded hosted `fprime-cli -> GDS` command
  surfaces
- hosted node-`5` and hosted node-`6` CCSDS proofs
- target/lab node-`5` and bounded quiet node-`6` CAN plus serial proofs
- historical target TCP or split-host subsystem proofs that are still useful
  as development-carrier ingredients

This change does not collapse those distinctions. Instead, it adds a new suite
that assembles them into a single matrix while preserving truthful carrier
boundaries:

- hosted UHF remains a governed serial stand-in path
- target TCP uses TCP-based southbound behavior as a development-carrier proof
  for the same capability family
- target CAN keeps physical UHF UART southbound provenance and quiet-path
  acceptance boundaries

## Design

### Subtree Layout

The new suite lives under `scripts/comm_verification/` with four layers:

- `lib/`
  - shared shell helpers for artifact layout, case execution, blocker
    classification, and summary generation
- `env/`
  - environment definitions for `hosted`, `rpi_tcp`, and `rpi_can`
- `cases/`
  - one wrapper per formal communication capability
- `matrix/`
  - environment matrix runners, all-environment runner, and cleanup smoke

The public operator surface remains thin root wrappers in `scripts/` so the
new logic stays in one governed subtree.

### Case Model

The suite uses nine formal capability cases:

1. `direct-control`
2. `sband-command`
3. `sband-file`
4. `sband-sequence-subsystem`
5. `uhf-primary-command`
6. `uhf-primary-file`
7. `uhf-primary-sequence-subsystem`
8. `failover-command`
9. `csp-reachability`

Each case writes its own artifact directory, metadata file, command log, and
captured stdout or stderr log. A case wrapper may:

- reuse an existing governed probe directly
- narrow an existing governed probe into one matrix capability
- or report an explicit bounded blocker when the repo does not yet have a
  trustworthy probe for that environment plus path combination

### Environment Truth

#### Hosted

Hosted uses the current `TopCcsds` baseline and keeps UHF on the governed
serial stand-in or PTY-style southbound path. Its matrix can directly reuse
existing hosted node-`5`, node-`6`, and official sequencing or file proofs
where available.

#### Target TCP

Target TCP is a development-carrier proof surface. It keeps the three-host or
split-host target topology, but accepts TCP-based southbound behavior in place
of physical UHF UART provenance. The suite records that distinction in case
metadata and summaries so the resulting evidence cannot be confused with the
target/lab physical UHF baseline.

#### Target CAN

Target CAN uses the current lab baseline:

- node `5` as default S-band
- node `6` as bounded quiet UHF
- shared SocketCAN interior
- physical UHF UART southbound for node `6`

Target CAN UHF-related cases keep quiet-aware acceptance. This suite does not
claim non-quiet background telemetry stability.

### Matrix Execution Contract

Each environment matrix:

- runs cases sequentially, never in parallel
- writes `summary.json` and `summary.md`
- continues after a failed case
- records a blocker class when a case fails
- exits nonzero if any case failed or stayed blocked

Case wrappers are responsible for writing truthful metadata about:

- `carrier_kind`
- `registered_path_reuse`
- `new_claim_attempted`
- `blocker_class`
- `rerun_safe`

### Cleanup And Rerun Safety

The suite reuses the repository cleanup contract instead of reinventing local
process handling. New wrappers call existing governed probes as subprocesses
with isolated per-case artifact roots. A dedicated cleanup smoke script
interrupts representative matrix wrappers and verifies immediate rerun
behavior.

### Boundaries

- This change adds the suite and evidence scaffolding, not final passing
  revalidation results for every environment-case combination.
- The verification-path registry stays unchanged until a specific rerun or new
  proof has actual passing evidence.
- Unsupported combinations are surfaced as explicit blockers rather than hidden
  behind ad hoc operator knowledge.

### Follow-On Change Train

The umbrella matrix remains the stable operator-facing surface, but the
remaining blocked cells are intentionally split into smaller follow-on changes:

1. `comm-verification-matrix-foundation-v1`
2. `comm-verification-sequence-subsystem-harness-v1`
3. `target-can-node6-matrix-closure-v1`
4. `target-tcp-southbound-parity-foundation-v1`
5. `target-tcp-matrix-closure-v1`
6. `target-direct-control-matrix-cases-v1`

Those changes may add or refine governed probes, launchers, and evidence in
their own branches and commits, but they do not replace this matrix suite as
the canonical entrypoint family. Their passing evidence rolls back up into this
umbrella record and only then expands registry claims.

## Risks And Boundaries

- The biggest risk is false equivalence between target TCP development-carrier
  proofs and target CAN plus physical-UHF provenance. The suite mitigates that
  by recording carrier kind per case and by keeping the runbook language
  explicit.
- Some matrix cells currently rely on fragmented historical probes or lack a
  dedicated formal harness. The suite records these as blockers so future work
  can land targeted probes without changing the matrix contract.
- The suite is intentionally orchestration-heavy. It does not replace the
  underlying governed probes or widen their formal claim scope on its own.
