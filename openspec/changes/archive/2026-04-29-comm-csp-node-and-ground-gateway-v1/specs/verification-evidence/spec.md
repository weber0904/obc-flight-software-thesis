## ADDED Requirements

### Requirement: Gateway-Backed COMM TT&C Evidence Is Reviewable
The verification evidence tree SHALL record the commands, launcher scripts, selected transport endpoints, observed bounded traffic, and final verdict for the first gateway-backed COMM omitted-RF TT&C path.

#### Scenario: First gateway-backed COMM evidence is reviewable
- **WHEN** the first gateway-backed COMM TT&C change completes
- **THEN** reviewers SHALL be able to inspect the ground-side GDS launch path, the gateway launch path, the subsystem-side COMM node launch path, the OBC launch path, and the observed bounded `command`, `event`, and `telemetry` behavior from the repository evidence tree

## MODIFIED Requirements

### Requirement: Evidence Keeps Direct GDS And Omitted-RF TT&C Separate
The verification evidence tree SHALL distinguish the stock direct `GDS -> TCP -> OBC` development path from any future omitted-RF TT&C path that traverses the comm subsystem or a ground-side gateway.

#### Scenario: Future TT&C evidence does not overwrite direct GDS meaning
- **WHEN** a later change captures evidence for a gateway-backed TT&C path
- **THEN** that evidence SHALL state that the direct TCP GDS baseline is a separate neighboring path unless it is also explicitly revalidated

#### Scenario: Gateway-backed COMM evidence identifies reused and newly proven paths
- **WHEN** the repository records the first gateway-backed COMM TT&C evidence
- **THEN** that record SHALL say which direct GDS or external comm baselines are merely reused prerequisites
- **AND** it SHALL identify the gateway-backed COMM path itself as the newly proven verdict boundary

### Requirement: Evidence Separates Spacecraft Internal Bus Claims From Lab Ingress Claims
The verification evidence tree SHALL distinguish lab-side omitted-RF ingress behavior from spacecraft-side internal bus behavior when the two are exercised in the same overall architecture.

#### Scenario: Lab UART ingress does not imply internal CAN FD proof
- **WHEN** a later change validates subsystem-side UART ingress plus spacecraft-side CSP traffic
- **THEN** the evidence SHALL name which part of the verdict applies to the lab ingress and which part applies to the internal spacecraft-side bus

#### Scenario: Gateway-backed COMM evidence names both ingress and internal-bus segments
- **WHEN** the repository records the first gateway-backed COMM TT&C evidence
- **THEN** that record SHALL explicitly identify the lab-side serial ingress segment and the internal COMM-to-OBC CSP segment as separate parts of the overall architecture instead of collapsing them into one transport claim
