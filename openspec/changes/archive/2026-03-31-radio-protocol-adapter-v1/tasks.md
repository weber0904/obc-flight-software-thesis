## 1. Change Scaffolding

- [x] 1.1 Sync the radio protocol-adapter delta specs with the current comm architecture and documented KISS / vendor-agnostic stance
- [x] 1.2 Identify the current request/response parsing logic that should move behind a named adapter boundary

## 2. Protocol Adapter Implementation

- [x] 2.1 Add an explicit radio protocol adapter interface and refactor the current hosted text protocol into the first named default adapter
- [x] 2.2 Update the runtime configuration and helper entrypoints so a radio protocol adapter can be selected while preserving the current default behavior
- [x] 2.3 Extend comm integration tests so the named default adapter path is exercised through the shared byte-stream transport

## 3. Verification And Documentation

- [x] 3.1 Re-run the shared local verification gate to confirm the adapter extraction does not regress the current comm baseline
- [x] 3.2 Capture reviewable evidence for the default adapter selection and regression outcome under `evidence/records/radio-protocol-adapter-v1/`
- [x] 3.3 Update repo documentation and narrative comm docs to explain the adapter layer and its future KISS / vendor extension point

## 4. Finalize

- [x] 4.1 Validate the OpenSpec change and sync the resulting deltas into the main specs
- [x] 4.2 Archive the change and preserve a clean git state for the next radio or hardware slice
