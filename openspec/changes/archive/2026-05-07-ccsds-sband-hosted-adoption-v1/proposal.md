## Why

The `ccsds-ground-link-spike` evidence recommends adopting the bounded hosted S-band CCSDS proof path, but the repository still treats CCSDS as a spike-only executable while default hosted `OBC` remains a `ComFprime` topology. This change converts that recommendation into an explicit, governed hosted S-band CCSDS default-path decision without broadening into RF, UHF CCSDS, target deployment, reliable transfer, or command-authority policy.

## What Changes

- Promote the hosted S-band CCSDS path to the default hosted `OBC` path.
- Keep the default command, event, telemetry, and file namespace as `OBCApp.*`.
- Preserve the old hosted `ComFprime` topology as an explicit legacy/regression executable named `OBC_ComFprimeLegacy` with `OBCAppComFprimeLegacy.*` command namespace.
- Update hosted development scripts so no-override `run_dev_stack.sh` and `run_gds_stack.sh` use CCSDS S-band node `5`, `ground_ttc_gateway` raw relay, and `fprime-gds` `space-packet-space-data-link` framing.
- Replace formal spike probe naming with a hosted S-band CCSDS adoption probe, while keeping the old spike probe script as a compatibility wrapper.
- Add verification-only raw CCSDS byte capture and decoding for the adoption probe so evidence records frame metadata and APID flow observations.
- Keep existing `ComFprime`, UHF, target/Pi, and physical-link evidence boundaries separate and explicitly legacy where needed.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: default hosted S-band CCSDS adoption, legacy hosted `ComFprime` boundary, and non-goal exclusions.
- `ground-ttc-gateway`: optional verification capture for raw relayed bytes and CCSDS default hosted gateway settings.
- `platform-baseline`: default hosted `OBC` topology/namespace decision and legacy `ComFprime` executable boundary.
- `verification-evidence`: adoption evidence requirements, decoded frame/APID observations, and legacy-regression evidence separation.
- `verification-path-registry`: new reusable default hosted CCSDS S-band path while preserving the historical spike path separately.

## Impact

- Hosted deployment registration, topology namespace ownership, dictionary discovery, and launch scripts.
- Hosted S-band/CCSDS probes and old `ComFprime` regression probes.
- `ground_ttc_gateway` verification-only capture behavior.
- New evidence under `docs/test-records/ccsds-sband-hosted-adoption-v1/`.
- README, scripts README, verification-path registry, and untracked pending planning notes.
