## Context

The active TopCcsds baseline already owns official sequence staging and
execution through FileIngressAuthority, SequenceAdmissionController,
SeqDispatcher, and CmdSequencer. Route 1 adds a reusable verification surface
over that contract: it drives a bounded payload flow whose success depends on
sequence compile/upload/validation/run, EPS SoC admission, mode state, and a
fresh payload-completion oracle. Existing integrated-route records establish
historical payload/downlink context but do not make this sequence path a
separately reviewable current claim.

The target lab has shared OBC, node-5, node-6, EPS, and ADCS services. A and B
are their canonical baseline managers; the Route 1 functional probe is C. UHF
readiness is mandatory baseline state even though the Route 1 command/data
case uses default S-band node 5.

## Goals / Non-Goals

**Goals:**

- Preserve one inspectable Route 1 sequence source for manual and automated
  use, compiled through the active dictionary and uploaded only to governed
  sequence staging.
- Make the hosted wrapper and target C probe assert the same functional chain:
  compile, governed destination, admission validation, execution, required
  SoC/mode state, and fresh payload completion.
- Keep hosted evidence separate from target provenance and target-ground
  observations, and leave target repair and shared-service lifecycle to A/B.
- Store a dedicated record and registry entry with exact evidence locations,
  command contracts, and non-claims.

**Non-Goals:**

- A mission scheduler, time-tag database, payload planner, generic payload
  throughput proof, RF/OTA closure, or a general sequence-control API.
- Replacing FileIngressAuthority, SequenceAdmissionController, or any current
  payload/mode authority.
- Treating passive logs, stale artifacts, an accepted command alone, or a
  failed sequence as a Route 1 PASS.

## Decisions

### Reuse the current official sequence contract

Route 1 uses the checked-in sequence source, the active compiler, governed
.sequence-staging destination, and admission wrapper. This avoids a parallel
command path and guarantees the manual example and automation exercise the
same API contract. Direct stock SeqDispatcher or CmdSequencer control remains
out of scope because current authority deliberately rejects it.

### Separate readiness from functional proof

The target wrapper invokes A then B before C and repeats A then B after C.
The C implementation may create only probe-owned helpers, temporary overlays,
and evidence. This makes readiness failure distinguishable from product
failure, rather than allowing a functional test to hide a shared-service
restart or UHF omission.

### Use a compound, fresh completion oracle

PASS requires successful compile/upload/validate/run plus the expected
SoC/mode admission and a completion artifact or status produced during the
same invocation. The probe records timestamps/identifiers and rejects stale
payload output, command errors, or failed-sequence observations. A simple
command response was rejected because it cannot prove payload completion.

### Keep hosted and target records distinct

Hosted runs use isolated roots and local evidence. Target runs additionally
record local head, remote workspace head, build metadata, installed release
pointer, target journal, and ground observations. The target result does not
imply stock GDS, RF, or OTA receipt closure.

## Risks / Trade-offs

- [Target source/binary mismatch] -> record provenance before C and classify
  failure as provenance before considering product changes.
- [Shared-lab residue or missing UHF] -> A/B establish the required baseline;
  C does not repair or stop shared services.
- [Old completion output accepted] -> use a fresh probe root and timestamped
  completion evidence, rejecting stale artifacts explicitly.
- [Sequence API drift] -> the same checked-in source and wrapper contract is
  used by manual and automated paths, with focused syntax and functional
  verification before closeout.
- [Long target proof] -> do not rerun the full repository gate locally for
  every review iteration; retain focused verification and rely on PR CI for
  the full gate unless CI diagnosis requires local reproduction.

## Migration Plan

The change is additive. Existing Route 1 integrated evidence remains
historical context; the new record becomes the authority only after its fresh
hosted and governed target reruns pass, OpenSpec validation succeeds, and the
change is synchronized and archived. If validation exposes a baseline or
provenance problem, preserve the old record and report a bounded non-PASS
rather than changing runtime behavior to force a result.

## Open Questions

None. The target functional boundary, A/B ownership, and required UHF
readiness are governed by the current target-proof documentation.
