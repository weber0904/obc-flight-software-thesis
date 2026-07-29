## Why

The guarded full TT&C attempt over physical lab serial did not prove command uplink or downlink: OBC readback stayed at the pre-command EPS/ADCS state while the COMM ground link reported repeated errors and reconnects. Before retrying TT&C, the repository needs a smaller proof that `subsystem.local` can initiate bounded serial traffic that macOS can acquire and parse without relying on an interactive terminal.

## What Changes

- Add a focused repo-owned lab serial acquisition probe for the `subsystem.local -> macOS` direction.
- Use explicit endpoints: macOS `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`, `subsystem.local:/dev/serial0`, baudrate `115200`.
- Have macOS start a passive raw serial receiver before `subsystem.local` transmits.
- Have `subsystem.local` send a bounded preamble plus framed marker payloads using a standard-library Python raw-serial helper over SSH.
- Validate that macOS can extract exact framed payloads from the stream after acquisition.
- Record the failed guarded TT&C attempt as diagnostic context without claiming TT&C, RF, file/downlink, direct TCP, target OBC, or COMM CAN.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: add a governed lab serial acquisition proof for subsystem-origin bytes toward macOS.
- `verification-evidence`: require acquisition evidence to distinguish raw serial acquisition from TT&C success.
- `verification-path-registry`: register the acquisition path separately from subsystem UART preflight and gateway-backed TT&C only if the focused probe passes.

## Impact

- Adds a focused probe script under `scripts/`.
- Adds a test record under `docs/test-records/comm-lab-serial-acquisition-v1/`.
- Does not modify the COMM CSP service contract, `ground_ttc_gateway`, stock F' framing, `fprime-gds`, or target platform build strategy.
