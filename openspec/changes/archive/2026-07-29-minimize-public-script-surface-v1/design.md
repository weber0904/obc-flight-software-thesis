## Context

The candidate repository inherited a large development script tree. Although
the first publication pass removed explicitly historical source entrypoints,
the remaining tree still mixed public operations, narrow feature probes,
compatibility implementations, test helpers, and release-generation tools.
That makes the repository harder to review and obscures the primary
engineering story.

The public release must retain complete OpenSpec and evidence history while
presenting a deliberately small executable surface. The three Chapter 5
routes, Mission Console, hosted development stack, target/lab baselines, and
verification CI are the principal public workflows.

## Goals / Non-Goals

**Goals:**

- Make every shipped script belong to one named public workflow or its direct
  dependency closure.
- Keep the public catalog short enough to review from one page.
- Reject accidental reintroduction of unlisted scripts.
- Preserve formal history and evidence without preserving every executable
  used to produce it.

**Non-Goals:**

- Rewrite the F Prime deployment or component implementation.
- Delete OpenSpec history or summarized evidence.
- Re-run hardware-dependent laboratory experiments.
- Preserve compatibility names for removed public commands.

## Decisions

1. Use an explicit `scripts/public-allowlist.txt` as the publication contract.
   Prefix entries cover cohesive application or route subtrees; file entries
   cover individual root tools. This is clearer than inferring importance from
   executable mode or historical registry status.
2. Keep five workflow groups: repository setup and verification, hosted
   operation, target deployment and A/B baselines, Chapter 5 routes, and
   Mission Console/manual operations.
3. Keep only a small representative probe set: internal CSP smoke/integration,
   secure hosted command, per-band hosted operation, Mission Console,
   target secure command/failover, payload route, recovery, and watchdog.
4. Rename the two active payload implementations to their public
   `raw_preview_dual_artifact` names and remove the compatibility indirection.
5. Keep historical script names only in formal evidence or source-to-public
   provenance, not in the public script inventory.
6. Extend governance checks to scan `scripts/README.md`, validate the
   allowlist, reject `.DS_Store`, caches, redundant `.gitkeep`, and unlisted
   tracked files.

## Risks / Trade-offs

- [Removed narrow probes are no longer directly runnable] → Their evidence and
  OpenSpec history remain reviewable; future work can restore a probe from
  source history when it becomes a maintained public workflow.
- [A retained entrypoint may reference a removed helper] → Validate shell path
  references, Python imports, documentation commands, and run the repository
  verification gate after pruning.
- [The allowlist can become stale] → Make `check_public_release.py` fail on
  missing or unlisted paths.
