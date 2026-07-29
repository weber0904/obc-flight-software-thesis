## Context

Hosted changes `challenge-handshake-secure-command-v1` and
`uplink-authority-and-key-hardening-v1` proved challenge/response secure auth,
secure command v2, fail-closed unknown-uplink behavior, and keystore-backed
staged-file authority on hosted paths. They did not prove target deployment
behavior.

The target proof must reuse existing target/lab workflows:

- registry entry `59` for default target node-`5` S-band operational truth
- registry entry `69` for physical node-`6` UHF backup plus explicit
  `uhf-primary-after-failover` truth
- the existing `run_target_can_matrix_probe.py` harness and repo-owned
  sync/bootstrap/install/runbook procedures

## Decisions

1. The new proof mode lives inside `run_target_can_matrix_probe.py` as
   `--mode secure-auth-proof`. This keeps target preflight, gateway launch,
   service cleanup, SocketCAN assumptions, and journal handling on the
   existing path.
2. `scripts/run_target_secure_auth_proof.sh` is only a wrapper that selects
   the existing target harness mode and records an operator-friendly proof
   root.
3. The proof loads `config/security/command-auth.ini` through the maintained
   secure-link helper and validates the installed release's bundled copy. It
   does not add `COMMAND_AUTH_*` or `--command-auth-*` runtime injection.
4. Target verdicts are journal-first where command dispatch or authority
   behavior matters, with gateway captures retained as transport evidence.
5. UHF scope remains bounded. The proof validates `uhf-backup` acceptance and
   denial behavior, then explicit switch and re-auth for
   `uhf-primary-after-failover` command acceptance. It does not claim UHF
   primary staged-upload success.

## Evidence Contract

The proof root records:

- preflight/provenance JSON for installed release, `current` symlink,
  systemd working directory, bundled keystore SHA, manifest SHA, service
  identity, forbidden command-auth environment absence, and forbidden
  command-auth CLI absence
- S-band and UHF gateway capture files
- target journal snapshots before and after the proof
- checkpoint JSONL for each proof gate
- summary JSON with pass/fail markers and residual non-claims
- cleanup status

## Failure Classification

Target failures are classified in this order before changing product logic:

1. provenance mismatch: wrong installed release, stale `current` symlink,
   wrong service working directory, wrong bundled keystore, forbidden env or
   CLI injection
2. environment mismatch: stale local listeners, stale remote services, CAN or
   serial setup, wrong service identity or profile
3. probe-oracle issue: capture path, journal window, timeout, or helper mismatch
4. product defect: only after the proof path and oracle are validated

## Open Questions

None. The selected UHF claim is bounded target secure-auth behavior and backup
denial plus explicit-switch re-auth, not UHF primary staged-upload success.
