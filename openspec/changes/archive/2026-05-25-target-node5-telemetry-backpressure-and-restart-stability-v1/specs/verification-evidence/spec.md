## ADDED Requirements

### Requirement: Service-Managed Timing Blocker Evidence Stays Separate And Reviewable

The repository SHALL record narrower product-side timing blockers under
dedicated evidence instead of folding them back into broad timing `TBD`
wording.

#### Scenario: Target-side timing blocker is captured as prerequisite truth

- **WHEN** current Raspberry Pi `obc-comm-csp-stack.service` timing closure is
  blocked by queue backpressure, restart-path instability, or an equivalent
  narrower product-side behavior
- **THEN** the evidence SHALL record that blocker under a dedicated change-level
  evidence root
- **AND** it SHALL identify whether the blocker concerns telemetry backpressure,
  restart stability, or another named prerequisite class
- **AND** it SHALL name the affected workload or restart phase
- **AND** it SHALL state explicitly that final service-managed timing ceilings
  remain pending until that blocker is removed or further narrowed
