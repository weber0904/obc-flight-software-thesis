## MODIFIED Requirements

### Requirement: Gateway-First Integration Precedes Custom GDS Plugin Work

The repository SHALL continue to allow the active omitted-RF TT&C baseline to
use stock `fprime-gds` plus repo-owned gateway processes before any custom
multi-band GDS communication plugin becomes required.

#### Scenario: Near-term simultaneous multi-band operations use separate stock stacks
- **WHEN** the current baseline needs simultaneous operator access to S-band
  and UHF before a custom orchestrated ground surface exists
- **THEN** the near-term governed solution MAY use separate stock
  `fprime-gds` plus `ground_ttc_gateway` stacks per band
- **AND** the repository SHALL treat that as a smaller baseline step than a new
  custom GDS communication plugin

#### Scenario: One gateway instance remains one-southbound
- **WHEN** reviewers inspect the current `ground_ttc_gateway` boundary
- **THEN** they SHALL see it described as one northbound GDS relay bound to one
  current southbound path
- **AND** they SHALL NOT treat one gateway instance as the current simultaneous
  S-band/UHF multiplexer baseline
