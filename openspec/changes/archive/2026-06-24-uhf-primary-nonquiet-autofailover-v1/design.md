## Context

The current repository has three adjacent truths that now conflict with the
desired active baseline.

1. `CommController` still drives `CommEgressMux` to suppress live
   `event/tlm` packet egress whenever UHF is the current primary band, unless a
   probe-owned `DIAGNOSTIC_DISABLE_UHF_PRIMARY_PACKET_QUIET=1` override is
   applied.
2. The maintained target secure-auth and dual-link proofs still exercise UHF
   primary through explicit `COMM_SET_ACTIVE(UHF)` switching from S-band rather
   than through detector-owned fault truth and executor-owned failover.
3. Chapter 5 target Route 2 and Route 3 wrappers currently inherit those older
   assumptions, so they cannot claim the desired current route truth.

The change therefore needs both product-level wording and proof-surface
realignment. It must keep A/B/C ownership intact, avoid creating a second
failover owner, and preserve honest historical evidence rather than silently
rewriting old claims.

## Goals / Non-Goals

**Goals:**

- Make current maintained UHF primary packet behavior non-quiet by default.
- Keep beacon suppress separate and still auth-triggered on the UHF secure
  session boundary.
- Reuse existing `COMM_PRIMARY_UNAVAILABLE` detection and
  `RecoveryExecutor::performRecoveryLinkFailoverForRuntime()` actuation as the
  only current autonomous failover path.
- Add one governed helper for bounded node-`5` unavailability injection and
  new target probes that use it.
- Rebase Chapter 5 Route 2 / Route 3 target closure onto the new current truth
  and update evidence records accordingly.

**Non-Goals:**

- No new S-band-only detector or new detector-owned failover actuation path.
- No auto-restore back to nominal S-band after fault recovery in this slice.
- No RF, modem, RSSI/SNR, or broad transport redesign claim.
- No deletion of all old explicit-switch or quiet-path evidence; those records
  remain historical and superseded.

## Decisions

### 1. Remove current UHF primary quiet in product logic, not only in probes

`CommController` will stop translating UHF-primary state into
`CommEgressMux::setUhfPrimaryPacketQuietForRuntime(true)`. This changes the
default current runtime semantics rather than preserving a quiet product
baseline with a non-quiet benchmark override.

The dedicated diagnostic quiet override remains separate:

- `DIAGNOSTIC_QUIET_PACKET_EGRESS=1` still means probe-owned all-packet quiet
- UHF primary no longer implies quiet by itself

This keeps the bounded diagnostic surface available for unrelated proofs while
removing the current product dependency on `DIAGNOSTIC_DISABLE...`.

### 2. Autonomous failover truth stays detector-owned for fault, executor-owned for action

The current change will not invent a new route-level switch command or a new
COMM control owner. The maintained truth becomes:

- `CommController` detects `COMM_PRIMARY_UNAVAILABLE`
- `RecoveryExecutor` performs bounded failover, session revoke, and owner clear
- UHF primary then requires fresh secure auth before bounded command/readback
  success

The old explicit-switch proof family remains historical evidence only.

### 3. Shared-service fault injection uses a governed helper, not ad hoc probe logic

To provoke `COMM_PRIMARY_UNAVAILABLE` reproducibly on the target/lab baseline,
the repository adds a dedicated helper that:

- stops `subsystem-sband-csp.service`
- waits for configured OBC journal markers or timeout
- restores `subsystem-sband-csp.service`
- records artifacts about the unavailable window

This helper is treated as a governed shared-baseline trigger, separate from the
functional `C` proof wrapper. Route probes may call it, but they do not embed
raw shared-service stop/start logic inline.

### 4. UHF primary runtime stability and autonomous failover proofs become separate current evidence families

Two current target proof families are needed because they answer different
questions.

- `uhf-primary-nonquiet-runtime-v1` proves that once UHF primary is current and
  authenticated, bounded live readback stays observable with acceptable
  stability.
- `target-autonomous-uhf-failover-v1` proves the fault-to-failover transition,
  re-auth boundary, and bounded post-failover readback.

Chapter 5 Route 2 can then reuse the failover proof plus its TTC/ADCS stage
without mixing the acceptance criteria.

### 5. Watchdog target proof stops depending on quiet-path recovery

Route 3 target watchdog proof currently forces a probe-owned quiet diagnostic
overlay before trigger and uses journal-first acceptance around that path. The
current slice removes that quiet dependency:

- pre-trigger auth/readback stays on current secure-auth S-band truth
- watchdog reboot still uses the bounded `SET_WATCHDOG_PROBE_SUPPRESSION`
  trigger
- post-reboot closure reuses the maintained secure-auth command-path proof
  without quiet-only gating

## Risks / Trade-offs

- [Ground noise increases on UHF primary] → The new runtime benchmark sets a
  hard `>= 7/10` success gate per subcase before Route 2/3 are allowed to rely
  on the new truth.
- [Autonomous failover timing is less deterministic than explicit switch] →
  The governed unavailable-window helper records detector/failover timing and
  restores the shared service even on timeout.
- [Historical records can become misleading] → Explicitly mark quiet/manual
  switch records as historical or superseded and remove them from current
  Chapter 5 closure references.
