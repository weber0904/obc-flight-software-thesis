## MODIFIED Requirements

### Requirement: OBC-Side Serial Comm Evidence Is Historical After GPS Reallocation
Once the governed target GPS live UART path reallocates `obc.local:/dev/serial0` to GPS, the repository SHALL preserve the prior OBC-side serial comm evidence as historical evidence and SHALL NOT continue to describe it as the current active target baseline.

#### Scenario: Historical comm evidence remains reviewable without overstating current ownership
- **WHEN** reviewers inspect older OBC-side serial comm records that used `/dev/serial0`
- **THEN** those records SHALL remain available as historical proof of prior behavior, but current baseline wording SHALL state that active comm development has moved back to TCP/dev paths until later comm migration work lands
