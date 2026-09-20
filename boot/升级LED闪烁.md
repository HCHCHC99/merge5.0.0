# Bootloader 升级 LED 闪烁 (PC13 / PH2)

> **相关文档:** [收发流程.md](收发流程.md) — OTA 三阶段流程 | [APP槽区切换.md](APP槽区切换.md) — 槽位调度
>
> **实现日期:** 2026-09-07 | **更新日期:** 2026-09-18（新增工装停留模式，改 IDLE/FAIL 灭灯）| **适用工程:** boot（仅 Bootloader 工程）

## 1. 需求

Bootloader 工程新增两个橙色 LED（**PC13**、**PH2**），用于指示 OTA 刷写状态：

| 场景 | 表现 |
|------|------|
| 上电进入 main / 未进入刷写 | **双灯灭** |
| 固件刷写过程 | 双灯 50ms 快闪（亮 50ms + 灭 50ms） |
| 刷写成功（COMPLETE 起） | **双灯常亮** |
| 刷写失败 | **双灯灭** |
| 工装升级成功（停留 bootloader） | 双灯常亮（同上，锁存，直至跳 APP） |
| 跳转 APP 前 | 双灯置灭（由 APP 接管） |

硬件约定：**低电平点亮**。

## 2. 状态映射

LED 状态机轮询 `FlashDownload_GetState()` 自动映射 + 一个锁存态：

| `FlashDownload_GetState()` | LED 状态 | PC13 | PH2 |
|---|---|---|---|
| `IDLE / PREPARING` | `LED_BOOT_IDLE` | 灭 | 灭 |
| `READY / TRANSFERRING / VERIFYING` | `LED_BOOT_PROGRAMMING` | 50ms 快闪 | 50ms 快闪 |
| `COMPLETE` | `LED_BOOT_DONE` | **常亮** | **常亮** |
| `ERROR` | `LED_BOOT_FAIL` | 灭 | 灭 |
| （工装停留，由 `Led_Boot_Stay()` 置入） | `LED_BOOT_STAY` | **常亮** | **常亮** |

- `LED_BOOT_STAY` 为**锁存态**：进入后不再随下载状态自动切换，直至 `Led_Boot_Shutdown()`（跳 APP 前）熄灭；
- 失败→恢复自动处理无需额外逻辑：重新 0x34 → 回 `READY` 快闪；会话超时 → 回 `IDLE` 灭灯。

## 3. 工装停留模式（2026-09-18 新增）

### 3.1 来源判定机制

- 升级工装**全程 100ms** 发送心跳 **`0x18FF5818`**（DLC≥4，`data[3]=1`）；客户 TBOX 不发该心跳；
- boot 在 `UdsOta_Init()` 注册该 ID 的软件滤波（`Boot_FixtureRegisterFilter()`），回调 `Fix_HbCallback` 记录最近心跳时刻；
- **0x11 (ECU Reset) 到达时**（`uds_diagnostic.c` bootloader 上下文）武装 500ms 判定窗口（`Boot_FixtureArmWindow()`），**推迟** pending_sid 写入与复位：

| 窗口结果 | 来源 | 行为 |
|---|---|---|
| 窗口内收到心跳（首帧即判） | 工装 | 立即补发 0x51 ack（`04 51 01 00 00`，与 APP 补发同格式）→ `Led_Boot_Stay()` 双灯常亮 → 停留 bootloader（不写 pending_sid、不复位） |
| 窗口满 500ms 无心跳 | TBOX | 原路径：写 `pending_sid=0x11` → 延迟复位 → 重启跳 APP → APP 补发 ack |

### 3.2 停留态行为

- 停留期间：UDS 照常运行、1108 心跳照发、双橘灯常亮；
- **连续 500ms 无工装心跳** → 灭灯 → `Bootloader_JumpToApp()` 直跳 APP（不复位；目标 = 实际下载槽，未刷写则当前槽）；
- 停留态再收 0x11 → **只回 ack，其余不动**（B 方案：不写扇区 8、不复位、不刷新判失计时器）；
- 工装侧约定：一次上电仅允许刷写一次（含中断），掉电后才能再次升级——因此停留期间 1108 心跳不会引发重复刷写。

### 3.3 扇区状态说明

工装路径 0x11 **跳过** `pending_sid` 写入，扇区 8 保持 COMPLETE 时写入的 `phase=PROGRAMMING_DONE, pending_sid=0`。后续跳 APP 时 `App_CheckPendingUdsAck()` 查 `pending_sid==0x11` 不成立 → 不补发（无重复 ack）→ 清扇区 8。天然自洽，无需额外擦写。

## 4. 实现设计

### 4.1 模块

`Adp/Led_Boot.c/.h`（LED 状态机）+ `Bootloader_App.c` 工装识别模块，与 Gpio_io（底层驱动）、TickTimer（时基）解耦：

```c
/* Led_Boot.h 引脚与极性 */
#define LED_BL1_PORT    GPIO_PORT_C         /* PC13 */
#define LED_BL1_PIN     GPIO_PIN_13
#define LED_BL2_PORT    PH2_PORT            /* PH2, 复用 Gpio_io.h 已有宏 */
#define LED_BL2_PIN     PH2_PIN

#define LED_ON(port,pin)    GPIO_RESET(port,pin)   /* 低电平点亮 */
#define LED_OFF(port,pin)   GPIO_SET(port,pin)

void Led_Boot_Init(void);      /* 双灯输出初始化(灭), 即 IDLE 灭灯态 */
void Led_Boot_Task(void);      /* 非阻塞状态机轮询 */
void Led_Boot_Stay(void);      /* 进入工装停留态: 双灯常亮(锁存) */
void Led_Boot_Shutdown(void);  /* 双灯置灭 + 停用状态机, 跳转 APP 前调用 */
```

```c
/* Bootloader_App.h 工装识别 API (实现在 Bootloader_App.c) */
void Boot_FixtureRegisterFilter(void);   /* 注册心跳接收滤波 (UdsOta_Init 调用) */
void Boot_FixtureArmWindow(void);        /* 0x11 到达: 武装 500ms 判定窗口 */
bool Boot_FixtureStayActive(void);       /* 停留态? */
void Boot_FixtureSendAck(void);          /* 补发 0x51 ack (停留态再收 0x11 时用) */
void Boot_FixturePoll(void);             /* 1ms 轮询: 窗口判定 + 判失直跳 */
```

### 4.2 非阻塞实现要点

- 时基：`tickTimer_GetCount()`（uint64 毫秒），时间戳翻转法，**无回绕问题**；
- 状态切换时重置相位：`s_lastTick = now` 并按新状态设定初始电平（闪灯态从"亮"开始），避免残留电平；
- 状态切换打印可读日志：如 `LED state <-- PROGRAMMING(50ms blink)`、`LED state <-- STAY(solid on)`；
- 工装直跳目标：优先 `FlashDownload_GetProgress().target_address`（实际下载槽），0x34 未发生则 `GetCurrentSlot()` 当前槽。

### 4.3 调用点（一处覆盖全部场景）

`UdsOta_Poll()` 是全部 `while(1)` 的公共调用点，`Led_Boot_Task()` 与 `Boot_FixturePoll()` 加在其 1ms 门控块内：

| while(1) 位置 | LED 效果 |
|---|---|
| main 主循环（正常启动路径, 跳转前短暂经过） | 灭 |
| 上电 50ms 强制指令窗口（`Boot_StartupSequence`） | 灭 |
| `Bootloader_UdsMain()` UDS 编程模式（常驻） | 随下载状态切换 / 工装常亮 |

## 5. 改动文件清单

| # | 文件 | 改动 |
|---|------|------|
| 1 | `Adp/Led_Boot.h` | **新建**: 引脚/极性宏 + 状态枚举(含 STAY) + 接口 |
| 2 | `Adp/Led_Boot.c` | **新建**: 非阻塞状态机实现（IDLE/FAIL 灭、STAY 锁存常亮） |
| 3 | `template/source/main.c` | `Hardware_Init()` 后加 `Led_Boot_Init()` |
| 4 | `UDS/uds_ota.c` | `UdsOta_Init()` 加 `Boot_FixtureRegisterFilter()`；`UdsOta_Poll()` 1ms 门控内加 `Led_Boot_Task()` + `Boot_FixturePoll()` |
| 5 | `UDS/uds_diagnostic.c` | 0x11 处理器改三分支：停留态→只回 ack；否则武装判定窗口（pending/复位推迟到 `Boot_FixturePoll` 窗口到期） |
| 6 | `Bootloader_App/Bootloader_App.c` | 新增工装识别模块（心跳滤波/回调/窗口/停留/ack/直跳）；`Bootloader_JumpToApp()` 内加 `Led_Boot_Shutdown()` |
| 7 | `Bootloader_App/Bootloader_App.h` | 导出 `Boot_Fixture*` API |
| 8 | `template/MDK/template - 副本.uvprojx` | 两个 target 的 Adp 分组均插入 `Led_Boot.c` 条目 |

### Keil 工程文件注意

`template.uvprojx` 为加密二进制格式，无法脚本编辑；`Led_Boot.c` 已加入明文的 **`template - 副本.uvprojx`**。若实际使用加密版工程构建，需在 Keil 中手动把 `..\..\Adp\Led_Boot.c` 添加到 Adp 分组（include path `..\..\Adp` 已存在）。

## 6. 跳转灭灯说明

`Bootloader_JumpToApp()` 内插入位置：

```
// 4. 清除中断使能和挂起寄存器 NVIC->ICER/ICPR
// 4.5 Shutdown LEDs before jump
Led_Boot_Shutdown();
// 5. __set_MSP / SCB->VTOR
// 6. 跳转 APP
```

- 两条跳转路径共用：TBOX 复位重启后正常跳转 + 工装停留态 500ms 判失直跳；
- 跳转路径不返回，LED 冻结在灭态，APP 启动后重新初始化 GPIO 接管。

## 7. 验证清单

- [ ] 上电（无下载）：双灯灭
- [ ] 50ms 强制指令窗口期间：双灯灭
- [ ] 0x34 接受后：双灯切 50ms 快闪（RTT: `LED state <-- PROGRAMMING(50ms blink)`）
- [ ] 0x37 校验完成（刷写成功）：双灯常亮（RTT: `LED state <-- DONE(solid on)`）
- [ ] 刷写失败：双灯灭（RTT: `LED state <-- FAIL(off)`）；重新 0x34 自动恢复快闪
- [ ] **工装升级**（心跳在线）0x11：立即收到 0x51（`04 51 01 00 00`），双灯保持常亮（RTT: `LED state <-- STAY(solid on)`），设备不复位
- [ ] 工装停留期间停发心跳 500ms：双灯灭 → 直跳 APP（RTT: `Fixture heartbeat lost 500ms <-- jump to APP`）
- [ ] **TBOX 升级**（无心跳）0x11：约 500ms 后复位 → 重启跳 APP → APP 补发 0x51（行为同旧版，多 ~500ms 延迟）
- [ ] 0x11 复位跳转瞬间：双灯灭 → APP 接管
