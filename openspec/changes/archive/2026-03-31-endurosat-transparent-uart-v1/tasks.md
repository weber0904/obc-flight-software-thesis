## 1. Change Scaffolding And Simulator Scope

- [x] 1.1 Review the current host-side mock transceiver behavior against the legacy EnduroSat transparent-UART manual notes and capture the exact gaps this change will address.
- [x] 1.2 Define the governed transparent peer behavior and runtime entrypoints without regressing the existing `mock-text` comm baseline.

## 2. Transparent Peer And Runtime Wiring

- [x] 2.1 Implement a host-side transparent serial peer mode or companion simulator for the legacy EnduroSat validation path.
- [x] 2.2 Extend the relevant Raspberry Pi and host-side launch or probe scripts so the transparent validation flow can select the correct serial devices and legacy UART settings.
- [x] 2.3 Reuse the existing raw UART exchange path to validate transparent payload transfer without rewriting the controller layer.

## 3. Evidence And Documentation

- [x] 3.1 Add a dedicated evidence record for the legacy EnduroSat transparent-UART validation flow, including explicit scope boundaries for the still-missing control plane.
- [x] 3.2 Update repository-facing docs to describe when to use `mock-text` versus the legacy transparent validation path.

## 4. Verification And Closeout

- [x] 4.1 Run regression checks to prove the default comm baseline still passes after the transparent path is added.
- [x] 4.2 Run the governed Raspberry Pi to host transparent-UART validation flow and record the observed result.
- [x] 4.3 Validate the change with OpenSpec and prepare it for archive.
