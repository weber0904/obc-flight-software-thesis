## 1. Governance And Scope

- [x] 1.1 Add delta specs for `platform-baseline`, `core-system-contracts`, and `verification-evidence` describing the hosted libcsp foundation scope.
- [x] 1.2 Update the narrative source layer and verification-path registry so the internal CSP foundation path is tracked separately from ground and external comm paths.

## 2. libcsp Foundation Build Intake

- [x] 2.1 Pin `lib/libcsp` to a reproducible in-repo revision suitable for the integration base.
- [x] 2.2 Add hosted build support for the libcsp runtime subset, the official ZMQHUB interface, and a repo-local hub/proxy executable.
- [x] 2.3 Add CSP runtime configuration inputs and keep them separate from GDS transport configuration.

## 3. CspBridge Runtime Ownership

- [x] 3.1 Add a small runtime facade library around libcsp init, interface binding, routing, ping, raw send, and metrics snapshot.
- [x] 3.2 Rework `CspBridge` to use the real runtime facade while preserving the public `CSP_*` F' contract.
- [x] 3.3 Update startup code so the hosted OBC runtime initializes real libcsp node `1` during launch.

## 4. Hosted Scripts And Verification

- [x] 4.1 Update hosted stack scripts to start the CSP hub/proxy and keep GDS configuration distinct from internal CSP configuration.
- [x] 4.2 Update `CspBridge` automated coverage and add a focused hosted CSP foundation smoke.
- [x] 4.3 Capture governed evidence for the hosted internal CSP foundation path.

## 5. Validation

- [x] 5.1 Run the updated `CspBridge` tests.
- [x] 5.2 Run the hosted CSP foundation smoke.
- [x] 5.3 Run `bash scripts/run_verification_ci.sh <artifact-dir>`.
- [x] 5.4 Run `openspec validate internal-csp-foundation-v1` and `openspec validate --specs`.
