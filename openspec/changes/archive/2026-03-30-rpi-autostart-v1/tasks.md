# Tasks: rpi-autostart-v1

## 1. OpenSpec baseline

- [x] 1.1 Write the proposal, delta specs, and design for the Raspberry Pi autostart slice

## 2. Service-managed startup implementation

- [x] 2.1 Add a headless runtime mode so the OBC process can run under systemd without interactive stdin
- [x] 2.2 Add governed systemd unit assets plus repo-local install and status helpers for the Raspberry Pi installed release
- [x] 2.3 Update the installed launch path and operator docs to cover service-managed startup behavior

## 3. Target validation and evidence

- [x] 3.1 Install and enable the governed autostart service on the Raspberry Pi target
- [x] 3.2 Reboot the Raspberry Pi and verify the installed `current` release relaunches through the governed service path
- [x] 3.3 Record reviewable autostart evidence and update the relevant narrative/source documents
- [x] 3.4 Run OpenSpec validation, archive the change, and sync the main specs
