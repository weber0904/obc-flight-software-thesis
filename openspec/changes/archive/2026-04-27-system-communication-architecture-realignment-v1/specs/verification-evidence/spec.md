## ADDED Requirements

### Requirement: Evidence Keeps Direct GDS And Omitted-RF TT&C Separate
The verification evidence tree SHALL distinguish the stock direct `GDS -> TCP -> OBC` development path from any future omitted-RF TT&C path that traverses the comm subsystem or a ground-side gateway.

#### Scenario: Future TT&C evidence does not overwrite direct GDS meaning
- **WHEN** a later change captures evidence for a gateway-backed TT&C path
- **THEN** that evidence SHALL state that the direct TCP GDS baseline is a separate neighboring path unless it is also explicitly revalidated

### Requirement: Evidence Separates Spacecraft Internal Bus Claims From Lab Ingress Claims
The verification evidence tree SHALL distinguish lab-side omitted-RF ingress behavior from spacecraft-side internal bus behavior when the two are exercised in the same overall architecture.

#### Scenario: Lab UART ingress does not imply internal CAN FD proof
- **WHEN** a later change validates subsystem-side UART ingress plus spacecraft-side CSP traffic
- **THEN** the evidence SHALL name which part of the verdict applies to the lab ingress and which part applies to the internal spacecraft-side bus

### Requirement: Evidence Names GPS As A Separate Direct Sensor Path
Future evidence involving live GPS hardware SHALL identify GPS as a direct sensor path distinct from both the comm TT&C path and the shared internal subsystem bus.

#### Scenario: GPS evidence is not conflated with comm or shared bus work
- **WHEN** a later change captures live GPS UART evidence
- **THEN** that record SHALL identify the direct GPS path explicitly and SHALL NOT reuse comm or internal-bus evidence labels
