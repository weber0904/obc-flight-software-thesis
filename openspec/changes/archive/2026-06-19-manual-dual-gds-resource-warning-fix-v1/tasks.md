## 1. Formalize The Change

- [x] 1.1 Write the proposal, design, and delta specs for the corrected resource-surface semantics.
- [x] 1.2 Update canonical docs so `SYS_MEM_RSS_MB` and the resource warning events describe current-RSS and threshold-crossing behavior.

## 2. Fix Runtime Semantics

- [x] 2.1 Replace hosted `ru_maxrss` sampling with current resident-memory sampling on macOS and Linux and add hosted-runtime unit coverage.
- [x] 2.2 Add threshold-crossing resource-warning latches to `WatchdogSupervisor` and extend watchdog unit tests for no-spam re-emission behavior.

## 3. Verify And Record Results

- [x] 3.1 Rerun hosted headless manual dual-GDS auth plus a normal secure command and confirm corrected RSS semantics without spurious low-memory spam.
- [x] 3.2 Rerun a bounded target manual dual-GDS auth plus a normal secure command as a regression check and record any residual auth instability separately from this fix.
