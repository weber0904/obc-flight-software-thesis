# payload-virtual-csp-node-v1 Test Record

## Scope

- Branch: `feature/payload-ops-v2`
- OpenSpec change: `payload-virtual-csp-node-v1`
- Date: 2026-05-21

This record captures the in-branch verification state for the first payload CSP
subsystem-style boundary.

The code-level scope covered here is:

- payload virtual node `7` reservation and future service-port reservation
  `40..49`
- first OBC-internal payload CSP shim hosted on node `1`
- read-oriented payload CSP request/reply surfaces for:
  - `STATUS`
  - `CAPABILITIES`
  - `LAST_CAPTURE_METADATA`
- runtime snapshot translation through `PayloadCspGateway`
- hosted proof that a non-OBC CSP client can query the payload shim without
  creating a second ground operator plane

## Commands

Commands run from `$REPO_ROOT`:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native --target OBC payload_csp_probe_main -j 8
PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native-ut --target payload_csp_gateway_unit_test OBC_Components_CommController_ut_exe -j 8
./build-fprime-automatic-native-ut/bin/Darwin/payload_csp_gateway_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe
bash scripts/run_payload_virtual_csp_node_v1_hosted_probe.sh
PATH="$PWD/fprime-venv/bin:$PATH" openspec validate payload-virtual-csp-node-v1
```

## Current Results

- Fresh native build: PASS
- Focused UT target build: PASS
  - `payload_csp_gateway_unit_test`: PASS
  - `OBC_Components_CommController_ut_exe`: PASS
- Hosted payload CSP shim probe: PASS
  - canonical PASS root:
    `/tmp/payload-virtual-csp-node-v1-hosted.TEpRcX`
- `openspec validate payload-virtual-csp-node-v1`: PASS

## Focused Coverage

`payload_csp_gateway_unit_test` currently proves:

- status snapshot fields and payload-state flags map to the CSP status reply
- payload capabilities map cleanly into the CSP capabilities reply
- last-capture metadata maps cleanly into the CSP metadata reply

`OBC_Components_CommController_ut_exe` currently proves:

- the disabled-ground-link hosted configuration does not spuriously latch
  `COMM_PRIMARY_UNAVAILABLE` when no subsystem-health node is configured
- this keeps the hosted internal payload CSP shim proof from being invalidated
  by unrelated COMM FDIR escalation while ground-link is intentionally disabled

## Hosted Proof Status

Repository-owned hosted entry point:

```bash
bash scripts/run_payload_virtual_csp_node_v1_hosted_probe.sh
```

Canonical path under test:

```text
payload_csp_probe_main(node 8) -> hosted internal libcsp runtime -> OBC node 1 payload CSP shim ports 34/35/36
```

Current truth:

- the first payload CSP service is hosted inside the OBC process on node `1`
- the active shim ports are:
  - `34` = `STATUS`
  - `35` = `CAPABILITIES`
  - `36` = `LAST_CAPTURE_METADATA`
- future payload virtual node `7` and service ports `40..49` remain reserved
  only; they are not live in the current OBC process
- the hosted proof reuses the governed internal CSP runtime and stays distinct
  from the default CCSDS node-`5` operator command path
- the current libcsp runtime reserves client source ports starting at `41`, so
  the first node-`1` shim intentionally avoids `41+` and uses bindable local
  ports `34..36`

## Non-Claims

- no live node-`7` payload process yet
- no second ground operator control plane
- no target Pi payload CSP shim proof yet
- no target real `libcamera` capture or raw-register closure yet
