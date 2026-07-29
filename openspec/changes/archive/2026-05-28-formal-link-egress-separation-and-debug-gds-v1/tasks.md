- [x] 1. Update `CommEgressMux` session-quiet behavior so active UHF
      command-session quiet suppresses live packet egress only and preserves
      official file/data-product downlink routing.
- [x] 2. Refresh focused `CommEgressMux` unit coverage so diagnostic quiet and
      session quiet remain separate, reviewable semantics.
- [x] 3. Add formal change text that freezes:
      `sband-primary`, `uhf-backup`, `uhf-primary-after-failover`, the beacon
      suppression boundary, and the official file/downlink boundary.
- [x] 4. Update current baseline narrative docs so they no longer describe
      `uhf-backup` as beacon-only or imply that UHF session quiet drops formal
      file/data-product downlink.
- [x] 5. Record the near-term ground-side operational baseline as separate
      per-band stock-GDS/gateway stacks, while keeping custom multi-band GDS
      integration as future work.
