## Overview

This change formalizes a repository-wide testing distinction that already exists in code shape:

- real F' components are classes under `OBC/Components/` that derive from `*ComponentBase`
- helper or support modules are plain classes, functions, stores, parsers, providers, and framing helpers that do not derive from `*ComponentBase`

The repository will stop treating those categories as interchangeable for L2 planning.

## Design Decisions

### Component Baseline Rule

- Every real repository component under `OBC/Components/` must have a classic F' component harness using `register_fprime_ut()`, generated `TesterBase`/`GTestBase`, and interface-level assertions.
- A plain contract test or integration test can complement that harness, but cannot replace it.

### Helper Test Rule

- Helper or support modules keep plain unit tests that exercise their narrow logic directly.
- Those tests remain valid and required, but they are inventory items under helper coverage rather than component L2 coverage.

### Checker Scope

The new checker will:

- enumerate real components by scanning for classes derived from `*ComponentBase`
- confirm the owning component `CMakeLists.txt` registers classic F' UT
- reject verification-matrix/report output that mislabels helper-only modules as components

### Workflow Integration

- The shared baseline gate will run the new checker.
- `AGENTS.md` and the repo-local closeout skill will point future implementers to this rule before they start new component work.
