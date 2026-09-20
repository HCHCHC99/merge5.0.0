# 问题记录 (md_record)

> 本文件记录开发/调试过程中遇到的典型问题与修复方案，按时间倒序追加。

---

## 记录 #1: CAN 收发器供电时序导致 TX 卡死、无法刷写 (2026-09-20)

**分支:** `led-orange` | **影响工程:** boot（app1 工程的 can_module/Adapter_Can 是独立副本，存在同样隐患，尚未同步）

### 现象

板子的 CAN 收发器需要 5V 供电。测试时先只给 MCU 3.3V（收发器无电），持续一段时间后再上 24V（此时收发器才有 5V），**有时**之后无法进行固件刷写，RTT 观察到 `TX queue full!`，必须断电重启才能恢复。

### 根因链路（代码实证）

1. **`can_is_tx_busy()` 是纯软件标志**（can_module.c `m_bTxBusy`），只在 PTB 发送时置 true，只在 `CAN_FLAG_PTB_TX`（PTB 发送**成功**）中断里清 false。
2. 收发器无 5V 时：TX 驱动器失效（节点对总线呈高阻）、无 ACK，RX 电平异常（stuck dominant 或看似空闲）。此时：
   - PTB 帧单发失败（`en_ptb_single_shot = ENABLE`），**TPIF 完成标志永不触发** → `m_bTxBusy` 卡死在 true；
   - 或者 PTB 请求一直挂起（RX stuck dominant 时无法起振）→ 同样卡死。
3. 死锁三连：
   - `CanIf_Send()` 里 `can_is_tx_busy()` 恒 true → 永不走直发路径 → 只进队列 → 队列满 → 每帧 `TX queue full! dropped`；
   - `CanIf_Poll()` 的安全网 `if (!can_is_tx_busy() && 队列非空)` 因 tx_busy 恒 true 而**永不触发**；
   - 期间 TEC 持续累加 → **bus-off**；而 `CanIf_CheckBusOff()` 的"恢复"只调了 `CAN_ExitLocalReset()`（清 RESET 位），**没有先进本地复位**，对 bus-off 状态是空操作。
4. 上 24V 后收发器虽然恢复，但 `m_bTxBusy` 仍卡 true、控制器可能仍在 bus-off → 所有发送继续被丢 → 无法刷写，直到断电重启。
5. "**有时**"复现的原因：3.3V 单独供电时间短、心跳次数少时，TEC 未涨满或某次 TPIF 恰好触发过，可自愈；时间长则必卡死。

**与总线其它报文的关系：** 该 bug 是纯本机状态问题（软件标志 + bus-off），与总线上有没有其它节点通讯无关。故障期间本节点对总线呈高阻，不干扰其它节点；恢复后其它节点的 ACK 反而帮助本机帧正常完成。

### 修复（boot 工程，4 层防御）

| # | 层 | 位置 | 内容 |
|---|---|------|------|
| 1 | 根因 | `can_module.c` `can_module_irq_handler()` | TX 异常兜底：忙标志仍在 + 出现 BUS_ERR/仲裁丢失/TX_ABORTED/BUS_OFF 标志 + TACTIVE=0 → 判定单发失败已被硬件终止 → 强制解锁并推进软件队列 |
| 2 | 导出 | `can_module.c/.h` | 新增 `can_tx_force_idle()`，供适配层强制解锁 `m_bTxBusy` |
| 3 | 兜底 | `Adapter_Can.c` `CanIf_Poll()` | **TX 卡死看门狗**：`can_is_tx_busy()` 持续 true 超 2s（正常发送毫秒级完成）→ `CAN_EnterLocalReset → ExitLocalReset` 复位序列 + 清空积压队列 + 解锁。不依赖任何错误标志，覆盖一切卡死变体 |
| 4 | 补全 | `Adapter_Can.c` `CanIf_CheckBusOff()` | bus-off 恢复补全：原来只调 `CAN_ExitLocalReset()`（空操作），现在先进本地复位再出（协议引擎重置、TEC/REC 清零、放弃 pending TX），同时清积压队列 + 解锁 |

### 修复后时序

```
3.3V 供电（收发器无5V）:
  心跳失败 → IRQ 兜底解锁（队列持续推进，不死锁）
  TEC 累加 → bus-off → 500ms 后复位序列恢复（周期性自愈尝试）
  期间队列满丢帧无所谓（总线收不到）

24V 上电（收发器恢复）:
  即使仍有变体卡住 → 2s 内看门狗强制恢复 → 队列清空 → 心跳恢复
  → 可正常发起刷写，无需断电
```

### 验证方法

1. 重现原故障时序：只给 3.3V，等待 30 秒以上，再上 24V；
2. RTT 应出现 `TX stall 2000ms <-- force recovery: reset cycle + flush queue` 或 `Bus-Off recovery: CAN reset cycle` 日志；
3. 24V 上电后 ≤2.5s 内 CAN 功能恢复，TBOX/工装可正常发起刷写；
4. 总线上挂其它节点正常通讯，确认故障期间与恢复过程均不干扰其它节点。

### 遗留事项

- [ ] app1 工程的 `can_module.c` / `Adapter_Can.c` 同步本修复
- [ ] 收发器无 5V 时 RX 实际电平（stuck dominant / 看似空闲）实测确认，决定故障变体（两种均已被覆盖，仅影响日志表现）
