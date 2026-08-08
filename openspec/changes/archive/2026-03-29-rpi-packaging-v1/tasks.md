# Tasks: rpi-packaging-v1

## 1. OpenSpec baseline

- [x] 1.1 Write the proposal, delta specs, and design for the Raspberry Pi packaging slice

## 2. Bundle creation flow

- [x] 2.1 Add governed helpers and packaging assets that stage a Raspberry Pi install bundle from Linux target artifacts
- [x] 2.2 Generate bundle metadata with packaged version information and file digests

## 3. Install root and installed launcher flow

- [x] 3.1 Add a governed install helper that unpacks a selected bundle into a fixed user-writable Raspberry Pi install root and maintains a `current` release pointer
- [x] 3.2 Add an installed-stack launcher path that starts the integrated target flow from the installed release while keeping runtime data outside the release payload

## 4. Validation and evidence

- [x] 4.1 Create a Raspberry Pi bundle, install it on target hardware, and verify the installed stack launches from the install root
- [x] 4.2 Record package/install/run evidence and update the relevant operator-facing documentation
- [x] 4.3 Run OpenSpec validation, archive the change, and sync the main specs
