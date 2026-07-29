# Security Policy

## Research Prototype Boundary

This repository is an academic CubeSat flight-software prototype. It is not
flight-certified, does not claim hardware-backed key storage, and has not
completed RF or production security certification.

## Public Development Credentials

`config/security/command-auth.example.ini` contains public deterministic
development credentials. They are suitable only for local hosted testing.

- Run `bash scripts/bootstrap_dev_config.sh` to create the ignored local file.
- Never deploy or package the example credentials.
- Target packaging requires `OBC_PACKAGE_KEYSTORE_PATH` and rejects the public
  example.
- Rotate any lab or target credentials that have ever matched a published
  example.

Do not submit real credentials, private keys, host inventories, or hardware
serial identifiers in an issue.

## Reporting A Vulnerability

Report security concerns privately through the repository owner's GitHub
profile. Include the affected commit, reproduction boundary, and whether the
issue applies to hosted simulation, target packaging, or an installed target.
