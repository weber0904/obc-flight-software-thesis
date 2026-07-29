## Context

The maintained `TopCcsds` baseline already has the secure-auth foundation, the
node-`5` S-band default path, the bounded node-`6` UHF quiet path, and a large
set of component-owned `GET_*` or read/status commands that already emit
bounded summary events/telemetry. What it does not yet have is explicit
runtime governance for S-band live packet observability:

- `CdhCore.events.PktSend` and `CdhCore.tlmSend.PktSend` both feed
  `CommEgressMux`
- `CommEgressMux` currently knows only two live packet classes: `events` and
  `telemetry`
- when S-band is the current primary telemetry link, the whole live packet
  stream routes to node `5`
- the only existing packet quiet policy is the UHF-primary quiet mechanism

That leaves current node-`5` S-band startup behavior broader than the intended
secure baseline. The repo direction is already narrower: quiet-by-default
startup, auth-gated live observability, always-on low-rate critical surfaces,
and bounded `GET_*`-driven summary readback.

## Design

### Tier model

This change formalizes three operational tiers without inventing new packet
families:

1. always-on critical
   - UHF live beacon
   - command/auth closure plumbing
2. auth-gated live
   - current packetized S-band live `event/tlm`
3. `GET_*`-driven summary
   - existing component-owned bounded read/status commands and their summary
     events/telemetry

The key design choice is to keep tier `3` component-owned. This change does
not add a new summary router, a new command family, or a generic telemetry
schema redesign. It only makes the current live packet tier explicit and
auth-gated.

### Policy ownership

`CommController` remains the policy owner because it already owns:

- current primary-band policy
- role invalidation and reconfiguration effects
- runtime session observer callbacks from `CommandIngressAuthority`
- current UHF packet-quiet and beacon-suppress adjacency

This change adds an S-band live-observability runtime state to
`CommController`. The state is derived from:

- current primary telemetry band
- whether a comm-managed S-band authenticated session is active
- the active session owner metadata needed for review
- diagnostic quiet state exposed by `CommEgressMux`

The runtime state is intentionally narrow:

- no new timeout
- no new authority model
- no independent "observability lease"
- no attempt to keep S-band live visibility open after secure session revoke or
  ingress-role reconfiguration

### Enforcement point

`CommEgressMux` remains the packet enforcement point because it is already the
only current owner of packetized live `event/tlm` routing. This change extends
it from:

- UHF primary packet quiet only

to:

- reusable band-scoped live-packet enable/disable state
- with current activation used only for S-band

The concrete v1 behavior is:

- S-band live packet egress is suppressed until `CommController` enables it
- accepted S-band secure auth enables S-band live packet egress
- revoke, reconfigure, or restart disables it again
- diagnostic quiet still overrides all packet egress
- UHF primary packet quiet remains unchanged and still suppresses UHF packet
  chatter independently from the new S-band gate
- file/downlink routing remains unchanged

This stays compatible with the current packet-class model because the repo
still routes only:

- live event packets
- live telemetry packets

through `CommEgressMux`.

### Runtime observability

The new COMM-owned observability state must be reviewable without adding a new
telemetry family. The runtime surface will therefore expose:

- whether S-band live observability is currently open
- why it is open or closed
- which S-band session currently owns the live-observability gate
- S-band routed/suppressed packet counters in `CommEgressMux`

The reason surface is policy-facing rather than transport-facing. It is meant
to answer:

- waiting for auth?
- open because authenticated session active?
- closed because S-band no longer primary?
- closed because diagnostic quiet overrides packet egress?

### Maintained proof dependencies

The audit result for maintained S-band consumers is:

- some current maintained or adjacent node-`5` records do depend on live
  `event/tlm` visibility
- those dependencies are compatible with auth-gated live observability because
  they can and should run inside an accepted authenticated session
- no current maintained path was found that requires broad pre-auth S-band
  chatter as baseline truth

Examples that stay valid after this change:

- node-`5` secure-auth control proof
- timing/backpressure or mode/payload adjuncts that already run on the current
  node-`5` authenticated command path before interpreting live visibility

Current non-example:

- the hosted official sequencing/system-resources wrapper remains supplemental
  historical evidence after `legacy-command-envelope-retirement-v1` and still
  depends on retired legacy command-envelope tuple material, so this change
  records that status instead of silently treating the wrapper as a maintained
  secure-baseline gate

### Probe strategy

The new probes should prove governance, not just command reachability.

Hosted node-`5` proof:

- verify pre-auth absence of live packet visibility on the maintained S-band
  path
- authenticate on S-band
- verify representative live `event/channel` visibility opens after auth
- exercise bounded `GET_*` summary readback, proving that summary visibility is
  still available without redefining it as broad background chatter
- revoke or invalidate the session and prove live packet visibility closes

Target node-`5` proof:

- reuse the maintained service-managed target S-band path
- prove the same quiet/open/readback/close sequence
- keep the control oracle tied to the secure-auth path, not to ambient chatter

Regressions:

- rerun the target secure-auth control proof or equivalent node-`5` secure
  command preflight because the new policy must not perturb the secure baseline
- record the current hosted official sequencing/system-resources wrapper status
  as supplemental historical evidence rather than as a required secure-baseline
  closeout gate

## Risks And Mitigations

- Risk: the change accidentally suppresses command closure instead of only live
  packet `event/tlm`.
  Mitigation: keep command/auth plumbing outside `CommEgressMux` packet gating
  and add pre-auth secure-command-oriented proof steps.

- Risk: the change silently redefines UHF behavior while adding shared hooks.
  Mitigation: default the new reusable gate state so UHF behavior remains
  unchanged in v1 and keep UHF proof regressions focused on non-regression.

- Risk: `GET_*` summaries get described as a shadow command plane.
  Mitigation: keep summary logic component-owned, bounded, and documented as an
  observability tier rather than as a new plane.

- Risk: some node-`5` proof was depending on pre-auth chatter implicitly.
  Mitigation: new probes explicitly prove pre-auth quiet and require maintained
  consumers to run after accepted secure auth.
