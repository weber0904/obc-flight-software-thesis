## Context

The repository already has multiple valid but adjacent communication paths:

- a direct hosted and target-side `GDS -> TCP/IP -> OBC` path used by current F' ground integration
- an external `comm` path centered on `CommController`, `RadioController`, `UartDriver`, and governed UART/mock/transparent validation
- an internal `libcsp` subsystem path for `EPS` and `ADCS`
- a `GPS` path that is fake/replay-first by default but now also has a governed direct live-UART hardware baseline on `obc.local:/dev/serial0`

These paths are individually useful, but they do not yet form one coherent architecture story for the intended end state:

1. software-only whole-system simulation on `macOS`
2. split-host simulation on `macOS + Raspberry Pi`
3. later migration of selected development carriers to physical links such as `UART` and `CAN FD`

The immediate design problem is that future implementation work could drift unless the repository first formalizes:

- which path is development-only
- which path is intended to become the omitted-RF TT&C chain
- which subsystems are future CSP nodes
- which links are planned to use shared `CAN FD`
- which devices remain direct-to-OBC sensors

The confirmed current hardware constraints are:

- `obc.local`
  - Raspberry Pi `3B+`
  - Bookworm
  - one `MCP2518FD` single-channel SPI-to-CAN FD controller
  - one UART intended for direct OBC-attached peripherals
- `subsystem.local`
  - Raspberry Pi `3B+`
  - Bookworm Lite
  - one two-channel SPI-to-CAN FD expansion board
  - one UART intended for lab-side ingress or standalone subsystem-side serial work
- current lab ingress wiring:
  - `macOS USB-to-RS485`
  - `RS485-to-UART`
  - `subsystem.local UART`

## Goals / Non-Goals

**Goals:**

- define a coherent long-term communication architecture without discarding current validated baselines
- preserve the current direct `GDS -> TCP -> OBC` path as a development baseline
- redefine `comm` as the future spacecraft-side ground-facing subsystem rather than the end-state being the current `mock-text` path
- keep `GPS` as a direct OBC-attached UART-style sensor in the next hardware phase
- define the intended spacecraft-side subsystem direction:
  - `EPS`, `ADCS`, and `COMM` as CSP-facing subsystems
  - shared `CAN FD` as the planned physical carrier direction
- formalize the first omitted-RF TT&C strategy as a `Ground TT&C Gateway` instead of starting with a custom `fprime-gds` communication plugin
- capture the agreed near-term bus/channel allocation:
  - `subsystem.local` CAN channel group A for `EPS/ADCS`
  - `subsystem.local` CAN channel group B for `COMM`
  - `obc.local` direct UART for `GPS`
- define the follow-on implementation order so Stage 1 and Stage 2 remain available after Stage 3 begins

**Non-Goals:**

- implementing the gateway in this change
- implementing `GPS` live UART in this change
- implementing `CAN FD` carrier support in this change
- creating a custom GDS communication plugin in this change
- proving RF behavior, vendor radio control semantics, or real comm hardware behavior in this change
- defining a final flight redundancy architecture beyond the current hardware-supported bounds

## Decisions

### Decision: Keep the existing repository and evolve it

The project will continue from the current governed repository instead of restarting in a new clean repo.

Rationale:

- the current repo already contains validated F' topology, libcsp-first subsystem integration, Pi workflows, split-host topology evidence, and formal governance
- restarting would mostly recreate process and integration infrastructure rather than solve current architecture gaps

Alternative considered:

- start a new repository with a cleaner communication model  
  Rejected because the cost of recreating verification and workflow infrastructure outweighs the architectural benefit

### Decision: Separate four communication domains explicitly

The architecture is split into four distinct domains:

1. direct GDS development path
2. omitted-RF ground TT&C path through the comm subsystem
3. internal subsystem CSP path
4. direct GPS sensor path

Rationale:

- current confusion largely comes from treating adjacent paths as if they were one continuous chain
- future evidence and implementation need stable path names and boundaries

Alternative considered:

- keep one broad “communications” narrative and refine during implementation  
  Rejected because that is the pattern that already produced ambiguity

### Decision: Keep direct `GDS -> TCP -> OBC` as the development baseline

The current direct TCP path to GDS remains the primary development and regression baseline even after future omitted-RF TT&C work begins.

Rationale:

- it is already proven and useful
- it is the lowest-risk path for software-only development, command iteration, and CI-friendly scenarios
- future TT&C work should add a second path, not invalidate the first

Alternative considered:

- replace the direct TCP path once the gateway exists  
  Rejected because software-only and developer-facing bring-up would become harder and more fragile

### Decision: Introduce a `Ground TT&C Gateway` before a custom GDS plugin

The first omitted-RF TT&C implementation will use a ground-side gateway between ground tooling and the lab-side comm ingress.

Rationale:

- F' supports custom communication and framing plugins, but using a gateway first keeps the current direct GDS baseline untouched
- the gateway approach allows incremental validation of uplink and downlink behavior before committing to GDS plugin packaging and maintenance

Alternative considered:

- start with a custom `fprime-gds` communication plugin and framing plugin  
  Deferred because it increases implementation risk and couples early TT&C validation directly to GDS internals

### Decision: `comm` becomes a future CSP-facing subsystem

The repository will treat `COMM` as a future CSP-facing subsystem participating in the spacecraft-side subsystem architecture, rather than leaving it as a standalone controller-only path forever.

Rationale:

- this is closer to the intended long-term system story where ground-originated traffic reaches the spacecraft through a dedicated comm subsystem before OBC consumption
- it aligns better with common smallsat transceiver architectures that expose spacecraft-facing interfaces such as `CAN`, `UART`, or CSP-like data planes

Alternative considered:

- keep `comm` only as a controller-oriented mock-radio path and never integrate it into the subsystem network story  
  Rejected because it does not satisfy the intended omitted-RF TT&C chain

### Decision: `GPS` remains direct-to-OBC instead of entering the CSP baseline now

The GPS hardware path stays direct to OBC over a dedicated UART-style path, and the first governed live-UART slice has already established that baseline.

Rationale:

- the current and planned GPS hardware is UART-style and not a native CSP node
- this is closer to a realistic first hardware integration path
- forcing GPS into CSP now would add adapter complexity without a matching hardware need

Alternative considered:

- treat GPS as another CSP subsystem from the start  
  Rejected because it is not justified by the available hardware and would complicate the immediate bring-up path

### Decision: Use both subsystem-side CAN channels, with `EPS/ADCS` sharing one and `COMM` using the other

The agreed near-term hardware mapping is:

- `subsystem.local` CAN group A: `EPS` and `ADCS`
- `subsystem.local` CAN group B: `COMM`
- `obc.local` CAN: shared spacecraft-side participation point

Rationale:

- this provides stronger physical separation than collapsing all subsystem roles onto one controller
- it gives the lab three independent physical CAN controllers total:
  - one on `obc.local`
  - two on `subsystem.local`
- it better exposes some controller-level and bus-level behavior than having every subsystem process share one `canX`

Important limitation:

- `EPS` and `ADCS` sharing one subsystem-side channel still means they are not fully independent physical controllers at the same time
- this is acceptable for the next bounded validation slices, but not sufficient to claim full multi-node physical-bus realism

Alternative considered:

- run `EPS`, `ADCS`, and `COMM` all over one single subsystem-side `canX`  
  Accepted only as an early functional fallback, not the preferred bounded physical validation direction

### Decision: Do not claim dual-bus redundancy with current hardware

The repository will treat full dual-CAN redundant architecture as future work.

Rationale:

- `obc.local` has only one CAN controller
- full redundant bus validation requires at least two independent OBC-side interfaces

Alternative considered:

- model dual-bus redundancy in software using current single-CAN OBC hardware  
  Rejected as a formal architecture claim because it would overstate what the hardware can actually prove

## Risks / Trade-offs

- **[Risk] Future TT&C implementation may diverge from the gateway-first story** → Mitigation: formalize the gateway path in specs now and keep custom GDS plugin work explicitly deferred
- **[Risk] `EPS/ADCS` sharing one subsystem-side CAN channel may hide some multi-node failure or timing behavior** → Mitigation: document the limitation and avoid over-claiming physical-bus realism
- **[Risk] Direct GDS and omitted-RF TT&C paths may be confused by future evidence authors** → Mitigation: add verification-evidence and path-registry requirements that force path separation
- **[Risk] Keeping both direct TCP and future TT&C paths increases maintenance cost** → Mitigation: accept that cost because the direct TCP path is strategically useful for software-only development and CI-style regression
- **[Risk] GPS direct UART consumes scarce OBC-side serial resources** → Mitigation: reserve the OBC UART path intentionally for GPS in the next hardware phase instead of attempting to multiplex comm and GPS on the same path
- **[Risk] Ground-side gateway framing may be chosen poorly and later conflict with hardware reality** → Mitigation: keep framing/plugin strategy as an explicit design question for the later gateway implementation slice

## Migration Plan

1. Create this architecture realignment change and sync the new formal requirements
2. Treat `gps-live-uart-source-v1` as the completed first direct OBC-attached live GPS slice
3. Implement `shared-canfd-csp-bus-foundation-v1` for spacecraft-side CSP over CAN FD
4. Implement `comm-csp-node-and-ground-gateway-v1`
5. Implement `ttc-over-comm-end-to-end-v1` with bidirectional omitted-RF TT&C
6. Keep existing software-only and split-host direct-TCP GDS baselines alive throughout

Rollback strategy:

- this design-only slice does not require runtime rollback
- if later implementation slices encounter hardware limitations, the repository can continue using the existing direct GDS and ZMQHUB/CSP baselines without invalidating current evidence

## Open Questions

- What `CSP` node ID and port allocation should be reserved for the future `COMM` subsystem?
- Should the first gateway-backed omitted-RF TT&C slice carry only command/event/tlm traffic, or should file/downlink traffic be part of the first bidirectional proof?
- Should the future TT&C framing reuse stock F' framing end-to-end, or should the gateway own an additional wrapper protocol over the lab UART ingress?
- In the first `CAN FD` foundation slice, will `EPS/ADCS` share one physical subsystem-side controller while remaining distinct logical CSP nodes, or does one of them need to stay off the physical bus until more hardware exists?
- When the gateway path is mature, does the repository still want a later custom GDS communication plugin for operator convenience, or is the gateway expected to remain the long-term lab integration mechanism?
