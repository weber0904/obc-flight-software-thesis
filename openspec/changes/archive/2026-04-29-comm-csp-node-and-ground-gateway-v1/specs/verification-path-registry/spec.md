## MODIFIED Requirements

### Requirement: Future Omitted-RF TT&C Path Is Registered Separately
When the repository proves a gateway-backed omitted-RF TT&C path, the verification-path registry SHALL record that path separately from both the direct `GDS -> TCP -> OBC` development path and the existing external comm mock or UART baselines.

#### Scenario: Registry keeps gateway-backed TT&C distinct from direct GDS
- **WHEN** the first gateway-backed omitted-RF TT&C path becomes formally proven
- **THEN** the registry SHALL identify it as a distinct path and SHALL keep direct GDS and current external-comm baseline entries separate

#### Scenario: Registry identifies reused baseline and new proof boundary together
- **WHEN** the first gateway-backed COMM TT&C path is registered
- **THEN** the registry SHALL name the gateway-backed `GDS -> gateway -> serial ingress -> COMM -> internal CSP -> OBC` path as the newly proven path
- **AND** it SHALL say separately whether direct `GDS -> TCP -> OBC` connectivity is only reused as a neighboring baseline or also revalidated in the same evidence

#### Scenario: Registry keeps historical or controller-oriented comm baselines separate
- **WHEN** the first gateway-backed COMM TT&C path is registered
- **THEN** the registry SHALL keep the controller-oriented mock, transparent, framed, and historical UART comm entries separate instead of treating them as interchangeable proof of the COMM CSP path
