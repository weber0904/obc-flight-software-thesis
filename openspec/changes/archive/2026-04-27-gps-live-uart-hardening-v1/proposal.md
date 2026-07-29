## Why

`gps-live-uart-source-v1` 已經建立第一版 live UART GPS 硬體路徑，但 merge 後的 review 指出幾個值得立即補強的邊界條件：blank-line handling、serial open defensiveness、baudrate parsing bounds、PTY test robustness，以及 target probe temporary artifact hygiene。這些問題不會推翻既有功能，但現在補齊最能避免後續在 CAN FD 與 TT&C 主線上累積低層噪音。

## What Changes

- Harden the live UART GPS source so blank serial lines are skipped instead of being reported as `NO_SOURCE_DATA`.
- Make serial device open/configure behavior more defensive without changing the governed `fake`/`replay`/`live-uart` source contract.
- Reject out-of-range baudrate environment values before narrowing them into the runtime GPS configuration.
- Tighten PTY-backed GPS source tests so setup failures short-circuit cleanly and the test harness uses the correct PTY declarations.
- Clean up the governed Raspberry Pi live GPS probe temporary directory on exit.

## Capabilities

### New Capabilities
- None

### Modified Capabilities
- `gps-subsystem`: harden the bounded live UART source behavior so ignorable serial whitespace does not look like transport loss, invalid baudrate configuration fails safely, and governed live-source validation remains deterministic

## Impact

- Affected code: `GpsBridge` runtime config parsing, serial GPS source implementation, PTY-backed GPS support tests, and the governed target GPS probe script.
- Affected systems: live UART GPS on `obc.local:/dev/serial0`, hosted GPS helper tests, and probe cleanup behavior.
- No changes to F' command dictionaries, GPS public command/event/tlm names, CSP contracts, or the direct `GPS -> OBC` architecture baseline.
