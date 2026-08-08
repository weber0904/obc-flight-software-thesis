## Context

F Prime command status semantics use the command buffer source/index and `Fw.Com.context` for status correlation. The current authority gate preserves those values, but it does not yet model multiple configured ingress sources inside the component. This change keeps source identity as topology-owned configuration and explicitly rejects attempts to infer mission source from `Fw.Com.context`.

## Design

`CommandIngressAuthority` will own a fixed-size array of `AuthorityConfig` values, one per `CommandIngressAuthorityPorts` index.

Configuration APIs:

- `clearIngressSources()` resets every ingress config to invalid/unconfigured.
- `configureIngressSource(portNum, config)` stores `config` for that exact input port.
- `configure(config)` is retained as a legacy convenience API and is exactly equivalent to `clearIngressSources(); configureIngressSource(0, config)`.

Ingress behavior:

- `seqCmdBuffIn_handler(portNum, data, context)` looks up only `m_ingressConfigs[portNum]`.
- If that config is invalid/unconfigured, the command is denied before dispatch.
- If command opcode deserialize succeeds, the synthetic status uses that opcode.
- If command opcode deserialize fails, the synthetic status uses the existing unknown opcode sentinel `0xFFFFFFFF`.
- Synthetic status returns through `seqCmdStatusOut[portNum]` with `cmdSeq == context`.
- `Fw.Com.context` is forwarded unchanged for allowed commands and returned unchanged for dispatcher or synthetic status; it is not source identity.

Event schema:

- Existing `COMMAND_AUTHORITY_REJECTED` is changed to include `ingressPort` and `linkIdentity`.
- Tests and probes must update expectations because this is dictionary-visible.

Topology:

- Default CCSDS and legacy ComFprime topologies continue wiring only index `0`.
- Runtime `--command-authority-profile` configures only index `0` via legacy `configure(config)`.
- Port `1` behavior is proven in component tests, not hosted runtime evidence.

## Risks And Boundaries

- This is a configured source-index foundation, not trusted source provenance.
- It does not prove physical UHF routing, dual simultaneous command ingress, crypto/authentication, session/sequence enforcement, or replay protection.
- Future work may wire a second routed source or introduce a command envelope. That must be a separate governed change with its own evidence.
