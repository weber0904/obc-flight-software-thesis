## ADDED Requirements

### Requirement: Gateway Process Remains A Single-Link Raw Relay

`ground_ttc_gateway` SHALL remain a raw relay process with one northbound GDS
connection and one active southbound link per process on the current baseline.

#### Scenario: One gateway process does not imply dual-link multiplexing
- **WHEN** current hosted or target/lab gateway behavior is described
- **THEN** one `ground_ttc_gateway` process SHALL be described as relaying
  either one serial southbound link or one TCP southbound link
- **AND** the repository SHALL NOT claim that one current gateway process is a
  simultaneous S-band/UHF multiplexer

### Requirement: Gateway Is Not The COMM Authority Or Reliable-Transfer Owner

`ground_ttc_gateway` SHALL remain outside command authority ownership, session
lifecycle ownership, link-role policy ownership, and reliable-transfer
ownership on the current baseline.

#### Scenario: Gateway boundaries remain explicit
- **WHEN** gateway-backed COMM paths are documented
- **THEN** `CommandIngressAuthority` and `CommController` SHALL remain the
  bounded OBC-side owners for command-session and link-role policy truth
- **AND** `ground_ttc_gateway` SHALL NOT be described as the authority owner,
  reliable-transfer engine, or dual-link policy orchestrator

### Requirement: Gateway Retry Helpers Stay Whole-Command Only

Any bounded retry behavior documented on current gateway-backed paths SHALL
remain ground-side whole-command helper behavior and SHALL NOT be treated as
packet retry or reliable-transfer logic inside `ground_ttc_gateway`.

#### Scenario: Gateway file or command retries do not expand gateway semantics
- **WHEN** a current gateway-backed proof repeats a command or downlink trigger
- **THEN** the repository SHALL describe that repeat as a bounded ground or
  probe resend of the same command surface
- **AND** it SHALL NOT describe `ground_ttc_gateway` as providing packet
  recovery, retransmission windows, ARQ, NACK handling, or CFDP behavior
