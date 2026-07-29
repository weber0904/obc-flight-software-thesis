## Overview

This change repairs the reporting layer so the repository can accurately answer two separate questions:

1. Which real F' components have classic L2 component coverage?
2. Which helper or support modules have direct L1 logic coverage?

## Design Decisions

### Inventory Model

- Enumerate real components by scanning `OBC/Components` for classes derived from `*ComponentBase`.
- Enumerate helper/support modules from the known direct-test surfaces used by the repo.
- Report classic F' L2 coverage from `register_fprime_ut()` and generated tester harness registration.

### Matrix Shape

- Add a `Component Coverage` section keyed by real repository components.
- Add a `Helper/Support Coverage` section keyed by direct-test support modules.
- Keep capability-level coverage summary, but stop using it as a substitute for the underlying component/helper inventory.

### Remaining Gaps

- After the classic harness backfill is complete, the matrix should only list genuine unresolved gaps such as future hardware paths or future feature work.
- It should not keep historical wording that implies later true components still intentionally lack classic harnesses.
