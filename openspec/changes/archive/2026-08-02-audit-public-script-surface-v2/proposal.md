## Why

The public script allowlist identifies a boundary but does not demonstrate why
each retained file is necessary. The repository needs a file-by-file audit so
reviewers can distinguish supported host and target workflows from incremental
development probes and duplicated verification scaffolding.

## What Changes

- Read and classify every tracked file under `scripts/`.
- **BREAKING**: remove development-stage, superseded, narrowly incremental, or
  duplicate verification files when a retained end-to-end workflow covers the
  same public purpose.
- Preserve the complete dependency closure for documented hosted operation,
  documented target/laboratory operation, CI/CMake, Chapter 5 routes, and
  Mission Console.
- Publish a concise file inventory describing each retained file's purpose,
  role, parent workflow, and public invocation status.
- Make repository checks reject undocumented retained files and broken
  host/target operator commands.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `public-release-profile`: require file-level justification for the public
  script surface and prefer end-to-end workflows over incremental probes.
- `documentation-governance`: require the script catalog to cover every
  retained file and identify all supported host and target operator entrypoints.

## Impact

This change affects `scripts/`, public operator documentation, script
manifests, release provenance, and repository governance checks. Product
components, interfaces, OpenSpec history, and summarized evidence remain
unchanged.
