## Overview

This change turns the current repository truth into a checked-in reporting package that can support a 10-15 minute professor report and a stable live demo without over-claiming hardware readiness. The package is intentionally optimized for a non-domain audience first and a technical follow-up audience second.

## Design Decisions

### Truth Sources

The reporting package is grounded only in checked-in repository truth:

- `README.md`
- `AGENTS.md`
- `docs/verification-matrix.md`
- `openspec/reconciliation/baseline-reconciliation-matrix.md`
- `evidence/verification-path-registry.md`
- `openspec/specs/*`
- `OBC/Top/topology.fpp`
- `OBC/Top/instances.fpp`
- relevant `evidence/records/*`

The package does not infer completed scope from chat history or from hoped-for future hardware.

### Deliverable Shape

The package is split into four checked-in documents under `docs/reporting/project-reporting-pack-v1/`:

1. a main briefing document for the 10-15 minute talk
2. one diagram document containing the three Mermaid diagrams
3. one capability-and-status matrix
4. one live-demo runbook

This keeps the package reviewable and easy to reuse for slides.

### Diagram Strategy

The final diagrams are hand-authored Mermaid instead of raw FPP layout output.

That choice is intentional because:

- the official FPP layout tooling is useful for sanity-checking topology structure, but it is not the right abstraction level for a professor-facing slide
- the repository topology imports subtopologies, so direct layout output is not the fastest path to a clear presentation artifact
- Mermaid lets the package clearly label what is already proven, what is Raspberry Pi or hardware constrained, and what is future scope

### Audience Layers

The package separates two audience views:

- **Professor / PM view**: context diagram, completed capability matrix, simple demo story
- **Engineer follow-up view**: internal component/runtime diagram and the governed workflow/verification diagram

This avoids forcing a non-domain audience to parse class names before they understand the system.

### Demo Strategy

The primary live demo path stays on the hosted, already-governed runtime path rather than on Raspberry Pi or future hardware expansion.

The runbook therefore:

- chooses a hosted stack plus reviewed operator commands as the primary path
- cites the existing governed evidence that proves that path
- includes a fallback plan that uses already-recorded evidence if live sockets or local conditions are unstable
- keeps GPS live UART, CAN-centered expansion, dual-band integration, and scheduler/payload operations explicitly out of the primary demo

### Scope Boundaries

The reporting package must say what the repository can do now, what has already been formally validated, and what remains constrained by missing hardware or future work. It is not allowed to collapse hosted baseline success into a claim of full flight-like hardware integration.
