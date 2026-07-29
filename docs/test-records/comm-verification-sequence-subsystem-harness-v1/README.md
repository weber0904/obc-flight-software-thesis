# comm-verification-sequence-subsystem-harness-v1 Evidence

Date:
- `2026-05-22`
- `2026-05-23`

OpenSpec change:
- `comm-verification-sequence-subsystem-harness-v1`

## Scope

This record covers the shared hosted sequencing helper extracted for the formal
communication verification matrix.

The helper governs:

- official sequence source generation and compilation
- same-path file ingress into `.sequence-staging`
- `SEQ_VALIDATE` as a non-reject preflight only
- `SEQ_RUN(..., WAIT)` as the execution oracle
- same-path EPS plus ADCS readback over the active ground path

This change closes the hosted `sband-sequence-subsystem` and
`uhf-primary-sequence-subsystem` matrix cells.

## Hosted S-band

Command:

```bash
COMM_VERIFICATION_ENV=hosted \
COMM_VERIFICATION_CASES=sband-sequence-subsystem \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/hosted/20260522T051636Z`

Observed result:

- `sband-sequence-subsystem`: `PASS`
- carrier: `sband-tcp`
- wrapper intent: `dedicated-governed`

Passing markers:

- sequence upload accepted on the active hosted node-`5` path
- `FileHandling.fileUplink.FileReceived`
- non-reject `SEQ_VALIDATE`
- `SEQ_RUN(..., WAIT)` completion with:
  - `EPS_GET_STATUS`
  - `ADCS_GET_ATTITUDE`
  - `CS_SequenceComplete`

## Hosted UHF Primary-After-Switch

Command:

```bash
COMM_VERIFICATION_ENV=hosted \
COMM_VERIFICATION_CASES=uhf-primary-sequence-subsystem \
bash scripts/comm_verification/matrix/run_env_matrix.sh
```

Passing artifact root:

- `$REPO_ROOT/build-artifacts/comm-verification/hosted/20260523T065523Z`

Observed result:

- `uhf-primary-sequence-subsystem`: `PASS`
- carrier: `uhf-hosted-serial-standin`
- wrapper intent: `dedicated-governed`

Passing markers:

- hosted `COMM_SET_ACTIVE(UHF)` with node-`6` primary-after-switch
- authenticated UHF `SESSION_OPEN` on ingress `1`, identity `2`, role `3`
- same-path sequence upload to `.sequence-staging/subsystem-roundtrip.bin`
- `FILE_INGRESS_START_ACCEPTED`
- `FileReceived`
- non-reject `SEQ_VALIDATE`
- `SEQ_RUN(..., WAIT)` completion with:
  - `EPS_GET_STATUS`
  - `ADCS_GET_ATTITUDE`
  - `CS_SequenceComplete`

Closure notes:

- the product fix that closed this cell was not in the hosted helper itself:
  hosted node-`6` required `FileIngressAuthority` plus `TopCcsds` wiring to
  return forwarded file-ingress buffers to the original ingress port instead of
  always routing them back to ingress `0`
- after that fix, the existing helper and packet contract were sufficient for
  hosted node-`6` to reach `FILE_INGRESS_START_ACCEPTED`, `FileReceived`,
  `SEQ_VALIDATE`, and `SEQ_RUN(..., WAIT)` on the same UHF path
- the helper still records packet-audit artifacts for node-`6`, but the branch
  no longer needs a manual fallback sender or COM queue churn to explain the
  outcome

## Interpretation

Current branch truth:

- hosted `sband-sequence-subsystem` is formally closed by the matrix-owned
  helper
- hosted `uhf-primary-sequence-subsystem` is now also formally closed
- the shared helper now proves hosted node-`5` and hosted node-`6`
  same-path sequence-subsystem round-trips under the governed matrix surface

## Follow-On Impact

The extracted helper is now reusable for target CAN and target TCP
sequence-subsystem work, and the hosted node-`6` stand-in path is no longer a
live blocker for the formal matrix.
