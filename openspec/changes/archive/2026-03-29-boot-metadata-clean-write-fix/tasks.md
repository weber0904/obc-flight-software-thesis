# Tasks: boot-metadata-clean-write-fix

## 1. Metadata persistence fix

- [x] 1.1 Make `BootMetadataStore` rewrite `metadata-v1.txt` in a truncate-safe mode

## 2. Regression coverage

- [x] 2.1 Add a rollback regression test that reloads the persisted metadata file from disk
- [x] 2.2 Re-run the relevant local test gate and the Raspberry Pi boot probe

## 3. Evidence and archive

- [x] 3.1 Update the Raspberry Pi target evidence with the stale-tail observation and the verified fix
- [x] 3.2 Validate and archive the change
