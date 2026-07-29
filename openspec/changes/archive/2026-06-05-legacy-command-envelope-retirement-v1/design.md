## Context

The repository already has a split reality:

- the preferred secure-command baseline is challenge-auth plus secure command
  v2 with auth-success synthesis of repo-internal opened-session state
- a historical legacy command-envelope v1 family still exists in code,
  old proof scripts, and lingering current wording

The retirement problem is therefore not "invent a new secure path". It is
"stop letting retained legacy compatibility masquerade as current baseline
truth, while documenting the exact places where supplemental replacement proof
would still be needed if those old wrappers are ever revived."

The design for this change is audit-first and dependency-first:

- if newer current evidence already covers the claim, retire the old legacy
  proof surface
- if a current claim still exists and migration is shallow, migrate it
- if a non-core maintained dependency is real and nontrivial, retire its
  current-baseline authority and record the follow-up explicitly instead of
  forcing migration

Repo-internal synthesized session-open state is intentionally preserved. The
retirement target is the wire/public legacy session model, not the internal
runtime observer contract that current secure auth already reuses.

## Retirement Matrix

| Surface / capability | Current authority source | Latest covering evidence | Active dependency kind | Retirement action |
|---|---|---|---|---|
| Hosted secure command baseline | `challenge-handshake-secure-command-v1`, `target-secure-auth-proof-v1`, `uplink-authority-and-key-hardening-v1` | same | none | `rewrite` |
| Legacy command-envelope metadata / sequence / lifecycle / freshness families | `docs/verification-path-registry.md` entry `45` and legacy citations | secure-auth baseline now covers current command-session truth; legacy records remain history only | docs-only | `rewrite` |
| Hosted UHF packet-quiet truth | old hosted packet-quiet record | `uhf-primary-secure-live-benchmark-v1` | docs-only | `rewrite` |
| Hosted UHF beacon suppress/runtime | historical hosted wrapper still keyed to legacy `SESSION_OPEN` | no dedicated secure-baseline replacement yet | probe-only | `rewrite` |
| Hosted payload dual-artifact proof | hosted wrapper under entry `71` | newer payload evidence plus governed target proof carry the active branch truth | probe-only | `delete` |
| Hosted official sequencing / `SystemResources` | current operator docs and record | no newer full-sequencing proof replaces it | probe-only | `rewrite` |
| Target timing empirical ceiling | current target timing record | no newer timing-only replacement | probe-only | `rewrite` |
| Shared keystore `source_id` / `key_slot` fields | `config/security/command-auth.ini`, keystore loaders, legacy helpers | secure-auth runtime uses `serviceId + moduleSerial + keyBytes`; tracked current keystore asset no longer needs tuple fields | config | `remove-config` |
| Repo-internal opened-session synthesis after auth | `CommandIngressAuthority` + `CommController` runtime observer contract | current secure-auth baseline | runtime | `rewrite` |

## Design

### Preferred baseline wording

After this change, current docs and specs must consistently say:

- secure auth success is the preferred comm-managed operator session boundary
- secure command v2 is the preferred current command ingress model
- repo-internal session-open/session-activity/session-revoke state is an
  implementation detail reused by the secure baseline

They must no longer say or imply:

- wire `SESSION_OPEN` is the current preferred boundary
- `source_id`, `key_slot`, or wire `session_id` are part of the preferred
  current secure operator model
- legacy proof families are the default authority for current command-session
  truth

### Legacy proof-family demotion

The legacy command proof family remains valid as archived evidence of what the
historical legacy path did:

- `command-envelope-metadata-v1`
- `command-session-sequence-v1`
- `command-session-lifecycle-v1`
- `command-auth-envelope-v1`
- `persistent-command-freshness-v1`

This change demotes them from current-baseline authority. Current registry and
main specs may still mention them, but only as historical compatibility
evidence or exact follow-up surfaces.

### `source_id` / `key_slot` rule

The change keeps a binary rule:

- remove them from current code/config/helper surfaces only if the audit proves
  no active secure runtime path, maintained proof, or required loader still
  depends on them
- otherwise keep only the minimum implementation plumbing and demote their
  semantics to compatibility-only wording

The audit in this branch now shows the removal outcome for the tracked current
asset and active secure baseline:

- `SecureLinkAuthorizer` itself is keyed by `serviceId`, `moduleSerial`, and
  root key bytes
- the active hosted and target secure-auth runtime now provisions those root
  keys directly instead of applying legacy tuples into
  `CommandIngressAuthority`
- the tracked current keystore asset now contains only `module_serial` plus
  per-service root keys
- any remaining legacy tuple use is confined to historical helpers or
  non-current proof ancestry

### Follow-up surfaces recorded, not hidden

The branch must explicitly record these non-shallow follow-up surfaces:

- hosted beacon suppress/runtime wrapper still proves the legacy boundary
- hosted official sequencing / `SystemResources` proof still depends on legacy
  command ingress
- target timing empirical ceiling proof still depends on legacy session-open
  helpers

Those surfaces are not blockers for retiring legacy as the current preferred
baseline. They should appear in the change tasks and branch docs as
supplemental proof gaps, historical registered evidence, or later cleanup
candidates rather than remaining implicit.
