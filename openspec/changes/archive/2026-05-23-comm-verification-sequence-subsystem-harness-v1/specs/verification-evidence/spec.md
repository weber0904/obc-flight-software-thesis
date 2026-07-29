## ADDED Requirements

### Requirement: Matrix Sequence-Subsystem Evidence Uses Official Sequencing

Matrix-owned sequence-subsystem cells SHALL use the official sequence upload,
admission, and run path rather than an ad hoc command bundle.

#### Scenario: Hosted sequence-subsystem proof is reviewable
- **WHEN** a hosted sequence-subsystem matrix cell passes
- **THEN** the evidence SHALL show sequence upload over the active ground path,
  non-reject validation, `SEQ_RUN(..., WAIT)` success, and EPS plus ADCS
  readback returning through the same path

### Requirement: Matrix Sequence Shape Remains Stable

The first shared sequence-subsystem harness SHALL use a stable read-only
sequence shape so later carrier closures compare like-for-like behavior.

#### Scenario: Same sequence payload is reused across carriers
- **WHEN** hosted, target CAN, or target TCP sequence-subsystem cells are
  compared
- **THEN** the evidence SHALL identify the same EPS and ADCS read/status
  sequence shape unless a later governed change expands it explicitly
