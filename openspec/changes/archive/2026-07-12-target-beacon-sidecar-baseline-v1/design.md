## Context

The target Beacon Viewer initially used the target manual-surface owner to
create PTYs, inject service environment, and capture a remote file. That makes
an operator-facing local surface restart shared OBC and UHF services. The
repository's A/B/C contract assigns that responsibility only to A.

## Goals / Non-Goals

**Goals:**

- Establish one A-owned remote PTY bridge and capture helper for UHF Beacon
  observation.
- Apply or repair its OBC/UHF service settings only while A establishes the
  shared baseline.
- Publish sidecar metadata in A's JSON and manual-baseline manifest.
- Let target manual ground surfaces mirror the published remote capture without
  modifying remote service, drop-in, bridge, or capture-process state.
- Reap only the baseline's uniquely labelled remote helpers when A repairs a
  broken sidecar.

**Non-Goals:**

- RF/OTA or stock-GDS Beacon receipt closure.
- Any change to UHF authority roles, serial ingress ownership, CAN FD policy,
  or Beacon payload semantics.
- A probe-owned target service restart or manual-surface remote cleanup.

## Decisions

### A owns a stable named sidecar

`ensure_target_comm_lab_baseline.py` will use a fixed baseline state root and
unique `pty_pair_bridge --instance-label` value. It records bridge/capture PIDs,
PTY endpoints, remote capture path, service names, frame size, and provenance.
It only recreates the sidecar when its recorded processes or capture path are
unhealthy. This makes repair auditable and avoids generic `pkill` matching.

Alternative considered: make each manual surface own a unique sidecar. This
was rejected because configuring its UHF egress device requires a shared
service restart.

### Baseline service environment is explicit and self-healing

A manages dedicated baseline drop-ins for the OBC Beacon destination and UHF
Beacon egress device. A verifies the effective environment and active service
state; if PTYs change during repair, only A updates the drop-in and restarts
the affected service. Existing serial ingress remains configured separately,
because Beacon egress is auxiliary.

### Manual surfaces are consumers only

Target manual ground owner reads `targetBeaconSidecar` from the current A
baseline manifest. It may fetch the advertised remote capture into its own
local root, but it never invokes `apply_service_override`,
`remove_service_override`, remote `pkill`, or remote bridge/capture startup.
If metadata is absent or unhealthy it publishes no Beacon capability, so
Mission Console takes its existing explicit unsupported fallback.

### C proves observation without changing readiness

The focused target Beacon probe runs A then B, starts the local manual surface,
and validates mirrored provenance plus Mission Console `/beacon`. C records
baseline service invocation IDs before and after and fails if it restarted or
stopped a shared service.

## Risks / Trade-offs

- [Remote workspace lacks the helper/build binary] → classify as provenance or
  baseline-readiness and report it before changing product behavior.
- [Baseline PTY process exits] → A recreates only the recorded label and
  refreshes metadata before consumers start.
- [A's auxiliary egress setting is mistaken for RF closure] → registry and
  evidence retain explicit sidecar/mirror and RF/OTA non-claims.
- [Manual surface starts before A metadata exists] → it safely exposes no
  Beacon capability instead of attempting remote repair.

## Migration Plan

1. Add A-owned sidecar setup and manifest metadata.
2. Replace target manual owner lifecycle code with baseline metadata consumption.
3. Add focused unit tests and an A -> B -> C target proof.
4. Refresh evidence/runbooks, then remove the former manual-owned drop-in names
   through A's ordinary stale-override repair path.

Rollback removes only the new A-owned drop-ins and labelled helper processes
through A; manual surfaces then degrade to unsupported target Beacon state.
