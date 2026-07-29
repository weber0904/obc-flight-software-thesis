# shared-canfd-csp-bus-foundation-v1 Tasks

## 1. Runtime Backend

- [x] 1.1 Extend `simulators/csp/CspRuntime` with a Linux-only `socketcan` backend using official libcsp SocketCAN integration.
- [x] 1.2 Add governed runtime configuration for `CSP_CAN_DEVICE` and `CSP_CAN_PROMISC`, while keeping `zmqhub` as the default baseline.
- [x] 1.3 Add fail-fast validation for missing, non-CAN, or down Linux CAN interfaces.
- [x] 1.4 Update build prerequisites so Linux builds compile the SocketCAN backend with `libsocketcan-dev`.

## 2. CAN Launcher And Probe Flow

- [x] 2.1 Add a ground-only GDS launcher for the CAN validation topology.
- [x] 2.2 Add subsystem-side CAN launchers for `EPS` and `ADCS` on the primary CAN device and record the reserved CAN device separately.
- [x] 2.3 Add an OBC-side CAN launcher that starts `OBC` through `socketcan`.
- [x] 2.4 Add a governed three-host CAN probe that exercises `csp ping`, `eps get`, `adcs get`, one EPS command, and one ADCS command while collecting active/reserved CAN evidence.

## 3. Specs, Registry, And Evidence

- [x] 3.1 Add delta spec updates for the first governed physical internal SocketCAN carrier path.
- [x] 3.2 Add or update the test-record evidence for CAN wiring, `parentdev` mapping, CAN statistics, `candump`, and reserved-channel isolation.
- [x] 3.3 Update the verification-path registry with a distinct entry for the new physical internal CSP path.

## 4. Verification And Closeout

- [x] 4.1 Run fresh local verification after the implementation is complete.
- [x] 4.2 Run the governed CAN hardware probe on the target topology and capture the resulting evidence.
- [x] 4.3 Run `openspec validate shared-canfd-csp-bus-foundation-v1` and `openspec validate --specs`.
- [x] 4.4 Sync/archive the change after the hardware proof and evidence are complete.
