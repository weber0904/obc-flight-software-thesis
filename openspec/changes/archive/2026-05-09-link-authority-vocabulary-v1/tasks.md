## 1. OpenSpec Artifacts

- [x] 1.1 Create proposal, design, core-system-contracts delta spec, verification-evidence delta spec, and tasks for `link-authority-vocabulary-v1`.
- [x] 1.2 Validate the change with `openspec validate link-authority-vocabulary-v1`.

## 2. Vocabulary And Policy Source

- [x] 2.1 Add shared authority vocabulary for link identity, configured role, command class, resource label, decision, and reason.
- [x] 2.2 Add policy source keyed by fully qualified FPP dictionary command names.
- [x] 2.3 Define the UHF backup read/status allowlist and rejection classes for current commands.
- [x] 2.4 Define `UHF + PRIMARY_AFTER_FAILOVER` vocabulary and policy-test-only behavior.

## 3. Dictionary Catalog Generation

- [x] 3.1 Add repo-owned generator that reads the default CCSDS and legacy ComFprime topology dictionaries plus the policy source.
- [x] 3.2 Generate a checked-in C++ runtime opcode catalog.
- [x] 3.3 Add tests that fail when generated catalog output drifts from current dictionaries.
- [x] 3.4 Add tests that fail when any active command is unclassified.

## 4. Verification And Docs

- [x] 4.1 Add focused policy/unit tests for S-band primary, UHF backup, dev/internal, unknown config, and future UHF primary-after-failover.
- [x] 4.2 Document that this change does not enforce runtime command authority by itself.
- [x] 4.3 Update pending docs with the vocabulary result and follow-on dependency on `command-ingress-authority-v1`.
