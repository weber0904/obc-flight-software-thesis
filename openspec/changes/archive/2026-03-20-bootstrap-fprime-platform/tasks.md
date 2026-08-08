## 1. Bootstrap the F' project tree

- [x] 1.1 Populate the current workspace with `fprime-bootstrap project --populate --path . --tag v4.1.0`
- [x] 1.2 Inspect the generated project tree and reconcile root-level ignore or config files with the existing OpenSpec workspace

## 2. Align the generated tree with the formal baseline

- [x] 2.1 Ensure the baseline project directories needed for near-term work exist (`OBC/`, `simulators/`, `scripts/`, `docs/`, `.github/` as applicable)
- [x] 2.2 Preserve `openspec/`, `obc-dev-spec/`, and `.codex/skills/` as governed project content after bootstrap

## 3. Run bootstrap gate checks

- [x] 3.1 Run `fprime-venv/bin/fprime-util generate`
- [x] 3.2 Run `fprime-venv/bin/fprime-util build`

## 4. Capture verification evidence

- [x] 4.1 Record the bootstrap commands, expected outcomes, and summary results under `evidence/records/bootstrap-fprime-platform/`
- [x] 4.2 Review the workspace for any bootstrap-phase limitations or follow-up notes
