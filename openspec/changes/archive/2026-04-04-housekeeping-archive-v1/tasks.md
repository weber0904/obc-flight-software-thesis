## 1. Change Setup

- [x] 1.1 Add the housekeeping-archive, resource-storage, and verification-evidence delta specs for ring archives, index metadata, and governed downlink behavior.
- [x] 1.2 Add the technical design for ring-file storage, cached-state capture, and controlled downlink commands.

## 2. Core Implementation

- [x] 2.1 Implement the `HousekeepingArchive` component, archive file/index format, and runtime snapshot provider without duplicating subsystem transport polling.
- [x] 2.2 Wire the component into the OBC topology and existing file-downlink path, including shared runtime-root storage handling and governed archive commands.

## 3. Verification And Evidence

- [x] 3.1 Add unit or integration tests covering periodic capture, slot rotation, index generation, and slot/index downlink request behavior.
- [x] 3.2 Record repository evidence and update user-facing documentation for the first housekeeping archive slice.
- [x] 3.3 Validate the OpenSpec change and main specs after the implementation and evidence are aligned.

## 4. Finalization

- [x] 4.1 Archive the change after validation succeeds.
- [x] 4.2 Commit the archived housekeeping-archive change with a Conventional Commit message.
