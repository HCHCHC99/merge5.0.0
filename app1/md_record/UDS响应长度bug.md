# 问题记录 (md_record)

> 记录 UDS 响应报文的长度/格式问题，按时间倒序追加。

---

## 记录 #1: 0x36 肯定响应多一个字节、0x34 肯定响应缺 lengthFormatIdentifier (2026-09-30)

**分支:** `led-orange` | **影响工程:** app1（四驱 APP 侧，`app/can/uds/uds_diagnostic.c` 与 boot 为同一份拷贝，两端同步修复）

**涉及文件:** `app1/app/can/uds/uds_diagnostic.c`

### 现象

- 0x36（TransferData）肯定响应在总线上是 `76 76 <seq>`（3 字节负载），RTT 打印 `Send response: SID=0x76, len=3`；
- 0x34（RequestDownload）肯定响应在总线上是 `74 40 00`，缺少 ISO 14229 要求的 lengthFormatIdentifier 字段；
- 上位机按 ISO 14229 解析时，响应长度与实际内容不符，可能被判定为异常响应。

### 根因

1. **0x36**：handler 违反了本工程的响应构造约定 —— **handler 只写"数据"，`uds_send_response()` 统一在数据前补 `SID + 0x40`**。而 `uds_handle_transfer_data()` 结尾多写了一个肯定响应 SID：

   ```c
   resp[0] = 0x76;        /* 多余：响应 SID 应由 uds_send_response 补 */
   resp[1] = block_seq;
   *resp_len = 2;
   ```

   叠加 `uds_send_response()` 补的 `0x36 + 0x40 = 0x76` 后，负载变成 `76 76 <seq>`。

2. **0x34**：`uds_handle_request_download()` 把最大块长度直接写在数据首字节，漏掉了 lengthFormatIdentifier：

   ```c
   resp[0] = 0x40;  /* 被上位机当成 lengthFormatIdentifier，表示"后面还有 4 字节" */
   resp[1] = 0x00;
   *resp_len = 2;
   ```

   `0x40` 高半字节 = 4 表示 maxNumberOfBlockLength 占 4 字节，但实际只给了 2 字节 → 报文不自洽。

### 修复

`app/can/uds/uds_diagnostic.c`：

```c
/* 0x36 */
resp[0] = block_seq;
*resp_len = 1;         /* 总线：76 <seq>（2 字节负载），日志 len=2 */

/* 0x34 */
resp[0] = 0x20;  /* lengthFormatIdentifier: 长度字段占 2 字节 */
resp[1] = 0x40;  /* 最大块长度高字节 */
resp[2] = 0x00;  /* 最大块长度低字节 */
*resp_len = 3;   /* 总线：74 20 40 00（4 字节负载） */
```

### 核查结论（否定响应无此问题）

用户怀疑"否定响应写了 NRC 但长度不对、NRC 没发出去"，核查后确认**否定响应本身正确**：

- `uds_send_negative_response()` 用 `uint8_t response[3]` 写满 `7F / SID / NRC` 三字节，`isotp_send_message(channel, UDS_PHYSICAL_RESPONSE_ID, response, 3)` 长度一致；
- ISO-TP 单帧路径 `tx_data[0] = ISOTP_FRAME_SINGLE | len` → PCI=0x03，总线为 `03 7F <SID> <NRC> AA AA AA AA`，NRC 在 DLC 内一定发出；
- 全工程 30+ 个 NRC 调用点参数顺序、NRC 常量值均正确。

唯一能让响应"静默发不出去"的机制：`isotp_send_message()` 在 `tx_state != ISOTP_TX_IDLE`（返回 `ISOTP_BUSY`）或未初始化（`ISOTP_ERROR`）时直接 return，而调用方忽略了返回值。当前所有响应都 ≤7 字节、走同步单帧不占用 `tx_state`，实际不会命中。

### 验证方法

1. 抓包确认 0x36 肯定响应为 `76 <seq>`（2 字节负载，PCI=0x02）；
2. 抓包确认 0x34 肯定响应为 `74 20 40 00`（4 字节负载，PCI=0x04）；
3. RTT 日志应为 `Send response: SID=0x76, len=2`；
4. 完整跑一次 OTA，确认上位机不再因响应长度报错。

### 备注

- 该文件为 GBK 编码 + CR 行尾，修改时需保持原编码与行尾不变。
- 若上位机对 0x34 的 lengthFormatIdentifier 另有约定（例如只认 `40 00` 不带该字段），需与上位机侧再对齐。