## MODIFIED Requirements

### Requirement: Authority Evidence Is Bounded And Observable

The command-security baseline SHALL continue to keep command, file, and unknown
uplink authority claims explicit instead of silently inferring repo-wide link
closure from command-only enforcement.

#### Scenario: Comm-managed authority scope now extends beyond command packets in a bounded way
- **WHEN** the current hosted secure-auth baseline is documented or cited
- **THEN** it SHALL be valid to claim command authority over routed
  `Fw.Com` command packets, handshake-only authority over comm-managed unknown
  uplink on APID `0x00FE`, and staged file-uplink authority over the active
  `.sequence-staging/<leaf>` file path
- **AND** it SHALL still NOT claim generic arbitrary file-uplink authority,
  repo-wide unknown-packet authority, full link authority, encryption, target
  proof, or hardware-backed security.

### Requirement: Comm-Managed Auth Material Uses A Shared Tracked Keystore Contract

The active hosted secure-auth baseline SHALL source comm-managed root key
material and retained comm-managed legacy v1 auth defaults from one tracked
keystore asset rather than from runtime-injected key bytes.

#### Scenario: Secure auth and retained legacy v1 share one tracked keystore
- **WHEN** the hosted baseline starts comm-managed S-band or UHF auth
- **THEN** `SecureLinkAuthorizer` SHALL load the secure-auth root key and module
  serial from the tracked keystore contract
- **AND** retained comm-managed legacy v1 auth configuration SHALL load the
  matching `source_id`, `key_slot`, and key bytes from that same contract.

#### Scenario: Hosted runtime does not accept command-auth key injection
- **WHEN** an operator or script starts the hosted OBC runtime on the active
  baseline
- **THEN** the runtime SHALL NOT require or accept `--command-auth`,
  `--command-auth-source-id`, `--command-auth-key-slot`, or
  `--command-auth-key-hex` for comm-managed auth defaults
- **AND** repo-owned helper tooling SHALL read the tracked keystore contract
  instead.
