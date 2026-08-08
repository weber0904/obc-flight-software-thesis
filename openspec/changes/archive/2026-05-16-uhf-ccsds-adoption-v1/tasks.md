## 1. Change Setup

- [ ] 1.1 Add proposal, design, tasks, and spec deltas for `uhf-ccsds-adoption-v1`.

## 2. Active UHF CCSDS Topology

- [ ] 2.1 Add the repo-local `OBCComCcsds` UHF subtopology and config.
- [ ] 2.2 Add the UHF CCSDS VCID stamping adapter and focused tests.
- [ ] 2.3 Rewire `TopCcsds` so active UHF command, packet, file, driver, and scheduling paths no longer use `OBCComFprime`.
- [ ] 2.4 Configure UHF CCSDS uplink acceptance for `SCID=0x44` and `VCID=2`.

## 3. Probes, Evidence, And Docs

- [ ] 3.1 Add the active hosted UHF CCSDS adoption probe.
- [ ] 3.2 Update active UHF-related probes and evidence to describe CCSDS-backed UHF truth while preserving historical `ComFprime` regressions.
- [ ] 3.3 Update README, current architecture, verification-path registry, and relevant OpenSpec specs for active UHF CCSDS truth.

## 4. Validation

- [ ] 4.1 Run fresh local generate/build/unit verification.
- [ ] 4.2 Run focused hosted CCSDS S-band and UHF probes.
- [ ] 4.3 Run `openspec validate uhf-ccsds-adoption-v1` and `openspec validate --specs`.
