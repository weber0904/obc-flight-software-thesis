# Tasks: deployment-runtime-v1

## 1. Runtime topology and executable

- [x] 1.1 Add a minimal hosted OBC topology module
- [x] 1.2 Register the OBC deployment executable in the root build
- [x] 1.3 Add the hosted runtime main loop and interactive control surface

## 2. Runtime integration helpers

- [x] 2.1 Add public runtime helpers required to operate the existing components without re-implementing subsystem logic elsewhere
- [x] 2.2 Configure runtime transport switching for TCP mock and PTY-backed UART-like paths

## 3. Dev stack and comm mock

- [x] 3.1 Add the standalone hosted radio mock server executable
- [x] 3.2 Add a repo-local script that launches the full software-only dev stack

## 4. Verification and evidence

- [x] 4.1 Build the hosted deployment successfully
- [x] 4.2 Launch the integrated dev stack and exercise basic operations
- [x] 4.3 Re-run unit / integration verification gates after the runtime additions
- [x] 4.4 Record integrated runtime evidence under `evidence/records/`

## 5. Spec and narrative sync

- [x] 5.1 Update the affected OpenSpec capability deltas
- [x] 5.2 Update the narrative source documents with the hosted runtime path
