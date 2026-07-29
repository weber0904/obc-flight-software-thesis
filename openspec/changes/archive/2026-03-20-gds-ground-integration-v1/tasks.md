# Tasks: gds-ground-integration-v1

## 1. Deployment topology

- [x] 1.1 Replace the minimal hosted runtime wiring with `CdhCore` + `ComFprime` + `Drv::TcpClient`
- [x] 1.2 Add rate-group-driven scheduling for the hosted deployment
- [x] 1.3 Keep the hosted runtime entrypoint usable with or without a configured GDS TCP target

## 2. Launch path

- [x] 2.1 Update the repo-local stack launcher to accept ground-link settings
- [x] 2.2 Add a repo-local helper for the documented hosted + GDS flow

## 3. Verification and evidence

- [x] 3.1 Rebuild the hosted deployment after the topology upgrade
- [x] 3.2 Launch `fprime-gds` in no-app headless mode against the hosted dictionary
- [x] 3.3 Launch the hosted stack against GDS and confirm the TCP connection
- [x] 3.4 Record the verification evidence under `docs/test-records/`

## 4. Spec and narrative sync

- [x] 4.1 Update the affected OpenSpec capability deltas
- [x] 4.2 Update the narrative source documents with the GDS-connected hosted flow
