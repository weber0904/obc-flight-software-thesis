## 1. 初始化 Git/OpenSpec scaffolding

- [x] 1.1 初始化 Git repository 並建立專案內 OpenSpec / Codex scaffolding
- [x] 1.2 補上 workspace ignore 規則與 bootstrap change 骨架

## 2. 重編 obc-dev-spec 與封存 legacy docs

- [x] 2.1 將 13 份 legacy narrative docs 封存到 `obc-dev-spec/archive/legacy-v0/`
- [x] 2.2 重寫 9 份 narrative source docs，附來源對照與新的 ownership 邊界

## 3. 撰寫 9 個 main specs

- [x] 3.1 撰寫 `proposal.md` 與 `design.md`，定義文檔治理與能力切分
- [x] 3.2 撰寫 9 個 capability spec files，覆蓋 platform、core、resource、subsystems、verification、workflow

## 4. 執行一致性檢查與 OpenSpec validate

- [x] 4.1 檢查 dead links、symbol ownership、狀態術語與預設值一致性
- [x] 4.2 執行 `openspec validate formalize-obc-baseline`

## 5. archive bootstrap change，建立後續 change queue

- [x] 5.1 archive `formalize-obc-baseline` 並確認 main specs 已同步
- [x] 5.2 建立 follow-on change queue：platform、core、EPS、ADCS、comm、boot、verification
