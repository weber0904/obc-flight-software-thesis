# Integrity And Hashing Policy

Status: current support note for integrity policy.
Last reviewed against current boot/update and comm baseline on 2026-05-19.

This note records the first-version integrity split used by the repository.

## Decision

- Communication and framing paths use CRC-32 for accidental corruption detection.
- Boot and update staged-image verification uses SHA-256 rendered as lowercase hex.
- The first version does not add signature-based authenticity checks.

## Why The Split Exists

CRC-32 is a good fit for transport-oriented paths because it is lightweight, fast, and already aligned with the project stack used for framing and comms validation.

SHA-256 is a better fit for boot and update image validation because the staged image is a file artifact rather than a short frame. The update path needs a stronger content digest than CRC-32, even though the first version is not yet trying to provide secure-authenticity guarantees with signatures.

## Current Repository Implications

- `BOOT_PREPARE_UPDATE` expects a 64-character lowercase SHA-256 hex digest.
- The project overrides `FW_CMD_STRING_MAX_SIZE` to `64` through the repo-local `config/FpConstants.fpp` override rather than editing the `lib/fprime` framework defaults.
- `BootMetadataStore` computes SHA-256 locally for staged-image verification instead of switching the global F' `Utils::Hash` backend away from CRC-32.

## Non-Goals

- This policy does not claim tamper-proof software update authenticity.
- This policy does not introduce signatures, HMAC, or secure-boot chain ownership.
- Communication-layer CRC-32 is not treated as a substitute for staged-image SHA-256 verification.
