## Why

The current payload baseline now closes canonical payload `.fdp` delivery on
the hosted official path, but it still ties `PAYLOAD_CAPTURE_*` command
completion to deferred capture and `.fdp` publication completion. That keeps
the command surface awkward for operators and for later mission logic because a
capture request is not acknowledged until the whole capture-plus-publication
chain completes or times out. At the same time, the governed target node-`5`
payload `.fdp` path is still unproven and must now be closed on the active
baseline instead of left for another change.

## What Changes

- Change the existing `PAYLOAD_CAPTURE_*` public commands to async-ack
  semantics: once a capture request is accepted and handed to the payload
  runtime, the command returns `OK` immediately.
- Keep actual capture outcome and canonical `.fdp` publication outcome on the
  payload state/event/telemetry/readback surfaces instead of the original
  command response.
- Add explicit payload observability for the post-capture publication phase so
  operators can distinguish `CAPTURING` from `.fdp` publication and final
  success/failure.
- Add a repository-owned governed target node-`5` payload `.fdp` proof that
  exercises target capture, canonical payload `.fdp` publication, stock
  `BUILD_CATALOG` plus `START_XMIT_CATALOG(NO_WAIT)`, byte-match, decode, and
  JPEG hash parity on the current official target S-band path.
- Reconcile specs, registry, active docs, and evidence so the old deferred
  target payload boundary is removed and the new async command model is the
  active contract.

## Capabilities

### Modified Capabilities

- `payload-operations`: capture commands become async-ack request surfaces, and
  payload state/readback becomes the formal completion oracle
- `payload-data-products`: canonical payload `.fdp` publication remains
  mandatory for final payload success, but no longer blocks capture command
  acknowledgement
- `comm-subsystem`: target node-`5` payload `.fdp` proof is added on the stock
  official path without widening reliable-transfer scope
- `verification-path-registry`: add the distinct target node-`5` payload `.fdp`
  proof path and update hosted/target path selection wording

## Impact

- Affected code: `PayloadOpsController`, payload state enum/event/tlm wiring,
  payload unit tests, hosted/target payload proof scripts, and target payload
  decode/downlink tooling
- Affected systems: payload operator polling model, official sequence usage
  assumptions around payload capture, target node-`5` payload `.fdp` evidence,
  and verification-path registry selection rules
- Public behavior: `PAYLOAD_CAPTURE_*` returns success on accepted dispatch
  instead of final capture completion; final completion or failure moves to
  payload state/event/tlm/readback surfaces
