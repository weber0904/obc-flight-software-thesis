# Route 2 Target Manual Flow

暫時手動操作用，之後可刪。

## 1. Baseline

```bash
cd ${REPO_ROOT}

JSON_OUT=/private/tmp/route2-target-baseline-target.json \
  bash scripts/ensure_target_comm_lab_baseline.sh

JSON_OUT=/private/tmp/route2-target-baseline-ground.json \
  bash scripts/ensure_ground_dual_gds_baseline.sh
```

## 2. TTC / ADCS Mode Stage

```bash
PROBE_ROOT=/private/tmp/route2-target-ttc-manual \
  bash scripts/chapter5_routes/target/route2_mode_ttc_entry.sh
```

看結果：

```bash
cat /private/tmp/route2-target-ttc-manual/diagnostics/route2-mode-ttc-entry-summary.json
sed -n '1,200p' /private/tmp/route2-target-ttc-manual/summary.log
tail -n 80 /private/tmp/route2-target-ttc-manual/sband-ground/events.log
```

直接看 target journal：

```bash
ssh operator@obc.local "journalctl -u obc-comm-csp-stack.service -n 120 --no-pager | tail -n 120"
```

如果要特別看 TTC / ADCS：

```bash
ssh operator@obc.local "journalctl -u obc-comm-csp-stack.service --since '-5 min' --no-pager | rg 'TTC_POLICY|SYS_MODE_CHANGE|ADCS_MODE_CHANGE|ADCS_POINTING_ACQUIRED'"
```

## 3. Link Recovery / HK Stage

```bash
PROBE_ROOT=/private/tmp/route2-target-link-manual \
  bash scripts/chapter5_routes/target/route2_secure_auth_link_recovery.sh
```

看結果：

```bash
sed -n '1,200p' /private/tmp/route2-target-link-manual/summary.log
cat /private/tmp/route2-target-link-manual/secure-auth-proof/diagnostics/secure-auth-proof-summary.json
```

看 UHF / S-band journal：

```bash
ssh operator@obc.local "journalctl -u obc-comm-csp-stack.service --since '-10 min' --no-pager | rg 'COMM_PRIMARY_LINK_CHANGED|BOOT_RECOVERY_STATUS|Secure command|GROUND_LINK'"
ssh operator@subsystem.local "journalctl -u subsystem-uhf-csp.service --since '-10 min' --no-pager | tail -n 120"
ssh operator@subsystem.local "journalctl -u subsystem-sband-csp.service --since '-10 min' --no-pager | tail -n 120"
```

## 4. Whole Route Wrapper

```bash
PROBE_ROOT=/private/tmp/route2-target-whole-manual \
  bash scripts/chapter5_routes/target/run_route2_target.sh
```

看結果：

```bash
sed -n '1,240p' /private/tmp/route2-target-whole-manual/summary.log
```
