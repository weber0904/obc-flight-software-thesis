## MODIFIED Requirements

### Requirement: Legacy Command Envelope V1 Is Compatibility-Only

The command ingress authority gate SHALL keep legacy command envelope v1 as a
retained compatibility surface and SHALL NOT describe it as the preferred
current comm-managed command-session model once the secure-auth baseline is
available.

#### Scenario: Secure baseline remains the preferred session model
- **WHEN** current branch docs or specs summarize the command-session model
- **THEN** they SHALL present secure auth success plus secure command v2 as
  the preferred baseline
- **AND** they SHALL scope legacy command envelope v1 and wire
  `SESSION_OPEN(seq0)` to retained compatibility or explicit historical
  cleanup surfaces only.

### Requirement: Shared Keystore Legacy Tuples Stay Compatibility-Scoped

The tracked keystore contract SHALL describe `module_serial` plus per-service
key material as the preferred secure-auth baseline. Current repo-tracked
assets SHALL NOT require legacy `source_id` or `key_slot` tuples for secure
runtime provisioning, and those fields SHALL NOT be described as preferred
secure-baseline wire or operator semantics.

#### Scenario: Compatibility plumbing is documented honestly
- **WHEN** current docs or specs describe the tracked keystore asset
- **THEN** they SHALL identify `module_serial` plus per-service root key
  material as the secure-auth baseline contract
- **AND** any retained legacy `source_id` / `key_slot` wording SHALL be called
  out as historical compatibility-only proof plumbing rather than part of the
  current tracked asset.
