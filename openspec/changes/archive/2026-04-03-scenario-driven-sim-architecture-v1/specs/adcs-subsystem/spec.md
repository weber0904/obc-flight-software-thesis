## ADDED Requirements

### Requirement: Scenario-Seeded ADCS Deployment Rates
The hosted ADCS simulator SHALL accept scenario-seeded deployment-rate angular velocity inputs during scenario initialization, and the simulator SHALL preserve its own control-responsive dynamics after that initialization instead of being continuously overwritten by replay.

#### Scenario: Detumble remains control-responsive after scenario seeding
- **WHEN** the scenario bridge seeds the hosted ADCS simulator with deployment-rate angular velocity and the OBC later commands `DETUMBLE`
- **THEN** the hosted ADCS simulator SHALL converge according to its own control-response model rather than being forced back to the seeded angular rate on each replay step
