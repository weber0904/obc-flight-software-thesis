## Why

The repository has already migrated EPS and ADCS business traffic onto libcsp, but the runtime binding layer still hard-codes the hosted ZMQHUB carrier into the main runtime implementation. Before adding a remote `Pi OBC + macOS simulators` topology or future physical-bus backends, the project needs one governed change that decouples carrier binding from the F' business-layer contracts.

## What Changes

- Refactor the libcsp runtime foundation so carrier selection is configuration-driven instead of directly embedded in `LibCspRuntime::init()`.
- Keep `ICspRuntime`, `IEpsTransport`, `IAdcsTransport`, `EpsBridge`, `AdcsBridge`, and existing F' public contracts unchanged.
- Introduce a first-class `zmqhub` carrier/backend selection while preserving the current `CSP_HUB_HOST`, `CSP_HUB_SUB_PORT`, and `CSP_HUB_PUB_PORT` environment compatibility.
- Update specs and docs so the repository states explicitly that `ZMQHUB over TCP/IP` is the current development carrier, not the future physical-bus proof.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: the internal CSP substrate becomes backend-selectable while keeping libcsp as the shared subsystem network contract.
- `core-system-contracts`: `CspBridge` runtime ownership now includes carrier/backend selection for the libcsp foundation.
- `verification-evidence`: evidence for backend/carrier changes must distinguish the active development carrier from future hardware-bus validation.

## Impact

- Affected code: `simulators/csp/`, `OBC/Components/CspBridge/`, EPS/ADCS transport setup, and scripts that pass CSP runtime configuration.
- Affected docs: README, verification docs, and the new change-level evidence.
- Affected runtime behavior: none intended for the existing hosted-local and Pi-local baselines beyond making carrier selection explicit.
