# GDS 與背景程序啟動指引

## 1. 文件目的

本文件整理第一版在 macOS / Linux 上啟動 ZMQ proxy、simulators、OBC deployment 與 GDS 的建議順序。

## 2. 通用原則

1. `fprime-gds` 為長時間執行程序，需使用獨立 terminal。
2. ZMQ proxy、simulators、OBC deployment、GDS 應分開管理，避免互相中斷。
3. 若需背景執行，請以 `nohup` 或等價方式保存輸出。
4. 文件中一律使用 **repo 相對概念**，不再寫死舊絕對路徑。

## 3. macOS 預設 port 建議

為避開常見埠衝突，建議：

- ZMQ subscribe port：`6100`
- ZMQ publish port：`7100`
- GDS GUI port：`8080`

## 4. 建議啟動順序

### Step 1：啟動 ZMQ proxy

```bash
. fprime-venv/bin/activate
nohup <zmqproxy-command> -s tcp://0.0.0.0:6100 -p tcp://0.0.0.0:7100 > logs/zmqproxy.log 2>&1 &
```

### Step 2：啟動 simulator

```bash
nohup <eps-simulator-command>  > logs/eps.log  2>&1 &
nohup <adcs-simulator-command> > logs/adcs.log 2>&1 &
```

### Step 3：啟動 OBC deployment

```bash
nohup <deployment-command> > logs/obc.log 2>&1 &
```

### Step 4：啟動 GDS

```bash
fprime-gds --gui-port 8080
```

## 5. 驗證項目

啟動後至少確認：

1. GDS 可開啟
2. 基本 telemetry 可見
3. `CSP_INIT` 成功
4. `CSP_PING` 可打到測試節點或 simulator

## 6. 停止建議

```bash
pkill -f zmqproxy
pkill -f eps
pkill -f adcs
pkill -f CubeSatDeployment
pkill -f fprime-gds
```

> 請依實際 binary 名稱調整 `pkill` 關鍵字。
