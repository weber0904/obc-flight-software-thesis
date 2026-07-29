## Context

The current payload baseline closes camera operation, backend isolation, and `.jpg + .json` capture reviewability, but it explicitly stops short of payload-specific stored-history and downlink closure. The active official delivery path is already `DpManager/DpWriter -> DpCatalog -> CommController -> FileDownlink`, and the repo has already hardened secure command, secure auth, and bounded uplink authority enough that payload delivery is now the main mission-side closure gap. The implementation must keep `PayloadOpsController` as the only public payload owner, avoid introducing generic file governance, and remain compatible with hosted proof plus a bounded target node-`5` S-band proof.

## Goals / Non-Goals

**Goals:**
- Promote each successful payload capture into one canonical payload `.fdp` that can enter the active stored/downlink baseline.
- Preserve local `.jpg + .json` artifacts for diagnostics while making the `.fdp` the only formal payload delivery artifact.
- Keep success semantics strict: capture is successful only when local capture and canonical `.fdp` publication both succeed.
- Make the hosted proof decode the received `.fdp`, extract JPEG bytes, and compare them against the source JPEG, while keeping target official payload `.fdp` proof as an explicit follow-on unless the same oracle is available through one repo-owned target COMM wrapper.
- Keep docs, registry, and operator flow aligned with current active baseline truth.

**Non-Goals:**
- No new payload manager, payload scheduler, or multi-payload framework.
- No payload-specific file browse/list/select/download operator surface.
- No broadening of the current reliable-transfer helper beyond the existing HK `.fdp` family.
- No raw-register target closure, physical switched-rail claim, RF closure, or broader security redesign.
- No multi-container chunking or arbitrary large-image transport redesign in this change.

## Decisions

### Keep `DpCatalog` as the only official downlink owner
The change reuses the existing official `.fdp` delivery chain instead of creating a second payload file owner. This preserves the active baseline’s ownership model, keeps command authority unchanged, and avoids reopening arbitrary-file governance. Alternatives considered:
- Generic file owner for payload artifacts: rejected because it widens repo scope into arbitrary-file governance.
- Payload-specific direct `FileDownlink` command: rejected because it bypasses the current official `DpCatalog` ownership boundary.

### Make payload `.fdp` canonical and keep `.jpg + .json` diagnostic-only
`PayloadOpsController` will continue to write the local JPEG and sidecar metadata under `persistent-data/payload/camera/`, then immediately read the JPEG bytes back and publish a canonical data product. The `.fdp` becomes the official stored/downlink artifact; the local files remain for local inspection and failure diagnosis. This avoids splitting formal ownership across two artifact families.

### Publish one payload product containing metadata and variable-size JPEG bytes
The new `payload-data-products` capability will define one fixed metadata record (`PayloadCaptureHeaderV1`) plus one variable-size `U8 array` JPEG record inside the same payload container. This keeps the product self-describing and decodable through standard F' data-product tooling while avoiding a second companion artifact. Alternatives considered:
- Metadata-only `.fdp`: rejected because it leaves the mission payload bytes outside the formal delivery path.
- Dual canonical family: rejected because it keeps ownership split and makes proof/oracle logic heavier.

### Gate command success on canonical publication
If local capture succeeds but payload `.fdp` publication fails, the command returns failure and the readback metadata records the failed publication state. This is required to close the actual mission-delivery gap instead of merely reporting local capture success.

### Bound payload product size at `512 KiB`
The current `DpBufferManager` bins are too small for realistic payload JPEGs. This change will add a bounded large bin and a payload publication ceiling of `512 KiB` for the canonical `.fdp` data region. Oversize captures fail publication instead of introducing chunking or helper redesign. Proof scripts will sample multiple resolutions to document representative JPEG sizes against this ceiling.

### Extend readback metadata instead of adding new operator commands
`PayloadCaptureMetadata`, `PAYLOAD_GET_LAST_CAPTURE_METADATA`, status events, and the payload CSP metadata reply will gain canonical payload product identity and publication state. This lets operator flows correlate payload readback with the official `.fdp` without adding new public browse/download commands.

## Risks / Trade-offs

- [Real target JPEGs may exceed the chosen ceiling] → Add multi-resolution size sampling to hosted/target probes, record the observed size envelope, and keep oversize failure explicit in command/readback semantics.
- [Canonical publication introduces new failure points after local capture] → Preserve `.jpg + .json` diagnostic artifacts and surface a distinct publication failure result/detail code.
- [Payload CSP metadata wire shape changes] → Keep the same service ownership and extend only the metadata reply structure plus unit tests.
- [Target proof would require stitching together target payload backend closure and governed node-`5` file/downlink closure without a single repo-owned wrapper] → Keep this change at hosted official closure, document the missing target wrapper as a bounded follow-on, and do not relabel Pi-local direct payload capture or generic target file/downlink evidence as target payload official closure.
- [Existing HK `.fdp` probes and reliable-transfer semantics could be over-claimed] → Keep payload proof records and registry entries separate from the current HK `.fdp` family and state explicitly that the reliable helper is unchanged.

## Migration Plan

1. Create the OpenSpec proposal/design/spec/tasks and delta specs for the new payload data-product family plus modified capabilities.
2. Extend `PayloadOpsController` and runtime metadata to publish canonical payload `.fdp` artifacts and expose their identity.
3. Increase the `DpBufferManager` configuration with a bounded payload-capable bin and enforce the `512 KiB` publish ceiling.
4. Add decode/extract tooling plus unit tests for serialization, oversize failure, and metadata parity.
5. Add hosted probe coverage for multi-resolution capture, payload `.fdp` byte-match, decode, and JPEG extraction parity.
6. Register the target node-`5` payload `.fdp` proof boundary explicitly as deferred until one repo-owned COMM-backed payload wrapper can drive the same `.fdp` oracle without falling back to Pi-local direct payload capture.
7. Reconcile active docs/registry/roadmap, run OpenSpec validation, and take the change to local-ready.

## Open Questions

- None for this change. The payload artifact family, owner boundary, success semantics, and proof scope are locked by the approved plan.
