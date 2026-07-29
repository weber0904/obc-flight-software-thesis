# Completed Roadmap Archive Through 2026-05-17

Status: historical non-normative summary.
Last reconciled against main: `75a85677` on 2026-05-17.

This file collapses the prior done-heavy roadmap notes. Use archived OpenSpec
changes, `docs/test-records/`, and `docs/baseline-reconciliation-matrix.md` for
reviewable implementation and evidence details.

## Completed Capability Lines

- Mode model and safety: `mode-model-v2`, `mode-safety-policy-v1`,
  `payload-ttc-mode-entry-v1`, `mode-soc-admission-and-exit-v1`,
  `ttc-pass-window-mode-v1`.
- COMM and ground paths: `comm-dual-link-sim-foundation`,
  `sband-tcp-ground-link-v1`, `uhf-uart-backup-link-v1`,
  `ccsds-ground-link-spike`, `ccsds-sband-hosted-adoption-v1`,
  `ground-link-boundary-split-v1`, `ground-link-observation-ownership-v1`,
  `uhf-ccsds-adoption-v1`.
- Runtime maintainability and active target alignment:
  `hosted-obc-runtime-maintainability-v1`,
  `active-topccsds-target-alignment-v1`,
  `active-probe-cleanup-hardening-v1`,
  `probe-cleanup-touch-on-use-policy`.
- Command authority and command-session security foundations:
  `link-authority-vocabulary-v1`, `command-ingress-authority-v1`,
  `command-ingress-source-index-v1`,
  `command-session-sequence-foundation-v1`,
  `command-envelope-metadata-v1`, `command-session-sequence-v1`,
  `command-session-lifecycle-v1`, `command-auth-envelope-v1`,
  `persistent-command-freshness-v1`.
- Boot, recovery, watchdog, and FDIR:
  `boot-trust-chain-v1`, `fdir-subsystem-timeout-v1`, `watchdog-v1`,
  `multi-subsystem-fdir-v1`, `persistent-fault-ring-v1`,
  `target-recovery-closure-v1`.
- Data products, state history, and observability:
  `hk-data-product-alignment-v2`, `canonical-state-hk-fallback-v1`,
  `onboard-state-data-fdp-parity-v1`.
- Interfaces and governance:
  `interfaces-contract-v1`, hosted probe governance updates, and the
  architecture-review follow-up dispositions now folded into current docs.

## Historical Boundaries To Preserve

- Old stock `ComFprime`, S-band TCP, UHF UART, and CCSDS spike records remain
  useful history but are not current maintained deployment surfaces.
- Old HK ring/runtime fallback records remain useful history but are not the
  active mission-history path.
- Old architecture-review packages remain under `docs/architecture-review/archive/`
  and reporting package history remains under `docs/reporting/archive/`.

## Superseded Planning Labels

- `legacy-top-retirement-v1` is superseded by
  `legacy-top-retirement-docs-reorg-v1`, which hard-retires the old topology and
  reorganizes docs.
- `payload-manager-v1` is superseded as a planning label by
  `payload-ops-contract-v1`.
- `topology-dedup-and-state-canonicalization-v1` is superseded by legacy Top
  retirement plus the already-completed canonical state work.
