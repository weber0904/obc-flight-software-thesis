# F Prime Native Capability And Project Contributions

Status: current public contribution boundary.  
Last reconciled: 2026-07-29.

| Area | F Prime / third-party foundation | This project adds |
|---|---|---|
| Component model | F Prime components, ports, FPP topology, queues and tasks | Mission-specific component decomposition and `TopCcsds` integration |
| Ground protocol | F Prime command, telemetry, events, files, CCSDS services | S-band/UHF CSP node routing, gateway composition and link policy |
| Command security | F Prime dispatch infrastructure | Challenge auth, secure-command sequencing, source/role authority and admission |
| Mission execution | F Prime commands and CmdSequencer | Mode safety, TTC windows, payload sequences and autonomous link recovery |
| Data history | F Prime data products and file services | HK/state/payload product definitions, chunk policy and delivery proofs |
| Subsystems | libcsp routing and service APIs | EPS/ADCS/COMM/payload bridges, simulators and service ownership |
| Reliability | F Prime events, telemetry, watchdog primitives | Detector-local FDIR, recovery executors, persistent fault context and target proofs |
| Operations | stock F Prime GDS and CLI | Mission Console, secure helpers, manual dual-GDS surfaces and demo routes |
| Delivery | upstream build/test infrastructure | OpenSpec governance, validation registry, evidence records and target packaging |

The contribution claim is integration and mission-specific engineering, not
authorship of F Prime, libcsp, TooJPEG, CCSDS, or their general capabilities.
Dependency versions and licenses are recorded in `THIRD_PARTY_NOTICES.md` and
`SBOM.spdx.json`.
