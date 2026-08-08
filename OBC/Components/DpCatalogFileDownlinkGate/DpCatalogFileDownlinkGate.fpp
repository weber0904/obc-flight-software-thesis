module OBC {

  @ Gates DpCatalog file completion callbacks so shared FileDownlink
  @ completions from other producers cannot advance the data-product catalog.
  passive component DpCatalogFileDownlinkGate {

    @ Data-product file request from DpCatalog
    guarded input port sendFileIn: Svc.SendFileRequest

    @ FileDownlink completion callback shared with other producers
    guarded input port fileCompleteIn: Svc.SendFileComplete

    @ Forwarded file request to FileDownlink
    output port sendFileOut: Svc.SendFileRequest

    @ Completion callback filtered to the active DpCatalog request
    output port fileCompleteOut: Svc.SendFileComplete

  }

}
