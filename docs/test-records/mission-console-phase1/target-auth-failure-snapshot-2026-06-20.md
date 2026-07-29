# Target Auth Failure Snapshot 2026-06-20

Status: branch-scoped debugging snapshot for `feature/mission-console-phase1`.  
Date: `2026-06-20` local lab time.  
Scope: current maintained target manual dual-GDS path only.  
Path authority: registry entry `70` ancestry plus the maintained target manual
dual-GDS operator surface; this note does not register a new reusable
verification path.

## Why this snapshot exists

This snapshot freezes the environment state for the current failure symptom:

- `manual_secure_ops.py` S-band auth establish timed out waiting for
  `CHALLENGE`
- observed traceback path:
  - `/private/tmp/manual-dual-gds/target-ground/captures/sband-southbound-to-gds.bin`

Important clarification:

- although the conversational shorthand called this a "hosted manual dual-gds"
  failure, the concrete failing path at snapshot time was the target manual
  dual-GDS surface, not the hosted surface

This snapshot intentionally separates two questions:

1. does the maintained target ground path still carry bytes without the custom
   auth helper?
2. if yes, is the remaining failure in secure-auth handshake closure rather
   than gross hardware/link breakage?

## Snapshot Boundaries

- no shared target baseline restart was performed for this snapshot
- no A/B manager rerun was performed inside the snapshot itself
- only read-only status capture plus one bounded direct non-auth stock
  `fprime-cli command-send` probe were used

## Current Failure Symptom

Representative failure:

```text
TimeoutError: timed out waiting for CHALLENGE on /private/tmp/manual-dual-gds/target-ground/captures/sband-southbound-to-gds.bin
```

Interpretation at snapshot start:

- custom auth request packets appear to leave the ground side
- no usable S-band handshake response is observed returning to the ground side

## Software State

### Target Baseline Surface

Observed from:

- `bash scripts/manual_ops/target/status_target_manual_baseline.sh`
- `/private/tmp/manual-dual-gds/target-baseline/ensure-target-baseline.json`

State:

- `surfaceType=target-manual-baseline`
- `surfaceRoot=/private/tmp/manual-dual-gds/target-baseline`
- `ownerPid=14401`
- `lifecycleState=prepared`
- `target-auth-preflight-applied=True`
- managed override present:
  - `operator@subsystem.local:subsystem-sband-csp.service:58-sband-ingress-diagnostics.conf`

Baseline readiness verdict:

- `verdict=ready`
- `problemsFound=[]`
- `remainingProblems=[]`
- `repairsPerformed=[]`

Observed target environment:

- `TARGET_COMM_PROFILE=sband`
- `COMM_CSP_NODE=5`
- `COMMAND_AUTHORITY_PROFILE=sband-primary`
- `OBC_GPS_SOURCE_MODE=live-uart`

### Ground Surface

Observed from:

- `bash scripts/manual_ops/target/status_target_manual_ground_surface.sh`
- `/private/tmp/manual-dual-gds/target-ground/manifest.json`

State:

- `surfaceType=target-manual-ground-dual-gds`
- `surfaceRoot=/private/tmp/manual-dual-gds/target-ground`
- `ownerPid=14736`
- `lifecycleState=running`
- dictionary:
  - `$REPO_ROOT/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json`

S-band operator surface:

- GDS bind: `0.0.0.0:55863`
- TTS port: `55864`
- GUI: `http://127.0.0.1:55867`
- southbound kind: `tcp`
- southbound endpoint: `subsystem.local:18520`
- southbound COMM node: `5`

UHF operator surface:

- GDS bind: `0.0.0.0:55865`
- TTS port: `55866`
- GUI: `http://127.0.0.1:55868`
- southbound kind: `serial`
- southbound endpoint: `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
- southbound COMM node: `6`

### Local Capture And Log State

Observed near `2026-06-20 12:24:43 CST` before the direct non-auth probe:

- `/private/tmp/manual-dual-gds/target-ground/captures/sband-gds-to-southbound.bin`
  - `115 bytes`
- `/private/tmp/manual-dual-gds/target-ground/captures/sband-southbound-to-gds.bin`
  - `0 bytes`
- `/private/tmp/manual-dual-gds/target-ground/captures/uhf-gds-to-southbound.bin`
  - `0 bytes`
- `/private/tmp/manual-dual-gds/target-ground/captures/uhf-southbound-to-gds.bin`
  - `0 bytes`

Local log file sizes at snapshot time:

- `/private/tmp/manual-dual-gds/target-ground/sband/logs/fprime-gds.log`
  - `699 bytes`
- `/private/tmp/manual-dual-gds/target-ground/sband/logs/ground-ttc-gateway.log`
  - `651 bytes`
- `/private/tmp/manual-dual-gds/target-ground/uhf/logs/fprime-gds.log`
  - `699 bytes`
- `/private/tmp/manual-dual-gds/target-ground/uhf/logs/ground-ttc-gateway.log`
  - `528 bytes`

## Hardware And Shared-Service State

Observed from `ensure-target-baseline.json`:

- `obc.local can0`: present, `UP`, `ERROR-ACTIVE`, `restart-ms 100`
- `subsystem.local can0`: present, `UP`, `ERROR-ACTIVE`, `restart-ms 100`
- `subsystem.local can1`: present, `UP`, `ERROR-ACTIVE`, `restart-ms 100`
- `sbandListenerReady=true`

Observed journal markers:

- OBC CSP init: seen
- OBC ground-link configured: seen
- OBC runtime started: seen
- OBC S-band availability state: `UP`
- OBC S-band ground-link state: `DOWN`
- OBC UHF ground-link state: `UP`

Observed shared services:

- `obc-comm-csp-stack.service`: `active/running`
- `subsystem-sband-csp.service`: `active/running`
- `subsystem-uhf-csp.service`: `active/running`
- `subsystem-eps-csp.service`: `active/running`
- `subsystem-adcs-csp.service`: `active/running`

Snapshot interpretation:

- there was no immediate evidence of shared target service collapse
- CAN provisioning and shared services looked baseline-ready at snapshot time

## Direct Non-Auth Minimal Probe

Purpose:

- bypass `manual_secure_ops` secure-auth helper logic
- use the stock GDS command listener to answer only:
  - can a plain command leave the local GDS?
  - can the target-side path observe and classify it?

Command:

```bash
fprime-venv/bin/fprime-cli command-send \
  --dictionary $REPO_ROOT/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json \
  --no-zmq \
  --tts-port 55864 \
  OBCApp.modeManager.MODE_GET
```

Observed local result:

- `fprime-cli` exited `0`
- stdout only showed GDS logging init

### Ground-Side Evidence

Before command:

- `sband-gds-to-southbound.bin = 115 bytes`
- `sband-southbound-to-gds.bin = 0 bytes`

After command at `2026-06-20 12:24:54 CST`:

- `sband-gds-to-southbound.bin = 134 bytes`
- `sband-southbound-to-gds.bin = 0 bytes`

Ground gateway log after the probe:

```text
ground_ttc_gateway gds-read bytes=19
```

Interpretation:

- the plain stock command definitely left GDS and entered the S-band gateway
- no return bytes were captured back to the ground on the maintained S-band
  downlink file during this probe

### Target-Side Evidence

Subsystem S-band service journal:

```text
COMM ingress diagnostic: node=5 endpoint=tcp-listen bytes-read=19 ingress-bytes=19 accepted=1 rx-errors=0 rx-chunks=6 uplink-queue-depth=19 preview=20 44 04 12 00 10 00 c0 00 00 05 00 00 10 03 00 01 d6 9c
```

OBC command-path journal:

```text
WARNING_HI: (commandIngressAuthority) COMMAND_AUTHORITY_REJECTED : Command authority rejected opcode 0x10030001 ingress 0 identity 1 role 1 class 1 reason 5 response VALIDATION_ERROR (2)
```

Interpretation:

- the plain command reached the subsystem S-band ingress
- the command then reached OBC-side command authority
- the maintained target path rejected it as unauthorised / invalid under the
  current command-authority policy
- this is strong evidence that the basic hardware-plus-transport path was
  alive during the snapshot, even though secure auth itself was not closing

## Snapshot Conclusion

What this snapshot supports:

- the current failing case is not immediately explained by detached CAN,
  missing shared services, dead `subsystem.local:18520`, or a dead local GDS
  TTS path
- a plain non-auth command can still traverse:
  - local `fprime-cli`
  - local GDS
  - local `ground_ttc_gateway`
  - subsystem S-band ingress
  - OBC command-authority surface
- the remaining blocker is narrower:
  - secure-auth request/response closure on the maintained target S-band path
  - or the proof/oracle layer that decides when `CHALLENGE` has been observed

What this snapshot does not prove:

- secure-auth math is correct on the current failing attempt
- the target path is free of timing flake
- the maintained S-band return path is healthy enough for every handshake
  attempt
- Mission Console target parity is closed

## Same-Surface Contrast On The Same Day

Later on the same maintained `target-ground` surface, a bounded direct auth
attempt on `--band sband` succeeded without changing the surface family:

```bash
fprime-venv/bin/python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /private/tmp/manual-dual-gds/target-ground/manifest.json \
  auth establish
```

Observed result:

```json
{
  "status": "authenticated",
  "band": "sband",
  "challengeSource": "wire-capture",
  "authStatusSource": "wire-capture",
  "serviceId": 1
}
```

Observed contrast points:

- `sband-gds-to-southbound.bin`
  - earlier snapshot: `115 bytes`
  - after successful auth: `212 bytes`
- `sband-southbound-to-gds.bin`
  - earlier snapshot: `0 bytes`
  - after successful auth: `8192 bytes`

Successful auth-time target evidence:

- subsystem S-band ingress:
  - `bytes-read=23` for auth request
  - `bytes-read=55` for follow-up secure packet
- OBC journal:
  - `SECURE_AUTH_CHALLENGE_ISSUED ingress 0 service 1`
  - `COMMAND_SESSION_OPENED ingress 0 identity 1 role 1 session 1`
  - `SECURE_AUTH_ESTABLISHED ingress 0 service 1`

Why this contrast matters:

- the successful run used the same `S-band` target manual surface, not UHF
- the auth helper did not succeed by accidentally reading a UHF path
- the more credible remaining explanation is intermittent S-band handshake
  closure or watcher/readiness flake on the maintained target path, not simple
  band confusion

## Comparison Guidance For Later Reruns

If a later rerun differs from this snapshot, classify the first divergence:

1. `baseline-readiness`
   - CAN not `ERROR-ACTIVE`
   - shared service not `active`
   - `subsystem.local:18520` not effectively receiving bytes
2. `probe-oracle`
   - target journal shows auth activity but local watcher still times out
   - late-arrival challenge/status is misclassified as a new attempt
3. `product`
   - plain and auth traffic both definitely reach OBC-side authority surfaces
   - watcher/oracle is correct
   - secure-auth still fails closed

This ordering follows the current target/lab debugging lessons and avoids
promoting a readiness or oracle problem into a product diagnosis too early.
