module OBC {

  @ Selects the active telemetry and file-downlink egress branch while
  @ preserving separate source-bound command-response paths elsewhere.
  passive component CommEgressMux {

    constant PacketClasses = 2

    @ Telemetry or event packet to route to the current primary telemetry link
    sync input port packetIn: [PacketClasses] Fw.Com

    @ File-downlink buffer to route to the current primary file link
    sync input port fileBufferIn: Fw.BufferSend

    @ Return path from the S-band file queue
    sync input port sbandFileBufferReturnIn: Fw.BufferSend

    @ Return path from the UHF file queue
    sync input port uhfFileBufferReturnIn: Fw.BufferSend

    telemetry port Tlm
    time get port Time

    @ Routed telemetry or event packet to the S-band branch
    output port sbandPacketOut: [PacketClasses] Fw.Com

    @ Routed telemetry or event packet to the UHF branch
    output port uhfPacketOut: [PacketClasses] Fw.Com

    @ Routed file-downlink buffer to the S-band branch
    output port sbandFileBufferOut: Fw.BufferSend

    @ Routed file-downlink buffer to the UHF branch
    output port uhfFileBufferOut: Fw.BufferSend

    @ Merged file-buffer return path back to FileDownlink
    output port fileBufferReturnOut: Fw.BufferSend

    @ Count of event packets routed to S-band while S-band is the active telemetry link
    telemetry SBAND_ROUTED_EVENT_PACKETS: U32 id 0x00

    @ Count of telemetry packets routed to S-band while S-band is the active telemetry link
    telemetry SBAND_ROUTED_TLM_PACKETS: U32 id 0x01

    @ Count of event packets suppressed while S-band is the active telemetry link
    telemetry SBAND_SUPPRESSED_EVENT_PACKETS: U32 id 0x02

    @ Count of telemetry packets suppressed while S-band is the active telemetry link
    telemetry SBAND_SUPPRESSED_TLM_PACKETS: U32 id 0x03

    @ Count of event packets routed to UHF while UHF is the active telemetry link
    telemetry UHF_ROUTED_EVENT_PACKETS: U32 id 0x04

    @ Count of telemetry packets routed to UHF while UHF is the active telemetry link
    telemetry UHF_ROUTED_TLM_PACKETS: U32 id 0x05

    @ Count of event packets suppressed while UHF is the active telemetry link
    telemetry UHF_SUPPRESSED_EVENT_PACKETS: U32 id 0x06

    @ Count of telemetry packets suppressed while UHF is the active telemetry link
    telemetry UHF_SUPPRESSED_TLM_PACKETS: U32 id 0x07

  }

}
