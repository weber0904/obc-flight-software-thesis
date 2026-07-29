## MODIFIED Requirements

### Requirement: Internal CSP Carrier Selection Is Configuration-Driven
The platform baseline SHALL keep `libcsp` as the shared internal subsystem network contract while making the carrier or interface binding layer selectable through governed runtime configuration instead of embedding one hosted carrier directly into the runtime core.

#### Scenario: Deployed topology centralizes runtime ownership
- **WHEN** the active `OBC` deployment configures CSP-backed subsystem or COMM clients
- **THEN** the deployment SHALL inject one topology-owned runtime owner into those clients instead of relying on each client to claim direct ownership through `defaultRuntime()`
- **AND** hosted low-level probes MAY still use `defaultRuntime()` where no deployed topology owner exists

#### Scenario: Target OBC service restart re-arms governed SocketCAN bring-up
- **WHEN** `obc-comm-csp-stack.service` starts or restarts on the governed Raspberry Pi target path
- **THEN** it SHALL explicitly restart the governed `obc-lab-can.service` before launching the user-space OBC stack
- **AND** a service-managed `R2` process restart SHALL NOT rely on an earlier boot-time `can0` bring-up invocation to recover the OBC-side SocketCAN path
