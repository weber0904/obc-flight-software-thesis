# Contributing

## Development Workflow

1. Create a `feature/`, `fix/`, `docs/`, or `hotfix/` branch.
2. Create an OpenSpec change for behavior, interface, verification, packaging,
   or workflow changes.
3. Implement code, tests, documentation, and evidence together.
4. Build before running a runtime probe.
5. Run the relevant focused probes and the repository gate.
6. Validate and archive the OpenSpec change.
7. Submit a pull request with the verification results.

Documentation-only corrections may use the normal branch and pull-request
workflow when they do not change a formal contract.

## Build And Test

Use the repository virtual environment for F Prime commands:

```bash
fprime-venv/bin/fprime-util generate -f
fprime-venv/bin/fprime-util build
bash scripts/run_verification_ci.sh
```

For a focused component change, run its classic F Prime
`TesterBase`/`GTestBase` test executable in addition to the full gate. Helper
logic with nontrivial behavior requires direct unit coverage.

## Runtime Probes

- Review the capability layer in
  [`docs/verification.md`](docs/verification.md).
- Select the registered path in
  [`evidence/verification-path-registry.md`](evidence/verification-path-registry.md).
- Hosted probes use isolated runtime roots and ports and clean up their own
  processes.
- Target probes prepare the target with
  `scripts/ensure_target_comm_lab_baseline.sh`, prepare the ground side with
  `scripts/ensure_ground_dual_gds_baseline.sh`, and then run the functional
  probe.
- Register executable scripts in `scripts/verification-manifest.json`.

## Specifications And Evidence

- Normative requirements belong in `openspec/specs/`.
- Formal changes belong in `openspec/changes/` and are archived when complete.
- Operator-facing behavior belongs in `docs/`.
- A reusable verification result belongs in `evidence/records/`.
- Large captures and binary artifacts are indexed by `evidence/catalog.json`.

Do not commit private keys, live credentials, host inventories, device serial
numbers, or personal data.
