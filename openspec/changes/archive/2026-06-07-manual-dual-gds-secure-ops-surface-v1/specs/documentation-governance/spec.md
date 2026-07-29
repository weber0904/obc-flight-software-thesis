## ADDED Requirements

### Requirement: Manual Operator Surface Has Current Runbooks And Indexed Routing

The repository SHALL document the maintained manual dual-GDS operator family as
current operator guidance with hosted and target runbooks plus indexed routing
from current repo entrypoints.

#### Scenario: Operators can discover the manual surface without reading probe code
- **WHEN** a reader starts from `README.md`, `docs/README.md`, or
  `scripts/README.md`
- **THEN** those current entrypoints SHALL route the reader to the hosted and
  target manual dual-GDS runbooks and the `scripts/manual_ops/` subtree
- **AND** the runbooks SHALL cover stack startup, manifest reading, auth,
  secure command send, governed staged upload, `SEQ_*`, re-auth, and cleanup
  without requiring probe-code archaeology.
