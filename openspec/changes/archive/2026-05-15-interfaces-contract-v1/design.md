## Context

The repository already has the implementation and evidence needed to answer
many interface questions, but those answers currently live in scattered places:
`CommandIngressAuthority` code and evidence for the envelope/session/auth path,
`ComCcsds` and CCSDS capture records for SCID/VCID/APID truth, topology files
for rate-group membership, and multiple roadmap/current docs for storage,
recovery, and sequencing boundaries. The architecture-review follow-up plans for
`00` and `04` are therefore still open even though most of the raw facts now
exist.

This change is intentionally documentation-first. It should not freeze unknown
target timing or MTU values, and it should not silently promote current prose
into normative product requirements.

## Goals / Non-Goals

**Goals:**

- Provide one checked-in `docs/interfaces.md` entrypoint that summarizes current
  interface truth for the active baseline.
- Make the status of each fact explicit using `verified`,
  `configured-hosted`, or `TBD`.
- Fold the hosted-versus-target timing follow-up into the same document without
  inventing target values.
- Remove stale current/planning wording that still treats completed 01/03/06
  work as pending, active, or HK-fallback-based.

**Non-Goals:**

- Do not change runtime behavior, topology wiring, or generated dictionaries.
- Do not make `docs/interfaces.md` itself a normative requirements source.
- Do not freeze target flightlike rate-group values, unknown MTU ceilings, or a
  target-wide CCSDS APID governance scheme.
- Do not rewrite archived evidence or historical test records.

## Decisions

### One current-contract index instead of split interface and timing docs

`docs/interfaces.md` will own both the general current contract index and the
rate-group timing section. Splitting timing into a second document would add
another place for drift without improving reviewability.

### Status-typed facts over prose-only summaries

Every table row or named surface in `docs/interfaces.md` will use one of three
statuses:

- `verified`: grounded in code/topology plus repo-owned evidence
- `configured-hosted`: true for the hosted/dev baseline but not claimed as
  general target truth
- `TBD`: intentionally not frozen yet

This keeps hosted CCSDS defaults and hosted timing truth useful without
misrepresenting them as target closure.

### APID/VCID stay current-hosted-proven, not target-wide governance

The current repo already uses upstream `ComCfg.Apid` assignments through
`ComCcsds` and proves them on the hosted S-band CCSDS path. The new document
will record `SCID 0x44`, `VCID 1`, and APID flows `0/1/2/3` as current
hosted-proven facts only, without implying a broader frozen mission-wide APID
allocation process.

### Stale narrative cleanup stays in the same PR

Because `docs/interfaces.md` is meant to summarize the current baseline, the PR
must also fix companion docs that still describe 01/03/06 as pending or still
talk about HK archive paths as active baseline history. Leaving those files
stale would immediately undermine the new index.

## Risks / Trade-offs

- [Risk] A non-normative index could be mistaken for a spec.
  → Mitigation: put the non-normative statement and source-priority rule at the
  top of `docs/interfaces.md`.
- [Risk] Current docs may still contain historical HK archive references.
  → Mitigation: update current architecture/roadmap/review layers in the same
  change and keep historical claims only in archived evidence context.
- [Risk] Formalizing the document could tempt the change to freeze unknown
  target values.
  → Mitigation: keep unknown MTUs and target timing fields explicitly `TBD`.

## Migration Plan

1. Create `docs/interfaces.md` with the new status-typed sections.
2. Update the current architecture-review companion docs and follow-up plan
   notes to mark 00/04 completed and 06 completed.
3. Update current architecture and roadmap layers so they no longer refer to
   branch-local 01/03/06 truth or active HK fallback wording.
4. Add the new `interface-contract-index` capability through OpenSpec archive.
5. Run docs/spec consistency checks only; no build/probe promotion unless an
   unexpected file-classification issue appears.
