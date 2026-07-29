# Public Evidence Catalog

Test-record summaries remain under `docs/test-records/`. Raw artifact trees are
distributed in the GitHub Release asset:

`obc-flight-software-thesis-evidence-thesis-submission-v1.tar.gz`

Each affected test record contains `ARTIFACTS.json` with the archive prefix,
file count, byte count, original/sanitized digests, and release asset name.
`catalog.json` is the machine-readable global index.

Text artifacts replace personal paths, accounts, private IPs, and serial IDs
with stable role placeholders. The catalog retains both original and public
SHA-256. Binary artifacts are preserved byte-for-byte.

Hardware evidence is commit scoped. It is not fresh verification of the public
tag unless an individual record explicitly says otherwise; no record in
`thesis-submission-v1` makes that claim.

The fresh public-tree build, hosted-probe selection, publication audit, and
non-claims are summarized in
[`public-thesis-submission-v1`](../test-records/public-thesis-submission-v1/README.md).
