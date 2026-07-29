## MODIFIED Requirements

### Requirement: Interface Index Records Current Event-Quieting Boundaries

`docs/interfaces.md` SHALL describe the current live event surface after the
event-noise reduction changes so reviewers can distinguish operator-facing
events from retained local/debug-only visibility.

#### Scenario: Current docs distinguish mask-change events from periodic reduction
- **WHEN** reviewers inspect the reduced-state observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL state that `STATE_MONITOR_UPDATED` is emitted on
  reduced-state mask changes rather than on every successful scheduled
  reduction.

#### Scenario: Current docs distinguish packetized operator events from local debug events
- **WHEN** reviewers inspect the COMM or data-product observability sections of
  `docs/interfaces.md`
- **THEN** the document SHALL state that success `CSP_PING_RESULT` is not part
  of the default packetized ground live event surface
- **AND** it SHALL state that `HK_TREND_PRODUCT_WRITTEN` remains operator-facing
  while `DpWriter.FileWritten` is local/debug-only visibility by default
- **AND** it SHALL describe `HK_TREND_PRODUCT_WRITTEN` as chunk-oriented rather
  than one-sample-per-file success

## ADDED Requirements

### Requirement: Interface Index Records Chunked Official HK Product Controls

`docs/interfaces.md` SHALL describe the current operator-facing control and
identity surfaces for chunked official HK `.fdp` products so reviewers can see
how freshness and file-size policy are controlled without reading helper code.

#### Scenario: Current docs record chunked HK mission-history identity
- **WHEN** reviewers inspect the official state/history section of
  `docs/interfaces.md`
- **THEN** the document SHALL state that the active official HK `.fdp` path
  uses `HkTrendRecord` record id `0` as an array V6 sample record
- **AND** it SHALL state that each emitted `.fdp` also carries
  `HkTrendChunkMeta` record id `1`

#### Scenario: Current docs record operator freshness and size controls
- **WHEN** reviewers inspect the HK producer interface description in
  `docs/interfaces.md`
- **THEN** the document SHALL state that `HK_TREND_FLUSH` can finalize a
  pending non-empty chunk on demand
- **AND** it SHALL state that `HK_TREND_GET_STATUS` reports pending sample/file
  state
- **AND** it SHALL state that `HK_TREND_TARGET_FILE_BYTES` is the persistent
  target-size control bounded by the current reliable-transfer ceiling
