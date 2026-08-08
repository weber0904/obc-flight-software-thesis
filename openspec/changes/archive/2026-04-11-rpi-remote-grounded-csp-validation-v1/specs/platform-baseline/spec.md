## ADDED Requirements

### Requirement: Remote Pi-To-macOS Internal CSP Topology Is Governed
The platform baseline SHALL allow a governed topology where the Raspberry Pi target runs only the OBC process while a remote macOS development host runs the internal CSP hub plus EPS and ADCS simulator nodes, and that topology SHALL keep internal CSP settings distinct from the remote GDS ground adapter settings.

#### Scenario: Pi target uses remote macOS host for both internal CSP and GDS
- **WHEN** the repository launches the governed remote topology
- **THEN** the Pi OBC process SHALL be able to point `CSP_HUB_HOST` and `GDS_HOST` at the remote macOS host while keeping the internal CSP ports and the GDS adapter port documented as separate configuration domains
