## MODIFIED Requirements

### Requirement: Interface Index Separates Preferred Secure Facts From Legacy Compatibility

The interface contract index SHALL let reviewers distinguish current secure
baseline facts from retained legacy compatibility fields or later cleanup
surfaces.

#### Scenario: Reviewers can see secure-baseline versus compatibility status
- **WHEN** a reviewer inspects the command-security section of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that secure auth uses `serviceId`,
  `moduleSerial`, and root-key material as the preferred baseline
- **AND** any retained `source_id`, `key_slot`, or wire `session_id` wording
  SHALL be explicitly marked compatibility-only or historical-cleanup scoped.
