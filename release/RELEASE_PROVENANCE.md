# Release Provenance

## Release Identity

| Field | Value |
|---|---|
| Tag | `thesis-submission-v1` |
| Repository | `weber0904/obc-flight-software-thesis` |
| Source commit | `142683f20ba46f59f894f594f2caf71dfeddf16f` |
| Maintained deployment | `OBC/TopCcsds/topology.fpp` |

The annotated tag identifies the exact repository commit. File-level source
objects and generated additions are recorded in
[`publication-manifest.json`](publication-manifest.json).

## Dependencies

| Dependency | Revision | Baseline |
|---|---|---|
| F Prime | `54f02168c676d5b61990d7a48ea9c61b9a8d0b5f` | v4.1.0 with project integration fixes |
| libcsp / csp-es | `241b756a7fb5af1ba0967183b4a2b5843f77ebdb` | v2.1 with project integration fixes |

Dependency licenses and local modifications are documented in
[`THIRD_PARTY_NOTICES.md`](../THIRD_PARTY_NOTICES.md).

## Verification Snapshot

The release gate covers the native deployment build, 74 registered tests,
`fprime-util check --all`, OpenSpec validation, static repository contracts,
and selected hosted end-to-end probes. The result record is
[`public-thesis-submission-v1`](../evidence/records/public-thesis-submission-v1/README.md).

Detailed target, UART, SocketCAN, subsystem, and watchdog results are indexed
by the [evidence catalog](../evidence/README.md), with commit, date, environment,
commands, and artifact digests recorded per entry.

## Evidence Asset

Raw captures and binary outputs are packaged as
`obc-flight-software-thesis-evidence-thesis-submission-v1.tar.gz`. Its SHA-256,
size, embedded manifest digest, and record mapping are stored in
[`evidence/catalog.json`](../evidence/catalog.json).
