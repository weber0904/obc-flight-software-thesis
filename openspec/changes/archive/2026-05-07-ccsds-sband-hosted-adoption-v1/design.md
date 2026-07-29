## Context

`ccsds-ground-link-spike` proved a bounded hosted S-band CCSDS path through `OBC_CcsdsGroundLinkSpike`, `sband_comm_csp_node` node `5`, `ground_ttc_gateway` raw byte relay, and `fprime-gds` `space-packet-space-data-link` framing. `hosted-obc-runtime-maintainability-v1` then moved shared hosted parsing and command dispatch into `OBC_Runtime`, reducing the risk of promoting the CCSDS path without reintroducing duplicate command loops.

The remaining architectural issue is that the default hosted `OBC` still represents the old `ComFprime` topology while CCSDS remains named and specified as a spike. That is no longer aligned with the selected S-band flight protocol direction.

## Goals / Non-Goals

**Goals:**

- Make hosted `OBC` the default S-band CCSDS deployment with `OBCApp.*` operator namespace.
- Preserve the existing hosted `ComFprime` topology as `OBC_ComFprimeLegacy` for regression and historical evidence.
- Keep the S-band CCSDS path on COMM node `5` and services `30` through `32`.
- Add verification-only CCSDS raw-byte capture and decoding for APID/frame observations.
- Keep old `ComFprime`, UHF, direct GDS, target/Pi, and physical-link evidence boundaries separate.

**Non-Goals:**

- UHF CCSDS.
- RF or real-radio behavior.
- Raspberry Pi or flight-target deployment claims.
- Reliable file transfer, CFDP, ARQ, NACK, retry, or sequence-count-as-reliability claims.
- Command authority, sessions, authentication, failover, pass scheduling, or HK data-product field alignment.

## Decisions

- **Promote CCSDS by namespace, not by sidecar.** The default hosted `OBC` shall use the CCSDS topology and keep `OBCApp.*`. The old `ComFprime` topology is renamed to a legacy namespace so future feature work targets the formal default path first.
- **Keep legacy regression explicit.** `OBC_ComFprimeLegacy` preserves reviewable access to historical `ComFprime` paths without letting them masquerade as the satellite default.
- **Make dictionary discovery deployment-aware.** Scripts shall look up the dictionary for the selected deployment name, because `OBC` and `OBC_ComFprimeLegacy` have different command namespaces.
- **Capture raw bytes without teaching the gateway CCSDS.** `ground_ttc_gateway` may tee raw northbound/southbound bytes into verification files, but it shall continue to relay bytes without parsing CCSDS semantics. The decoder belongs to scripts/tests.
- **Decode APID/frame observations, not reliability.** The adoption probe shall decode enough TC/TM/Space Packet structure to observe SCID, VCID, frame size, APIDs, and sequence counters where present. These observations are evidence of path framing, not proof of packet-loss tolerance.

## Risks / Trade-offs

- **Hosted probe drift** -> Pin legacy probes to `OBC_ComFprimeLegacy` and default/adoption probes to `OBC`.
- **Wrong dictionary selected** -> Add deployment-aware dictionary lookup and pass dictionary paths explicitly in probes.
- **Topology namespace rename churn** -> Keep source changes mechanical and preserve shared `OBC_Runtime` service adapters.
- **Gateway capture mistaken for gateway semantics** -> Specs and evidence shall state that gateway capture is verification-only and the gateway does not parse CCSDS.
