## Context

Hosted COMM identity already uses S-band node `5` and UHF node `6`, while target/lab still routes through generic `comm_csp_node` node `4` on `subsystem.local:can1`. Target reboot/recovery proofs and operator runbooks therefore depend on a compatibility path rather than the intended link identities.

Target serial/background-TM instability remains unresolved. Quiet-mode target UHF proofs are acceptable as bounded diagnostics, but this change must not restate that bounded workaround as general target serial closure.

## Goals / Non-Goals

**Goals:**

- make node `5` the default target/lab COMM path
- make node `6` the target/lab physical-serial UHF path with bounded `uhf-backup` and `uhf-primary-after-switch` proof roles
- remove implicit node-`4` target behavior from install/run/status/probe surfaces
- redefine target/lab COMM detector truth so both `COMM_PRIMARY_UNAVAILABLE` and any reboot-class COMM escalation depend on primary COMM subsystem responsiveness instead of ground-station attachment
- keep existing target reboot-class proofs alive by moving them onto the new default node-`5` path
- prove node-`6` target command/readback in quiet mode for both bounded UHF roles

**Non-Goals:**

- no simultaneous dual-link runtime or gateway multiplexing
- no beacon / standby RX / handshake / retry state machine
- no general serial/background-TM stability fix
- no hosted behavior change

## Decisions

### Use profile-driven target configuration

Target/lab surfaces will expose `TARGET_COMM_PROFILE=sband|uhf-primary|uhf-backup`, but the OBC service bootstrap remains grounded in the default target S-band configuration. The requested target profile selects the bounded proof path and the probe-side authority/session role:

- `sband` uses the default node-`5` target path directly
- `uhf-backup` uses node `6` as a bounded quiet backup-ingress proof while OBC still boots in the default S-band target configuration
- `uhf-primary` uses a bounded quiet node-`6` proof **after explicit switch to UHF primary** from the default node-`5` target path

This avoids drifting combinations where target scripts silently pick node `4`, while also preserving the current runtime truth that UHF-primary authority is a post-switch state rather than a standalone cold-start bootstrap surface.

Alternative considered: keep free-form `COMM_CSP_NODE` plus `COMMAND_AUTHORITY_PROFILE`. Rejected because it preserves the operator ambiguity that created the current node-`4` drift.

### Place target S-band node `5` on `subsystem.local`

`subsystem.local` will host `sband_comm_csp_node` as a TCP listener on the same target-side COMM host that already owns EPS/ADCS/COMM lab services. This keeps target OBC talking to a target-side COMM peer instead of moving the COMM peer into macOS-hosted ground tooling.

Alternative considered: host node `5` on macOS. Rejected because it weakens target/lab boundaries and turns target S-band truth into a host-assisted shortcut.

### Keep one OBC target service entrypoint

`obc-comm-csp-stack.service` remains the single OBC target service entrypoint. Profile selection changes its runtime mapping; it does not create separate OBC services per link identity. This keeps watchdog/recovery ownership and target lifecycle management in one place.

Alternative considered: separate OBC services for S-band and UHF. Rejected because it complicates target recovery/watchdog ownership and does not add proof value for this migration.

### Split target proofs by path role

Node `5` becomes the default target path and therefore owns the existing reboot-class proofs. Node `6` gets separate bounded quiet-mode operational proofs:

- `uhf-backup` as a bounded backup-ingress proof
- `uhf-primary` only **after explicit switch to UHF primary** via the default node-`5` path

Alternative considered: keep reboot-class proofs on node `6`. Rejected because the most important target proofs would then remain attached to a non-default path.

### Redefine target/lab COMM detector input around subsystem responsiveness

For target/lab node `5/6`, `COMM_PRIMARY_UNAVAILABLE` will no longer be driven by `GroundLinkDriver` attachment or `ground_ttc_gateway` presence. Instead, the detector will use repeated **internal CSP ping failure** from OBC to the current primary COMM subsystem stand-in:

- node `5` for S-band
- node `6` for UHF

`GroundLinkHealthProvider` remains reviewable observability for ground-facing carrier state and transport noise, but it is no longer the truth source for whether the primary COMM subsystem itself is available or transport-healthy in target/lab node `5/6` mode.

Alternative considered: gate COMM FDIR by `passActive`. Rejected because pass windows and ground-contact opportunities are not equivalent to subsystem health, and lack of ground contact is not itself a fault.

## Risks / Trade-offs

- **[Risk] Target service/profile migration touches many scripts and systemd templates** → Keep one canonical profile-mapping helper and update all target entrypoints to use it.
- **[Risk] Node `6` proof results could be mistaken for general serial closure** → Record quiet-mode boundaries explicitly in specs, docs, and evidence.
- **[Risk] Node `6` proof results could be mistaken for standalone UHF bootstrap support** → Record `uhf-primary` as a post-switch proof only, and keep cold-start bootstrap explicitly out of scope.
- **[Risk] Node `5` target proof may reveal subsystem-side TCP service orchestration gaps** → Treat subsystem service graph updates as part of this change, not as ad hoc probe setup.
- **[Risk] Historical node `4` references may survive in docs or probes** → Update canonical truth and verification registry together, and keep node `4` only as explicit historical/compatibility language.
- **[Risk] COMM detector redesign affects availability and failover semantics, not just fault submission** → Route target/lab reboot-class COMM detection through subsystem responsiveness only, keep ground-link transport as observability for node-`5/6`, and preserve fallback behavior for non-node-`5/6` paths.

## Migration Plan

1. Add OpenSpec artifacts and codify target node-`5`/node-`6` truth.
2. Introduce target profile mapping in OBC target launch/install/status surfaces.
3. Replace `subsystem-comm-csp.service` baseline with profile-specific node-`5` and node-`6` services/targets.
4. Redesign `CommController` target/lab COMM detector input to use repeated internal CSP ping failure for node `5`/`6` while preserving ground-link observability only.
5. Migrate target operational and reboot-class probes to the new path ownership.
6. Update runbook, architecture truth, and verification registry/evidence.
7. Run fresh local verification, then target node-`5` and node-`6` proofs.

Rollback is branch-local: restore the old target services/templates and rerun the prior node-`4` path if migration proves incomplete before merge.

## Open Questions

- None for this change. Beacon/handshake/retry state machine and dual-link concurrency remain explicitly deferred follow-on work.
