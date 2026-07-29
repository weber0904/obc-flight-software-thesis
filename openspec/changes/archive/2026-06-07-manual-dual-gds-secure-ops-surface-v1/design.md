## Context

The current maintained repo already has most of the building blocks needed for
manual operations:

- hosted per-band stock stacks on one shared `TopCcsds` runtime
- target/lab shared baseline managers for OBC, subsystem, and CAN services
- single-surface target ground launcher for stock `fprime-gds` plus
  `ground_ttc_gateway`
- secure handshake and secure-v2 packet helpers
- governed sequence/file-uplink proof code that already knows how to encode
  commands and F' file packets

What is missing is a maintained operator layer that composes those pieces into
stable human-facing entrypoints without inheriting proof-owned defaults such as
passive event/channel listeners, probe-specific cleanup, or embedded target
baseline repair logic.

## Design

### Subtree ownership

All new operator entrypoints live under `scripts/manual_ops/`:

- `hosted/`: hosted dual-GDS operator lifecycle
- `target/`: target baseline and target local ground lifecycle
- `lib/`: shared manifest, status, auth, and command/file helpers
- `manual_secure_ops.py`: the operator CLI

No new root-level `scripts/run_*` wrapper is added for this family. The repo
entrypoints only route readers into this subtree.

### Surface split

The maintained manual family has three separate surfaces:

1. hosted manual dual-GDS surface
2. target manual baseline surface
3. target manual local ground dual-GDS surface

The split is intentional:

- hosted owns the full local shared runtime plus both GDS/gateway surfaces
- target baseline owns only remote shared target readiness
- target local ground owns only local GDS/gateway helpers

This keeps the current A/B/C target governance intact and avoids one wrapper
implicitly owning both shared remote services and local operator listeners.

### Manifest/status contract

Every manual surface writes a manifest JSON under its owned surface root. The
helper reads only that manifest and a colocated auth/session state file.

Required contract fields:

- `surfaceRoot`
- `ownerPid`
- `lifecycleState`
- `surfaceType`
- `gdsUiMode`
- `operatorSurfaces.<band>.gdsPort`
- `operatorSurfaces.<band>.gdsTtsPort`
- `operatorSurfaces.<band>.fileStorageDir`
- `operatorSurfaces.<band>.southbound`
- `operatorSurfaces.<band>.logs`
- `nonClaims`

Hosted manifests also carry:

- `sharedHostedRuntime`
- hosted internal-only listener and log metadata

Target baseline manifests also carry:

- remote service snapshot
- OBC/subsystem SSH targets
- readiness scope and explicit non-ownership of local GDS

Target ground manifests also carry:

- local dual-GDS ownership
- explicit dependency on an already-ready remote target baseline

### GDS UI mode

Manual surfaces accept `GDS_UI_MODE=ui|headless`.

- `ui` is the operator default
- `headless` is the validation/default automation mode

Existing maintained launchers keep their current headless behavior unless the
new mode is explicitly requested. The manual surface and its helper must see
the same manifest/status shape in both modes.

### Secure helper

`manual_secure_ops.py` is the only maintained manual secure-command surface.
It does not proxy or intercept stock GDS UI commands.

Subcommands:

- `auth establish`
- `auth clear`
- `status`
- `command send`
- `file upload`
- `seq validate`
- `seq run`
- `seq prepare-manual`
- `seq start`
- `seq step`
- `seq cancel`

Required selectors:

- `--env hosted|target`
- `--band sband|uhf-backup|uhf-primary-after-failover`
- `--manifest <path>`

### Session derivation and state

The helper reuses the tracked keystore and secure handshake implementation to
derive the session key locally:

- send `ReqAuth`
- wait for challenge in the gateway capture
- derive the session key from the tracked keystore
- send `Response`
- persist the accepted session metadata locally

Stored state fields:

- `serviceId`
- `activeBand`
- `nextSecureSequence`
- `lastAuthTime`
- `manifestPath`
- `authorityMode`

State invalidation is explicit and conservative:

- `auth clear`
- surface restart
- timeout
- explicit band switch between S-band and UHF operator roles

### Governed operator scope

The manual helper exposes only the current governed path:

- secure-v2 command send
- `.sequence-staging/<leaf>` upload only
- `SequenceAdmissionController` wrapper commands only

It does not expose:

- arbitrary file destinations
- raw `SeqDispatcher.RUN`
- raw `CmdSequencer` operator controls
- legacy `SESSION_OPEN` as the default operator flow

### Target provenance gate

Target `auth establish` runs a lightweight provenance check before deriving a
session:

- repo keystore SHA must match the installed release keystore SHA
- installed release manifest SHA must match that bundled keystore record
- active OBC service must still point at the current governed release root

The gate is an operator safety check, not a new large proof workflow.

## Risks And Mitigations

- Risk: manual wrappers accidentally become new policy owners.
  Mitigation: keep proof wrappers unchanged, keep target baseline and local
  ground ownership separate, and document non-claims in the manifest.

- Risk: helper reintroduces legacy session semantics.
  Mitigation: implement only tracked secure-auth plus secure-v2 paths in the
  maintained operator CLI.

- Risk: target local cleanup stops shared remote services.
  Mitigation: target local ground wrappers only reap local GDS/gateway/helper
  processes and never call remote stop/restart during ordinary cleanup.

- Risk: UI support breaks existing headless proofs.
  Mitigation: keep current headless behavior as the launcher default and add
  opt-in `ui` mode instead of replacing current invocation shapes.
