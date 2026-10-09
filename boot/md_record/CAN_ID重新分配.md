# CAN ID 重新分配记录 (md_record)

> 记录 OTA/诊断相关 CAN ID 的重新分配变更，按时间倒序追加。

---

## 记录 #1: UDS 诊断 ID 由调试 ID 换为正式分配 ID (2026-10-09)

**分支:** `led-orange` | **影响工程:** boot + app1（两端同步修改）

### 变更内容

| 旧 ID | 新 ID | 方向 | 说明 |
|---|---|---|---|
| `0x18DA03F1` | `0x18FF3AFD` | TBOX → 推杆 | UDS 物理寻址请求（唯一接收 ID） |
| `0x18DAF103` | `0x18FF1F11` | 推杆 → TBOX | UDS 物理寻址响应（只发不收） |
| `0x18DBFFF0` | 删除 | — | 功能寻址广播废弃（本就缺多 ECU 响应抑制逻辑） |
| `0x18FF8118` | 删除 | — | 死 ID（注册了 RX 滤波但被 UDS 层 CAN ID 过滤丢弃，从未生效） |

### 改动文件

**boot（5 个文件）**

| 文件 | 改动 |
|---|---|
| `UDS/uds_diagnostic.h` | 两个 ID 宏换值；删除 `UDS_FUNCTIONAL_REQUEST_ID` |
| `UDS/uds_diagnostic.c` | `uds_receive_handler()` 的 CAN ID 过滤改为只判物理 ID（含日志格式串/参数同步） |
| `UDS/uds_ota.c` | CanIf 滤波表 4 项 → 1 项（只注册请求 ID；响应 ID 不应注册为接收 ID） |
| `Bootloader_App.c` | 延迟 31 ACK（`71 01 FF 00`）与延迟 11 ACK（`51 01`）两处硬编码发送 ID；`BL_ISOTP_RegisterRxFilters()`（死代码）同步 |
| `UDS/isotp_transport.h` | `ISOTP_FILTER_CAN_ID_LIST` = {3AFD, 1F11, 5858}（COUNT=3）；`ISOTP_DEFAULT_RESPONSE_ID` 换值 |
| `UDS/isotp_transport.c` | OTA 日志方向判断/注解的硬编码 ID；删除 0x18FF8118 注解分支 |

**app1（6 个文件）**

| 文件 | 改动 |
|---|---|
| `app/can/uds/uds_diagnostic.h` / `.c` | 同 boot |
| `app/can/can_module.c` | 硬件滤波 Filter 6 base → `0x18FF3AFD`；Filter 7（原 `0x18DBFFF0`）改为 `{0x1FFFFFFF, 0}` 占位（精确匹配一个总线上不存在的 ID，8 槽位保持不变） |
| `app/can/uds/uds_rx_entry.c` | `is_uds_id()` 白名单只留 `0x18FF3AFD` |
| `app/can/uds/bootloader_app.c` | 补发 51 01 ACK 的发送 ID → `0x18FF1F11` |
| `app/can/uds/isotp_transport.h` / `.c` | 记录列表 {3AFD, 1F11}（COUNT=2）、默认响应 ID、日志方向判断 |

### 关键注意点

1. **`ISOTP_FILTER_CAN_ID_LIST` 不是纯调试**：`isotp_receive_frame()` 开头 `ISOTP_ENABLE_FILTER_RECORD` 门控会把不在列表里的 CAN ID 直接丢弃（`return ISOTP_ERROR`）。新请求 ID 必须在列表里，否则收不到任何 UDS 请求。
2. 响应 ID `0x18FF1F11` 是纯发送，**没有**注册进任何 RX 滤波表（符合此前教训：响应 ID 注册为接收 ID 会处理自己发的帧）。
3. 不受影响的 ID：`0x18FF5858`/`0x18EF5858`（强制 OTA 指令及回帧）、`0x18FF5555`/`0x18EF5555`（SWDT 测试）、`0x18FF1108`（心跳）。
4. 新 ID 不符合 ISO 14229 的 `0x18DAxxxx` 约定 —— 上位机若是通用标准诊断仪需确认兼容性；自家工装/TBOX 则无影响。

### 验证方法

1. 上位机/TBOX 侧同步改用新 ID 后：发 `10 03` 等，抓包确认请求到达（boot RTT OTA 日志 `[RX] 0x18FF3AFD`），响应从 `0x18FF1F11` 返回；
2. 完整跑一次 OTA 刷写流程（34/36/37），确认多帧 FC/CF 正常（FC 回复走 `ISOTP_DEFAULT_RESPONSE_ID` = 0x18FF1F11）；
3. 确认复位后 boot/APP 的延迟 ACK（31 → `71 01 FF 00`、11 → `51 01`）从新 ID 发出；
4. app1 运行中发 `0x18FF5858` 强制指令确认未受影响；
5. 用旧 ID `0x18DA03F1` 发请求，确认两端均不再响应。

### 遗留事项

- [ ] 上位机/TBOX/工装脚本同步换 ID
- [ ] Keil 编译验证（boot + app1）
