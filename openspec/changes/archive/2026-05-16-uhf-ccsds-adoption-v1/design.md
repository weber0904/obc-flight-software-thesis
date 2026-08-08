## Context

The active hosted `TopCcsds` topology already imports `ComCcsds.Subtopology` for S-band and a local `OBCComFprime.Subtopology` for UHF. `GroundLinkDriver` instances, `CommController`, `CommEgressMux`, and `CommandIngressAuthority` already distinguish S-band and UHF at the runtime-policy layer, so the remaining migration work is the app-side framing and queueing path for UHF.

The current repository also keeps `FW_COM_BUFFER_MAX_SIZE = 512` and `FW_FILE_BUFFER_MAX_SIZE = 256`, which means the stock upstream `Svc::Ccsds::TmFramer` cannot support a UHF-specific TM frame size below the full `Fw::ComBuffer` size without a much deeper buffer-model fork. For this wave the UHF CCSDS path therefore keeps `TmFrameFixedSize = 1024`, matching the default CCSDS frame size, while still separating UHF from S-band through a distinct stack instance and explicit `VCID = 2`.

## Goals / Non-Goals

**Goals**

- Remove active `TopCcsds` dependence on `OBCComFprime`.
- Keep S-band and UHF as separate CCSDS stack instances.
- Preserve active UHF backup, UHF primary-after-switch, and UHF file/downlink behavior.
- Make UHF link identity explicit with `VCID = 2`.
- Add bounded hosted UHF CCSDS adoption evidence with decoded framing observations.

**Non-Goals**

- Retire `Top` or `OBC_ComFprimeLegacy`.
- Rework the global `FW_COM_BUFFER_MAX_SIZE` or `FW_FILE_BUFFER_MAX_SIZE` model.
- Shrink UHF TM frames below `1024` in this wave.
- Introduce RF, reliable-transfer, or Raspberry Pi target claims beyond the existing governed boundaries.

## Decisions

- **Use a separate local CCSDS stack for UHF.** The active topology shall instantiate a second CCSDS path for UHF rather than sharing the S-band `ComCcsds` instance. This keeps link-local frame state, queue state, and driver bindings separate.
- **Keep the stock CCSDS packet vocabulary.** UHF shall reuse the same `SCID` and APID semantics as S-band. Link identity is carried by the dedicated stack instance, ingress source, and `VCID = 2`.
- **Use explicit VCID stamping instead of global config changes.** A small repo-local context adapter shall stamp `vcId = 2` on UHF egress between the UHF `ComQueue` and UHF CCSDS downlink framing path. UHF uplink shall configure `TcDeframer` to accept only `VCID = 2`.
- **Preserve the existing command and ownership model.** `CommandIngressAuthority`, `CommController`, and `CommEgressMux` keep ownership of policy and runtime semantics. The migration changes only the active UHF transport/framing path.
- **Keep historical UHF `ComFprime` evidence intact.** Historical records and probes remain reviewable regression references and are not rewritten as if they had always been CCSDS-backed.

## Implementation Notes

- Add a repo-local `OBCComCcsds` FPP module and config under `OBC/TopCcsds/`.
- Add a repo-local `UhfCcsdsVcidAdapter` component to stamp `vcId = 2` on UHF egress.
- Reuse upstream `FrameAccumulator`, `TcDeframer`, `SpacePacketDeframer`, `SpacePacketFramer`, `ApidManager`, `ComQueue`, `ComStub`, and `TmFramer`.
- Configure the local UHF `TcDeframer` to `(vcid=2, scid=0x44, acceptAllVcid=false)`.
- Keep the local UHF CCSDS frame size at `1024` for this wave because the stock `Fw::ComBuffer` model still requires a TM frame large enough to carry the full communication buffer.

## Risks / Trade-offs

- **Generated-name collisions for local FPP artifacts**: use unique enum and topology names inside the local UHF CCSDS module instead of reusing generic `Subtopology` names from the old local `OBCComFprime` wrapper.
- **UHF proof drift with old `ComFprime` wording**: update active probes, evidence, and architecture truth together so the active path no longer implies `ComFprime`.
- **Legacy regression confusion**: keep explicit language that `Top`, `OBC_ComFprimeLegacy`, and `run_uhf_uart_backup_link_probe.sh` remain historical or regression-only in this wave.
