## MODIFIED Requirements

### Requirement: Payload Commands Are Explicit And Sequence-Consumable

The active payload contract SHALL expose explicit prepare, capture, abort,
shutdown, status, and runtime-default configuration commands, and official
sequencing SHALL consume those commands through the normal governed command
path.

#### Scenario: Capture acceptance is explicit even though final completion is deferred

- **WHEN** an accepted `PAYLOAD_CAPTURE_*` request is issued on the governed
  payload path
- **THEN** the command SHALL return success once the controller accepts the
  request and dispatches it to the payload runtime
- **AND** final capture completion SHALL be determined by payload
  state/event/tlm/readback surfaces rather than by the original command
  response

### Requirement: Payload Prepare Uses Deferred Outcome Truth While Capture Completion Is State-Driven

Payload prepare SHALL report success or failure only after the real operation
result is known, while accepted still-capture commands SHALL acknowledge
dispatch immediately and surface final completion through payload state,
event, telemetry, and readback surfaces.

#### Scenario: Capture command acknowledgement is separated from final outcome

- **WHEN** `PAYLOAD_CAPTURE_*` is accepted for execution
- **THEN** the command SHALL return success immediately
- **AND** later final success or failure, including canonical payload `.fdp`
  publication outcome, SHALL be reflected on payload state, status, and
  metadata surfaces
- **AND** later sequence or operator logic that depends on final completion
  SHALL use those surfaces instead of the original capture command response

### Requirement: Payload Results And Files Stay Reviewable

The payload contract SHALL provide bounded result readback and governed local
image storage.

#### Scenario: Payload completion remains reviewable after async command ack

- **WHEN** an operator requests payload status after an accepted capture
  request
- **THEN** the runtime SHALL provide enough state to distinguish in-progress
  capture, post-capture publication, final success, and final failure through
  bounded telemetry and/or status events
