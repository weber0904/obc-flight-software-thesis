## 1. Establish The Formal Owner Boundary

- [x] 1.1 Add the new `csp-runtime-owner-component-v1` change artifacts and owner component skeleton.
- [x] 1.2 Instantiate `CspRuntimeOwner` in the deployed topology and route deployed CSP clients through explicit owner injection instead of direct `defaultRuntime()` binding.

## 2. Retarget Existing Deployed Clients

- [x] 2.1 Rebind `CspBridge`, `GroundLinkDriver`, and `CommReliableTransfer` to topology-injected runtime ownership.
- [x] 2.2 Rebind `EpsBridge` and `AdcsBridge` owned CSP transports to the runtime owner while preserving their current public contracts.

## 3. Verify And Prepare The Async Follow-On

- [x] 3.1 Run local generate/build coverage for the owner-injected topology and refresh any affected tests.
- [x] 3.2 Record the staged owner-refactor evidence and leave the remaining RG1 async/coalesced polling conversion explicit as the next implementation step.

## 4. Remove Remaining RG1 Blocking Poll Paths

- [x] 4.1 Convert COMM periodic subsystem health probing to owner-mediated async submit plus cached freshness evaluation.
- [x] 4.2 Convert scheduled EPS and ADCS bridge polling to owner-mediated async/coalesced request flow without regressing public cache and poll-health semantics.
