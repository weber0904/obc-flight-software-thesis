- [x] 1. Add formal delta specs for `comm-subsystem`, `hk-data-products`,
      `interface-contract-index`, and `verification-evidence`.
- [x] 2. Implement node-`5` reliable-transfer protocol surfaces on ports `37`
      and `38`, including `BEGIN`, `ACK_POLL`, `COMPLETE`, `CANCEL`, and
      `ABORT`.
- [x] 3. Implement the `CommController` bounded reliable-transfer helper for
      official `.fdp` whole-file requests on the default S-band baseline.
- [x] 4. Keep non-selected requests on the current stock `FileDownlink`
      baseline path.
- [x] 5. Add transfer-truth telemetry/events/status for start, progress,
      resend, retry-exhausted failure, and final result.
- [x] 6. Add helper and protocol tests for begin, progress, timeout resend,
      retry exhaustion, duplicate handling, cancel/abort, and invalid packet
      handling.
- [x] 7. Add `CommController` integration coverage for reliable owner
      admission, busy reject while active, and owner-drop/link-policy abort.
- [x] 8. Add a repository-owned hosted reliable-transfer probe for:
      happy-path success, degraded resend with final success, and bounded final
      failure.
- [x] 9. Add a repository-owned target/lab default node-`5` happy-path probe
      for the new reliable-transfer path.
- [x] 10. Update current architecture/interface/operator docs and add the
      `reliable-transfer-v1` evidence record.
- [x] 11. Run fresh local verification, `openspec validate reliable-transfer-v1`,
      and `openspec validate --specs`, then reconcile tasks/evidence.
