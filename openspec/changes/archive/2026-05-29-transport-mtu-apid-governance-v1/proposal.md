## Why

The active baseline still leaves transport payload ceilings and APID allocation
governance partially open in `docs/interfaces.md`, even though the current
repository already contains enough checked-in transport constants, serializer
structure, and path-scoped evidence to freeze a narrower current truth.

That gap is now a review and implementation risk. Later target-side COMM work
should not have to infer payload budgets from a hosted `1024`-byte CCSDS frame,
from the bounded `160`-byte reliable-transfer helper segment size, or from
whatever APID values happen to appear in current code without explicit
governance wording.

## What Changes

- Freeze the current path-specific transport ceilings needed by the active
  baseline:
  - `sband-primary` command ingress
  - `uhf-backup` command ingress
  - `uhf-primary-after-failover` official file/downlink
- Record the exact budget components behind those ceilings and separate:
  - hosted/configured framing facts
  - current repo-local governed transport ceilings
  - residual non-claims
- Freeze the current APID allocation / reservation policy from `ComCfg.Apid`
  while explicitly distinguishing active path-proven flows from reserved
  current-code values.
- Add a small repo-owned checker that derives the current ceiling values and
  APID map from checked-in source/constants and verifies `docs/interfaces.md`
  stays aligned.
- Add evidence that clearly separates numeric derivation proof from reused
  path-scope evidence.
- Audit `.codex/skills/change-closeout/SKILL.md` for recurring current-doc
  drift ambiguity and leave it unchanged unless a concrete ambiguity is found.

## Capabilities

### Modified Capabilities

- `comm-subsystem`: adds explicit current transport-ceiling and APID-governance
  requirements without reopening broader transport redesign.
- `interface-contract-index`: requires `docs/interfaces.md` to show derivation
  class, path scope, APID governance, and reliable-transfer non-generalization.
- `verification-evidence`: requires transport-governance evidence to separate
  numeric derivation, path-scope reuse, and residuals.

## Impact

- Current docs and main specs become reviewable in one pass for transport
  ceilings and APID governance.
- No runtime transport redesign is introduced in this change.
- Future target-side COMM work gets a governed payload and APID baseline
  instead of inferring one ad hoc.
