module OBC {

  constant CommandIngressMuxIngressPorts = 2

  passive component CommandIngressMux {

    sync input port commandIn: [CommandIngressMuxIngressPorts] Fw.Com

    output port commandOut: Fw.Com

    sync input port commandStatusIn: Fw.CmdResponse

    output port commandStatusOut: [CommandIngressMuxIngressPorts] Fw.CmdResponse

  }

}
