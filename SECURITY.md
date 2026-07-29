# Security Policy

## Security Architecture

The command path uses a challenge-response handshake, authenticated command
envelopes, service-scoped sessions, monotonic sequence checks, and
source-aware authority policy. File admission and operational actions use the
same authority model.

Boot and update packages use signed manifests, file digests, version metadata,
and rollback-aware installation state. Runtime recovery includes bounded
process restart and hardware-watchdog integration.

## Credentials

`config/security/command-auth.example.ini` contains deterministic credentials
for local simulation and CI.

Create the ignored local configuration with:

```bash
bash scripts/bootstrap_dev_config.sh
```

Raspberry Pi packaging requires a separate keystore:

```bash
OBC_PACKAGE_KEYSTORE_PATH=/absolute/path/private.ini \
  bash scripts/package_rpi_bundle.sh
```

The packaging checks reject the checked-in example. Store operational keys
outside the repository, restrict filesystem access, and rotate keys after lab
sharing or suspected exposure.

## Reporting A Vulnerability

Send security reports privately through the repository owner's GitHub profile.
Include:

- the affected commit and component;
- reproducible inputs and observed behavior;
- the applicable environment: hosted, target, radio link, or package install;
- any credentials or captures in a secure attachment rather than an issue.

Do not publish real credentials, private keys, host inventories, hardware
serial identifiers, or personal data in issues or pull requests.
