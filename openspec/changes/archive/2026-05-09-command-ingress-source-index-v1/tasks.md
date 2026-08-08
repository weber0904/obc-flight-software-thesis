## 1. OpenSpec Artifacts

- [x] 1.1 Create proposal, design, core-system-contracts delta spec, verification-evidence delta spec, and tasks for `command-ingress-source-index-v1`.
- [x] 1.2 Validate with `openspec validate command-ingress-source-index-v1`.

## 2. Component Contract

- [x] 2.1 Add `clearIngressSources()` and `configureIngressSource(portNum, config)` to `CommandIngressAuthority`.
- [x] 2.2 Preserve `configure(config)` as legacy convenience equivalent to clearing all ports and configuring port `0`.
- [x] 2.3 Evaluate each command using only the config for its input port.
- [x] 2.4 Deny unconfigured or invalid source ports with exactly one synthetic `EXECUTION_ERROR` response.
- [x] 2.5 Preserve opcode/context semantics for decoded and malformed denied commands.
- [x] 2.6 Expand rejection event/evidence with ingress port and link identity.

## 3. Tests And Evidence

- [x] 3.1 Update classic component UTs for port `0` and port `1` configured/unconfigured behavior.
- [x] 3.2 Add tests proving legacy `configure(config)` clears prior per-port mappings.
- [x] 3.3 Add tests proving `Fw.Com.context` spoofing does not change source identity.
- [x] 3.4 Update hosted authority probe for the changed event schema while keeping proof limited to port `0`.
- [x] 3.5 Record evidence and deferred boundaries.

## 4. Verification

- [x] 4.1 Run affected component tests and catalog check.
- [x] 4.2 Run focused hosted command authority probe after a fresh build.
- [x] 4.3 Run `openspec validate command-ingress-source-index-v1` and `openspec validate --specs`.
