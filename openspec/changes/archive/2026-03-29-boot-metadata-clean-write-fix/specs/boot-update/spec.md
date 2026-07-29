## MODIFIED Requirements

### Requirement: File-Backed Metadata
Boot metadata v1 SHALL remain persisted as structured files in persistent storage rather than in a database, SHALL include active, pending, last-known-good, confirmed, digest, boot-attempt, and error fields, and SHALL rewrite the persisted file cleanly when field values shrink so stale trailing bytes are not left behind. Communication and framing paths MAY continue to use CRC-32, but staged-image verification SHALL use lowercase SHA-256 hex without requiring the project to switch the global F' `Utils::Hash` backend.

#### Scenario: Rollback rewrites metadata without stale trailing bytes
- **WHEN** rollback clears a previously populated persisted field such as `staged_path`
- **THEN** the next saved `metadata-v1.txt` SHALL parse cleanly as a complete file by a fresh metadata reader and SHALL NOT retain stale bytes from the earlier longer serialization
