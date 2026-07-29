## 1. OpenSpec Artifacts

- [x] 1.1 Add `proposal.md`, `design.md`, `tasks.md`, and a
  `verification-evidence` delta spec for
  `target-tcp-southbound-parity-foundation-v1`.
- [x] 1.2 Validate the change artifacts with
  `openspec validate target-tcp-southbound-parity-foundation-v1`.

## 2. Three-Host Target TCP Launcher

- [x] 2.1 Add the governed three-host target TCP parity launcher and shared
  helpers for `macOS`, `obc.local`, and `subsystem.local`.
- [x] 2.2 Keep node `5` and node `6` on `subsystem.local` and record node `6`
  as TCP-based southbound emulation, not physical UART provenance.

## 3. First-Wave Target TCP Matrix Cells

- [x] 3.1 Implement target TCP `csp-reachability` on the parity launcher.
- [x] 3.2 Implement target TCP `sband-command` and `sband-file`.
- [x] 3.3 Implement target TCP `uhf-primary-command` and `uhf-primary-file`.

## 4. Verification

- [x] 4.1 Run touched script syntax and helper checks.
- [x] 4.2 Prove two immediate reruns of the target TCP parity launcher.
- [x] 4.3 Record focused evidence and update the umbrella matrix dashboard for
  the passing cells only.
- [x] 4.4 Run `openspec validate target-tcp-southbound-parity-foundation-v1`
  and `openspec validate --specs`.
