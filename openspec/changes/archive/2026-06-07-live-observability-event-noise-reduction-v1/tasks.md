## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `comm-subsystem`, `onboard-data-products-and-live-beacon`, and `interface-contract-index`.
- [x] 1.2 Validate the change artifacts with `openspec validate live-observability-event-noise-reduction-v1`.

## 2. Event Emission Policy Changes

- [x] 2.1 Refactor `OnboardStateMonitor` so `STATE_MONITOR_UPDATED` emits only when `healthMask`, `faultMask`, or `qualityMask` changes while keeping telemetry publication and reduced-state cache behavior intact.
- [x] 2.2 Refactor `CspBridge` so success ping no longer emits `CSP_PING_RESULT`, while failure ping still emits `CSP_PING_RESULT(false, ...)` and existing error behavior remains intact.
- [x] 2.3 Extend `EventManager` with a checked-in default filtered event ID config surface and use it to suppress packetized `OBCApp.dpWriter.FileWritten` by default without removing local text/journal visibility.

## 3. Tests And Current Docs

- [x] 3.1 Update focused unit tests for `OnboardStateMonitor`, `CspBridge`, and `EventManager` to cover the new semantics.
- [x] 3.2 Update `docs/interfaces.md` and `docs/verification-debugging-lessons.md` so they describe the quieter current live event surface accurately.
- [x] 3.3 Run focused build/test validation plus `openspec validate live-observability-event-noise-reduction-v1` and `openspec validate --specs`, then mark these tasks complete with the actual results.

## Validation Notes

- `openspec validate live-observability-event-noise-reduction-v1`: pass
- `openspec validate --specs`: pass
- `cmake --build build-fprime-automatic-native-ut --target OBC_Components_OnboardStateMonitor_ut_exe OBC_Components_CspBridge_ut_exe Svc_EventManager`: pass
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_OnboardStateMonitor_ut_exe`: pass
- `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CspBridge_ut_exe`: pass
- `cmake --build build-fprime-automatic-native --target OBC`: pass
- Hosted runtime spot checks after fresh native build confirmed:
  - `STATE_MONITOR_UPDATED` no longer repeats every cycle when masks are stable
  - healthy `CSP_PING_RESULT` success no longer appears in the current hosted event stream
  - `HK_TREND_PRODUCT_WRITTEN` and local `DpWriter FileWritten` text-log visibility remain present
- `EventManager` focused UT source was updated, but this repo's current build policy keeps `FPRIME_ENABLE_FRAMEWORK_UTS=OFF`, so no standalone `Svc_EventManager_ut_exe` was materialized. The filtered-ID change was validated by successful compile of `Svc_EventManager` plus hosted runtime behavior instead of a framework-UT executable run.
