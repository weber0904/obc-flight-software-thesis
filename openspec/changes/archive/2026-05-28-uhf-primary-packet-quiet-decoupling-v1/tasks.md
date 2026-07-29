- [x] 1. Create delta specs that separate UHF primary packet quiet from UHF
      beacon suppress/runtime and require separate hosted evidence/registry
      treatment.
- [x] 2. Rename packet-side runtime APIs and fields in `CommEgressMux` so they
      describe UHF-primary-driven packet quiet instead of accepted-session
      quiet.
- [x] 3. Update `CommController` runtime policy and state so packet quiet is
      driven by `currentPrimaryBand == UHF` while beacon suppress remains tied
      to accepted qualifying UHF sessions.
- [x] 4. Refresh `CommController` and `CommEgressMux` unit coverage for:
      entering UHF primary before accepted `SESSION_OPEN(seq0)`, session-owned
      beacon suppress refresh/clear behavior, file preservation, and diagnostic
      quiet non-regression.
- [x] 5. Add a dedicated hosted packet-quiet proof path separate from the
      existing hosted beacon suppress/runtime proof and record explicit
      non-claims.
- [x] 6. Update current docs/spec wording so they no longer imply packet quiet
      begins only when accepted-session-owned beacon suppress begins.
- [x] 7. Run fresh local verification, hosted proof, `openspec validate
      uhf-primary-packet-quiet-decoupling-v1`, and `openspec validate --specs`,
      then record the resulting evidence/update surfaces.
