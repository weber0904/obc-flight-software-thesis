## 1. Robustness Scope And Probe Design

- [x] 1.1 Define the sustained-exchange and reconnect behaviors that belong in this slice without changing the frame v1 format.
- [x] 1.2 Identify the current host-peer and probe limitations that prevent governed robustness validation.

## 2. Host Peer And Transport Hardening

- [x] 2.1 Extend the host-side framed transparent peer so it can participate in repeated binary-safe exchange and expose reviewable markers for robustness probes.
- [x] 2.2 Add or extend transport-level tests that cover repeated framed payload exchange and reconnect-oriented behavior without breaking existing baselines.

## 3. Governed Probe And Evidence

- [x] 3.1 Add a governed Raspberry Pi to host robustness probe that exercises sustained framed exchange over the current UART/RS485 path.
- [x] 3.2 Add a governed reconnect probe or reconnect phase within the robustness probe that proves post-restart framed exchange.
- [x] 3.3 Record dedicated evidence and update user-facing docs to explain what this robustness slice validates and what still remains out of scope.

## 4. Verification And Closeout

- [x] 4.1 Run the shared regression gate to prove the existing baselines still pass.
- [x] 4.2 Run the governed framed robustness validation and record the observed sustained-exchange and reconnect results.
- [x] 4.3 Validate the change with OpenSpec and prepare it for archive.
