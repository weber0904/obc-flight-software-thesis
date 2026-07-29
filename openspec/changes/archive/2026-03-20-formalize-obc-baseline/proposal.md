## Why

The current repo started with 13 narrative draft documents, but it did not yet have a formal main-spec baseline, a Git repository, or an OpenSpec change lifecycle anchored in the actual CLI. This change consolidates those drafts into a stable v1 specification baseline so later implementation work can proceed capability by capability without duplicating or drifting requirements.

## What Changes

- Initialize the workspace as a Git repository and OpenSpec project, including project-local Codex/OpenSpec skills.
- Reorganize the narrative docs from 13 legacy drafts into 9 source documents with explicit source mapping and ownership boundaries.
- Archive the legacy drafts under `obc-dev-spec/archive/legacy-v0/` and replace dead links with the new document map.
- Create 9 main OpenSpec capabilities that mirror the reorganized narrative documents.
- Normalize cross-cutting decisions that were previously implicit or ambiguous, including:
  - `BOOT_CONFIRM` default timeout = 60 seconds, configurable within 30-180 seconds
  - boot metadata v1 uses file-backed persistent storage, not a database
  - ADCS acceptance thresholds are mission constants with defaults of `< 0.05 rad/s` detumble norm and `< 5 deg` pointing error
- Define the formal OpenSpec lifecycle around `openspec new change`, `openspec status`, `openspec instructions`, `openspec validate`, and `openspec archive`.
- Seed the follow-on implementation change queue that will drive the actual F', simulator, comms, and verification work.

## Capabilities

### New Capabilities
- `platform-baseline`: Platform modes, bootstrap path, directory baseline, and integration startup order.
- `core-system-contracts`: Shared types, base IDs, CSP node identities, and core system contracts.
- `resource-storage`: Memory, buffer, storage, metadata, monitoring, and constrained validation rules.
- `eps-subsystem`: EPS simulator scope, `EpsBridge`, and EPS public contracts.
- `adcs-subsystem`: ADCS simulator scope, `AdcsBridge`, and ADCS public contracts.
- `comm-subsystem`: External comms layering, transport switching, and comms public contracts.
- `boot-update`: Boot manager, A/B update data path, confirm, and rollback policy.
- `verification-evidence`: Test layers, evidence expectations, and manual record rules.
- `delivery-workflow`: Git/PR/CI policy, OpenSpec lifecycle, and change queue governance.

### Modified Capabilities

None.

## Impact

- Affected files: `obc-dev-spec/`, `openspec/changes/formalize-obc-baseline/`, `openspec/specs/`, `.codex/skills/`, `.gitignore`
- Affected systems: documentation governance, OpenSpec workflow, future implementation sequencing
- Dependencies: OpenSpec CLI, project-local Codex/OpenSpec skills, later `fprime-bootstrap` adoption in a follow-on change
