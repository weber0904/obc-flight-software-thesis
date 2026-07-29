## Context

Three truths constrain this change.

1. The thesis Chapter 5 routes must now be proven against the active
   `TopCcsds` architecture and the current validation-path registry, not
   against legacy probe families.
2. Current product behavior already covers most route semantics:
   `ModeSafetyController` owns SoC fallback/admission, `TtcPassManager` owns
   pass-window entry/exit, `AdcsBridge` already owns the runtime ADCS mode
   surface, and `RecoveryExecutor` already defines the active EPS timeout and
   reboot behavior.
3. The missing pieces are narrow but cross-cutting: runtime EPS SoC
   stimulation, TTC-to-ADCS glue, and repo-owned Chapter 5 proof surfaces.

The design therefore avoids a broad autonomy redesign and instead adds small
interfaces and new proof ownership around the maintained baseline.

## Goals / Non-Goals

**Goals**

- Add a simple external control surface to the EPS simulator so hosted and
  target probes can change SoC while the stack is running.
- Make TTC auto-entry best-effort command ADCS `POINTING` through the current
  internal runtime path.
- Establish one new Chapter 5 probe tree that can host staged scripts or
  command-playbook fallbacks without reviving obsolete probe families.
- Preserve the current target recovery and COMM semantics instead of inventing
  new product meaning during probe redesign.

**Non-Goals**

- No new OBC EPS command for SoC injection.
- No quaternion-tracking law, target-vector management, or TTC-exit ADCS
  restore in this slice.
- No requirement that every route be one monolithic script.
- No attempt to make Mission Console packet-lab negative evidence a Chapter 5
  dependency.
- No new claim that S-band loss automatically fails over to UHF as a new
  product semantic.

## Decisions

### 1. EPS runtime SoC control stays simulator-owned and external

The EPS simulator will accept an optional local control socket path. External
helpers will send only one narrow command family:

- `set-soc --value <pct> [--transition-sec <sec>]`

The simulator stores the target and optional transition window and resolves the
current SoC lazily from monotonic time when later requests or readbacks occur.
This keeps the control surface simple, keeps OBC semantics unchanged, and
avoids introducing a second simulator tick loop only for Chapter 5 proof.

### 2. TTC auto-entry uses a small ADCS runtime control dependency

`TtcPassManager` will gain one additional runtime dependency dedicated to ADCS
entry control. After a successful TTC entry request, it will best-effort send
one internal ADCS mode request to `POINTING`.

The hook is intentionally one-way and non-blocking:

- TTC entry still succeeds even if the ADCS call fails.
- Only the first successful entry edge sends the request.
- Exit or safety fallback does not restore the prior ADCS mode in this slice.

`AdcsBridge` will implement the new interface by reusing the existing runtime
mode-set path rather than adding a second ADCS command surface.

### 3. Chapter 5 proofs get a new owned script tree

All new scripts live under `scripts/chapter5_routes/`:

- `lib/` for baseline attach, auth, readback, artifact capture, summary, and
  cleanup helpers
- `hosted/` for hosted route wrappers or staged sub-probes
- `target/` for target A/B/C-owned route wrappers or staged sub-probes

The tree is allowed to mix:

- full probes
- staged probes
- repo-owned step-by-step command playbooks

A route may be considered formally closed by multiple scripts under the same
change as long as the evidence names the exact path, observability surface,
artifacts, and verdict aggregation.

### 4. Mission Console packet-lab remains out-of-scope for Chapter 5

The already observed Mission Console packet-lab negative evidence does not
break attach, auth, subsystem readback, or current recovery/orchestration
baselines. It stays a bounded Mission Console oracle issue rather than a
Chapter 5 blocker. The new Chapter 5 probes may reuse the maintained baseline
ownership pattern established by `mission-console-phase1`, but they must not
depend on that packet-lab verdict surface.

### 5. Route semantics stay aligned to current repo truth

This change adopts the repo’s current behavior instead of the thesis wording
where they differ:

- EPS timeout stays `R3_RESET_SUBSYSTEM_INTERFACE` plus `SAFE` fallback, not an
  EPS software reboot surface.
- Route 2 target closure proves TTC entry, ADCS `POINTING` readback, bounded
  node-5 loss / UHF continuity / re-auth / HK `.fdp`, but does not upgrade
  that into a new automatic failover product claim.
- Route 3 hosted proof stops short of hosted `R6`; Route 3 target proof keeps
  the current quiet watchdog reboot boundary.

## Verification Strategy

- OpenSpec deltas define the new product and evidence boundaries.
- Focused unit coverage extends `TtcPassManager` for the TTC-to-ADCS hook.
- EPS simulator tests cover direct set, timed ramp, invalid control input, and
  reconnect behavior.
- Initial implementation round provides partial local closure:
  - new product/runtime capability
  - new helper/control plumbing
  - new Chapter 5 script tree and playbook/probe scaffolding
- Full Route 1/2/3 closure will be accepted through staged hosted/target
  reruns governed by the new Chapter 5 evidence layout.

## Risks / Trade-offs

- A socket-based EPS control surface adds simulator complexity. The narrow
  command vocabulary and lazy-ramp model keep that complexity bounded.
- TTC auto-entry can now touch ADCS. Making the hook best-effort prevents TTC
  policy from taking ownership of ADCS health or entry success.
- Staged route proofs may be less elegant than one big script, but they are
  easier to keep deterministic and to audit against the exact observability
  surfaces the repo already owns.
