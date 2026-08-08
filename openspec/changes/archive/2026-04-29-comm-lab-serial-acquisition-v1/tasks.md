## 1. Setup

- [x] 1.1 Validate the OpenSpec artifacts before implementation.
- [x] 1.2 Preserve the guarded TT&C failed-attempt verdict as diagnostic context, not a proof.

## 2. Acquisition Probe

- [x] 2.1 Add `scripts/run_comm_lab_serial_acquisition_probe.sh` with a macOS passive raw receiver and `subsystem.local` raw serial sender.
- [x] 2.2 Implement bounded preamble, magic, sequence, length, and CRC validation so cold-start garbage can be skipped.
- [x] 2.3 Keep cleanup and process ownership explicit for local receiver and remote sender paths.

## 3. Evidence And Registry

- [x] 3.1 Add `evidence/records/comm-lab-serial-acquisition-v1/README.md` with commands, endpoints, observed verdict, non-claims, and the TT&C stop context.
- [x] 3.2 Update `evidence/verification-path-registry.md` only if the focused acquisition probe passes.

## 4. Verification And Closeout

- [x] 4.1 Run `bash scripts/run_verification_ci.sh build-artifacts/comm-lab-serial-acquisition-v1-closeout`.
- [x] 4.2 Run the focused acquisition probe with `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`, `/dev/serial0`, and `115200`.
- [x] 4.3 Run `openspec validate comm-lab-serial-acquisition-v1` and `openspec validate --specs`.
- [x] 4.4 Archive the change and commit a reviewable boundary if the verdict is coherent.
