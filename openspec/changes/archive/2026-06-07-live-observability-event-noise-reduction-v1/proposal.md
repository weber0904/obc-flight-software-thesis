## Why

The current S-band auth-gated live event stream still forwards several
scheduled success and heartbeat-style events that are useful for local debug
but too noisy for operator-facing GDS review. In practice, node-`5` post-auth
visibility is dominated by repeated `STATE_MONITOR_UPDATED`,
success `CSP_PING_RESULT`, and low-level `DpWriter.FileWritten` activity even
when no meaningful state transition occurred.

This change reduces that live event noise without weakening real anomaly
signals, changing auth policy, or redesigning telemetry transport. The goal is
to keep genuine warnings and mission-meaningful data-product completion visible
while moving low-value periodic success chatter out of the packetized ground
event surface.

## What Changes

- Make `OnboardStateMonitor.STATE_MONITOR_UPDATED` emit only when its three
  reduced-state masks change.
- Stop packetized success `CSP_PING_RESULT` events; keep only failure
  `CSP_PING_RESULT` while preserving existing link-transition signaling.
- Keep `HK_TREND_PRODUCT_WRITTEN` as the operator-facing HK product success
  event.
- Keep `DpWriter.FileWritten` defined and locally visible through text/journal,
  but suppress it from the default packetized ground event stream through a
  checked-in default event ID filter config.
- Leave `RateGroupCycleSlip` unchanged.

## Capabilities

### Modified Capabilities

- `onboard-data-products-and-live-beacon`: reduced-state update events become
  mask-change-only and HK product success remains the operator-facing product
  success event.
- `comm-subsystem`: periodic CSP ping success stops being part of the current
  ground live event surface; failure and transition semantics remain.
- `interface-contract-index`: current event-surface semantics are updated for
  reduced-state, CSP ping, and data-product writer visibility.

## Impact

- Affected code:
  - `OBC/Components/OnboardStateMonitor`
  - `OBC/Components/CspBridge`
  - `lib/fprime/Svc/EventManager`
  - repo config and focused unit tests
- Affected docs:
  - `docs/interfaces.md`
  - `docs/verification-debugging-lessons.md`
- Non-goals:
  - no telemetry packet format changes
  - no auth/session policy changes
  - no new event filter command workflow
  - no `RateGroupCycleSlip` suppression
