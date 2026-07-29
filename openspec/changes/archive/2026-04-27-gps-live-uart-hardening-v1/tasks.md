## 1. Live UART runtime hardening

- [x] 1.1 Harden live UART serial-source behavior so blank lines are skipped, non-blocking open is used, and bounded timeout behavior remains intact
- [x] 1.2 Harden `GpsBridge` baudrate parsing so impossible environment values are rejected before narrowing into runtime configuration

## 2. Test and probe hardening

- [x] 2.1 Tighten the PTY-backed GPS support tests so PTY setup fails fast and uses the correct declarations
- [x] 2.2 Add local cleanup to the governed Raspberry Pi live GPS probe temporary directory

## 3. Verification and closeout

- [x] 3.1 Run the GPS support/unit/component/hosted verification set for the hardening slice
- [x] 3.2 Run repository consistency and OpenSpec validation, then archive the change and sync the affected baseline surfaces
