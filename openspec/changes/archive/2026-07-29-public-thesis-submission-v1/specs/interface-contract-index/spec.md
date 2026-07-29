## ADDED Requirements

### Requirement: Interface Index Records Public Keystore Provisioning
`docs/interfaces.md` SHALL distinguish the tracked example keystore, ignored
hosted runtime copy, packaging-time target input, installed fixed path, and
prohibition on runtime command-auth injection.

#### Scenario: Reviewer inspects command-auth configuration
- **WHEN** the public interface index describes secure command provisioning
- **THEN** it SHALL state that example keys are non-deployable
- **AND** it SHALL record `OBC_PACKAGE_KEYSTORE_PATH` as packaging-only
- **AND** it SHALL preserve the fixed installed runtime path and manifest digest

## REMOVED Requirements

### Requirement: Interface Index Records The Mission Console Companion Reference
**Reason**: The implementation handoff is a transitional planning document and
is not part of the public canonical documentation layer.

**Migration**: Move still-valid assumptions and authority reuse into
`docs/interfaces.md`, the Mission Console runbook, and the mission-console
spec before excluding the handoff.
