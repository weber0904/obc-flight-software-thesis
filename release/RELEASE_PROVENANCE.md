# Release Provenance

## Thesis Release

- Release: `thesis-submission-v1`
- Public repository: `weber0904/obc-flight-software-thesis`
- Public commit: the commit referenced by the annotated release tag
- History policy: curated commits only; development Git history was not imported

## Development Source

- Repository working name: `obc-flight-software`
- Source commit: `142683f20ba46f59f894f594f2caf71dfeddf16f`
- Source date: 2026-07-29
- Maintained deployment: `OBC/TopCcsds/topology.fpp`

## Dependencies

- F Prime fork: `54f02168c676d5b61990d7a48ea9c61b9a8d0b5f`
- F Prime baseline: v4.1.0 plus project fixes and license notices
- libcsp/csp-es fork: `241b756a7fb5af1ba0967183b4a2b5843f77ebdb`
- libcsp baseline: v2.1 plus project fixes

## Verification Boundary

- Required release gate: full native build, 74 registered tests,
  `fprime-util check --all`, static governance, and selected hosted probes on
  the public release candidate
- Fresh hosted selection: per-band ground stacks, secure challenge/command,
  uplink authority, node-`5` observability, Mission Console, and Chapter 5
  Route 1/2/3; see
  `docs/test-records/public-thesis-submission-v1/README.md`
- Raspberry Pi/lab evidence: previously demonstrated at the source commits and
  dates recorded by each test record; not rerun on the public commit
- Not claimed: flight certification, RF closure, fresh target proof on the tag,
  hardware-backed key storage, or production credentials
