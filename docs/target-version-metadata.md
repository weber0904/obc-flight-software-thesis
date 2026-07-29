# Target Version Metadata

Status: current support note for Raspberry Pi version sourcing.
Last reviewed against the governed target bootstrap path on 2026-05-19.

This note explains how the Raspberry Pi target build keeps correct version strings even though the synced workspace omits `.git`.

## Problem

The governed `integ-rpi` workflow copies a reduced workspace to the Raspberry Pi. That sync intentionally excludes `.git` so the target tree stays lightweight and reviewable, but F' normally derives framework and project versions from `git describe`. Without git metadata, the stock generator falls back to its built-in release string.

## Project Policy

- Hosted builds inside the full repository continue to read versions directly from git.
- Raspberry Pi builds use the governed bootstrap script to export host-derived framework and project versions before running `fprime-util generate`.
- The project overrides the F' version target at the repo layer rather than editing `lib/fprime` in place.

## Result

When `bash scripts/bootstrap_rpi_workspace.sh` runs, the Raspberry Pi build produces:

- `build-fprime-automatic-native/versions/version.json`
- dictionary metadata under `build-artifacts/Linux/OBC/dict/AppTopologyDictionary.json`

Both files carry the intended framework and project versions instead of the framework fallback `v3.5.0`.

## Scope Limits

- This mechanism preserves version metadata for the governed Raspberry Pi bootstrap path.
- The governed Raspberry Pi packaging flow may consume the generated `version.json` and dictionary metadata, but this mechanism itself only defines version sourcing.
- It does not introduce signing or authenticity verification.
- If the target is rebuilt later outside the governed bootstrap script, the build may not have the same host-derived version inputs.
