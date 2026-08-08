## ADDED Requirements

### Requirement: Registry Separates Comm UART From Future GPS UART
The verification-path registry SHALL state that the existing Raspberry Pi UART/RS485 entries prove external comm behavior over the governed comm serial device and SHALL NOT be reused as evidence for GPS live UART bring-up.

#### Scenario: Reviewer checks GPS UART status
- **WHEN** a reviewer asks whether live GPS UART has been proven
- **THEN** the registry SHALL direct them to the hosted GPS fake/replay entry and the explicit serial-allocation gap rather than to the comm UART evidence
