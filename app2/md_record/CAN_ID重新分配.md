# CAN ID 重新分配记录 (md_record)

> 记录 OTA/诊断相关 CAN ID 的重新分配变更，按时间倒序追加。

---

## 记录 #1: UDS 诊断 ID 由调试 ID 换为正式分配 ID (2026-10-09)

**分支:** `led-orange` | **影响工程:** app2（本工程与 app1 同源，改动继承自 app1；boot 侧记录见 `boot/md_record/CAN_ID重新分配.md`）

### 变更内容

| 旧 ID | 新 ID | 方向 | 说明 |
|---|---|---|---|
| `0x18DA03F1` | `0x18FF3AFD` | TBOX → 推杆 | UDS 物理寻址请求（唯一接收 ID） |
| `0x18DAF103` | `0x18FF1F11` | 推杆 → TBOX | UDS 物理寻址响应（只发不收） |
| `0x18DBFFF0` | 删除 | — | 功能寻址广播废弃 |
| `0x18FF8118` | 删除 | — | 死 ID（从未真正生效） |

### 改动文件（app2，6 个，内容同 app1）

| 文件 | 改动 |
|---|---|
| `app/can/can_module.c` | 硬件滤波 Filter 6 base → `0x18FF3AFD`（本机收到 UDS 请求的唯一入口）；Filter 7（原 `0x18DBFFF0`）改为 `{0x1FFFFFFF, 0}` 占位（精确匹配一个总线上不存在的 ID，8 槽位数量保持不变） |
| `app/can/uds/uds_diagnostic.h` / `.c` | ID 宏换值；删除 `UDS_FUNCTIONAL_REQUEST_ID`；`uds_receive_handler()` 的 CAN ID 过滤改为只判物理 ID |
| `app/can/uds/uds_rx_entry.c` | `is_uds_id()` 白名单只留 `0x18FF3AFD` |
| `app/can/uds/bootloader_app.c` | 补发 51 01 ACK（`App_CheckPendingUdsAck`）的发送 ID → `0x18FF1F11` |
| `app/can/uds/uds_ota.c` | CanIf 滤波注册表 4 项 → 1 项 |
| `app/can/uds/isotp_transport.h` / `.c` | `ISOTP_FILTER_CAN_ID_LIST` = {3AFD, 1F11}（COUNT=2）；`ISOTP_DEFAULT_RESPONSE_ID` 换值；OTA 日志方向判断同步 |

### 关键注意点

1. **`ISOTP_FILTER_CAN_ID_LIST` 不是纯调试**：`isotp_receive_frame()` 开头的 `ISOTP_ENABLE_FILTER_RECORD` 门控会把不在列表里的 CAN ID 直接丢弃（`return ISOTP_ERROR`）。新请求 ID 必须在列表里。
2. 响应 ID `0x18FF1F11` 是纯发送，未注册进任何 RX 滤波表。
3. 硬件滤波掩码语义（位=1 通配、位=0 必须匹配）下已验证：`0x18FF3AFD` 不会误命中 Filter 1/3/5/8 的合并掩码，只被 Filter 6 精确接收。
4. 不受影响的 ID：`0x18FF5858`/`0x18EF5858`（强制 OTA）、`0x18FF5555`/`0x18EF5555`（SWDT 测试）、`0x18FF1108`（心跳）、全部车身协议 ID。
5. 新 ID 不符合 ISO 14229 的 `0x18DAxxxx` 约定 —— 上位机若是通用标准诊断仪需确认兼容性。

### 验证方法

1. 上位机/TBOX 侧同步改用新 ID 后：发 `10 03` 等，确认请求进 `uds_rx_entry`（`is_uds_id` 放行）→ ISOTP → UDS，响应从 `0x18FF1F11` 返回；
2. 确认 `10 32`/`31 01` 等多字节响应的多帧 FC/CF 正常（FC 回复走 `ISOTP_DEFAULT_RESPONSE_ID` = 0x18FF1F11）；
3. OTA 全流程（0x31 → 复位进 boot → 下载 → 0x11 → APP 补发 51 01）各环节 ACK 均从新 ID 发出；
4. 用旧 ID `0x18DA03F1` 发请求，确认不再响应。

### 遗留事项

- [ ] 上位机/TBOX/工装脚本同步换 ID
- [ ] Keil 编译验证（app2 + boot）
