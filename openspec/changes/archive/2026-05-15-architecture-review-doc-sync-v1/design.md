## Context

`canonical-state-hk-fallback-v1` made official `.fdp` products plus
`DpCatalog` the only active stored-history path and retired
`HousekeepingArchive`, `runtime/hk`, `hk/index.csv`, and `HK_*` command
surfaces. The historical evidence remains useful, but active specs and
operator-facing docs must not direct future work to recreate the retired path.

`ground-link-boundary-split-v1` added a provider-owned link-health view for
current availability policy. The first split intentionally covers connection,
activity freshness, error growth, and reason reporting only. RSSI/SNR should be
designed later with explicit metric ownership, units, freshness, and unavailable
semantics.

## Decisions

- Preserve archived test records and imported review text as historical source
  material.
- Update active specs, registry prose, operator/runbook prose, and roadmap
  handoff notes so future changes see HK fallback as retired.
- Do not add RSSI/SNR placeholder fields to current `CommLinkHealthView`.
- Point current target and hosted file/downlink acceptance at official `.fdp`
  / `DpCatalog` evidence, while retaining historical HK evidence labels where
  the underlying archived run actually used HK files.

## Non-Goals

- No product code, topology, command dictionary, or telemetry changes.
- No new validation path proof.
- No rewrite of archived evidence records.
- No radio-metrics design beyond deferring and naming the future boundary.
