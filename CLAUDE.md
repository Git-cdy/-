# CLAUDE.md — 智慧大棚多模态测控与病虫害边缘预警系统

> 项目负责人：陈德源 (202309522230) | 梧州学院 电子信息与人工智能学院
> 指导教师：李煜
> 双重用途：大创项目 + 毕业设计
> 主力 AI：Claude（VS Code）
> 备用 AI：Codex（Codex APP）— 需要时手动召唤

---

## 一、项目概述

研制一套以 STM32 为主控、K210 为边缘视觉单元、ESP8266 接入阿里云 IoT 的
端-边-云一体化智慧大棚测控与病虫害预警系统。

### 核心硬件
| 器件 | 型号 | 接口 |
|------|------|------|
| 主控 | STM32F103C8T6 | — |
| 温湿度 | SHT30 | I2C1, 地址 0x44 |
| 光照 | BH1750 | I2C1, 地址 0x23 |
| 土壤湿度 | 电容式 | ADC1_IN0 (PA0) |
| 显示 | OLED 0.96寸 SSD1306 | I2C2 (PB10/PB11) |
| 边缘 AI | K210 + OV2640 | UART1 (PA9/PA10) |
| WiFi | ESP8266 (AT 固件) | UART2 (PA2/PA3) |
| 执行器 | 4 路光耦继电器 | PA4/PA5/PA6 + 蜂鸣器 PA7 |

---

## 二、文件夹结构

```
大创兼毕计/
├── CLAUDE.md              ← 本文件
├── README.md              ← 项目快速开始指南
├── .gitignore             ← git 配置
├── scripts/
│   └── commit-stage.sh    ← 一键 commit + tag + push 脚本
├── tools/                 ← MCP 工具
│   ├── aiot-mcp-server/   ← 阿里云 IoT MQTT MCP（Node.js）
│   └── serial-mcp/        ← 串口调试 MCP（Python）
├── 大创/                   ← 申报书、参考资料
├── 互联网+/               ← 互联网+ 比赛材料
└── 毕业设计/
    └── code/stm32/
        └── STM32_CDY/     ← ★ 当前工程（机房版基线 → 大棚版演化）
            ├── Driver/    ← 所有驱动
            │   ├── scheduler.c/h   ← 时间片轮询调度器
            │   ├── uart.c/h        ← UART1(K210帧) + UART2(ESP8266)
            │   ├── dht11.c/h       ← DHT11（当前）→ SHT30（M1）
            │   ├── OLED.c/h        ← 软件I2C PB12/PB13（当前）
            │   ├── OLED_Font.h     ← 13个汉字字模
            │   ├── motor.c/h       ← PWM（当前）→ 4路继电器（M4）
            │   ├── buzzer.c/h      ← 蜂鸣器
            │   ├── adc.c/h         ← ADC（后续加土壤湿度）
            │   ├── key.c/h         ← 按键（参考）
            │   ├── led.c/h         ← LED（参考）
            │   └── delay.c/h
            ├── User/
            │   ├── main.c          ← 机房版主程序（完整体）
            │   ├── stm32f10x_conf.h
            │   ├── stm32f10x_it.c/h
            │   └── main.c.gb2312   ← 备份
            ├── Tests/
            │   └── step0_main.c    ← 第0步极简测试版（待烧录验证）
            ├── Library/            ← STM32 标准外设库 v3.5.0
            ├── Start/              ← 启动文件
            ├── Objects/            ├── 编译产物（.gitignore 排除）
            ├── Listings/           ├── 编译产物（.gitignore 排除）
            ├── STM32_CDY.uvprojx   ← Keil 工程文件
            ├── STM32_CDY.uvoptx
            ├── 工程介绍.md         ← 机房版工程说明
            └── 修改历程.md         ← 版本演化记录
```

---

## 三、引脚分配总表（STM32_CDY 实际配置）

| 引脚 | 功能 | 设备 | 当前状态 | 备注 |
|------|------|------|---------|------|
| PB6 | I2C1_SCL | SHT30 + BH1750 | ⬜ 未用 | 4.7kΩ 上拉至 3.3V（M1/M2 时启用） |
| PB7 | I2C1_SDA | SHT30 + BH1750 | ⬜ 未用 | 同上 |
| PB10 | I2C2_SCL | OLED（可选） | ⬜ 未用 | 硬件 I2C2（后期优化用） |
| PB11 | I2C2_SDA | OLED（可选） | ⬜ 未用 | 同上 |
| **PB12** | **软件 I2C_SCL** | **OLED（当前）** | **✅ 在用** | **4.7kΩ 上拉至 3.3V** |
| **PB13** | **软件 I2C_SDA** | **OLED（当前）** | **✅ 在用** | **同上** |
| PA0 | ADC1_IN0 | 土壤湿度 | ⬜ 未用 | VCC 接 3.3V（M3 时启用） |
| PA9 | USART1_TX | K210 | ⬜ 未用 | 115200 8N1, 3.3V 直连（M6 时启用） |
| PA10 | USART1_RX | K210 | ⬜ 未用 | 同上 |
| PA2 | USART2_TX | ESP8266 | ⬜ 未用 | 115200 8N1（M8 时启用） |
| PA3 | USART2_RX | ESP8266 | ⬜ 未用 | 同上 |
| PA4 | GPIO | 风机继电器 | ⬜ 未用 | 低电平 ON（M4 时启用） |
| PA5 | GPIO | 水阀继电器 | ⬜ 未用 | 同上 |
| PA6 | GPIO | 补光灯继电器 | ⬜ 未用 | 同上 |
| PA7 | TIM3_CH2 | 蜂鸣器 PWM | ✅ 在用 | 2kHz（机房版已用） |
| PA13 | SWDIO | ST-Link | ⛔ 禁止 | — |
| PA14 | SWCLK | ST-Link | ⛔ 禁止 | — |

---

## 四、电源拓扑

```
12V DC 适配器
  ├─ LM2596 #1 → 5V ─→ 继电器 VCC（JD-VCC 拔掉）
  │                   ─→ K210（5V 供电）
  └─ LM2596 #2 → 5V ─→ AMS1117-3.3 → 3.3V ─→ STM32
                                              ─→ ESP8266（独立引脚！）
                                              ─→ 传感器/OLED VCC
```

⚠️ ESP8266 VCC 旁必须并 100μF 电解 + 0.1μF 陶瓷。Wi-Fi 发射峰值 300mA+。

---

## 五、开发原则

**增量集成测试，禁止一口吃成胖子。STM32_CDY 机房版已验证可跑，现在逐步演化为大棚版：**

```
[已完成 - 机房版继承]
✅ 调度器 + UART + OLED + Motor + Buzzer + DHT11

[向大棚版演化 - 每个 milestone 一个 commit + tag]
M1  SHT30 替换 DHT11           ← 温湿度升级（I2C1 0x44）
M2  加 BH1750 光照             ← 光照传感器（I2C1 0x23）
M3  加土壤湿度 ADC             ← 土壤湿度（ADC PA0）
M4  Motor → 4 路继电器         ← 执行器升级（PA4/PA5/PA6）
M5  OLED 文案改大棚 + 字模扩充 ← 本地化（中文字模）
M6  K210 UART1 帧协议          ← 边缘 AI 集成（PA9/PA10）
M7  多模态融合                 ← 传感器+图像决策树
M8  ESP8266 + 阿里云 MQTT      ← 云端接入（PA2/PA3）
M9  全链路 + 离线降级          ← 容错机制

每个 milestone 验证标准：
  - 串口 printf 可见
  - OLED 显示正确
  - 肉眼可确认硬件工作
```

---

## 六、关键通信协议

### K210 → STM32 (UART1)
```
帧： [0xAA] [病害类别 1B] [置信度 1B] [校验和 1B]
校验和 = (类别 + 置信度) & 0xFF
```

### 阿里云 IoT MQTT
```
Broker:   ${ProductKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com:1883
ClientID: ${DeviceName}|securemode=3,signmethod=hmacsha1|
Username: ${DeviceName}&${ProductKey}
Password: HMAC-SHA1(DeviceSecret, "clientId{clientId}deviceName{deviceName}productKey{productKey}")
```

---

## 七、开发环境

- Keil µVision v5 + ARM Compiler v5
- STM32 标准外设库 v3.5.0（非 HAL）
- 勾选 `Use MicroLIB`
- UTF-8 编码

---

## 八、当前状态（2026-05-28 更新）

### 机房版基线（v1.0-jifang-baseline）
- ✅ STM32_CDY 工程完整（调度器 + UART + OLED + DHT11 + Motor + Buzzer）
- ✅ 4 个已知 bug 已修复
- ✅ 第 0 步测试代码就绪（Tests/step0_main.c）
- ⏳ **待烧录验证第 0 步**（时钟+调度器+串口）
- ✅ 本地 git 首次 commit + tag（v1.0-jifang-baseline）
- ⏳ GitHub push（网络问题，可后续处理）

### 工具链就绪
- ✅ aiot-mcp-server（Node.js，MQTT 调试）
- ✅ serial-mcp（Python，串口调试）
- ✅ anthropic skills 17 个（文档/代码生成）
- ✅ USB-TTL 驱动（硬件就绪）

### 大棚版演化（待启动）
- ⬜ M1: SHT30 替换 DHT11
- ⬜ M2: 加 BH1750 光照
- ⬜ M3: 加土壤湿度 ADC
- ⬜ M4: Motor → 4 路继电器
- ⬜ M5: OLED 文案改大棚 + 字模扩充
- ⬜ M6: K210 UART1 帧协议
- ⬜ M7: 多模态融合
- ⬜ M8: ESP8266 + 阿里云 MQTT
- ⬜ M9: 全链路 + 离线降级

---

## 九、Codex 召唤方式

当 Claude 遇到以下情况时，用户可召唤 Codex：
- 需要批量修改多个文件
- 需要创建/重命名/移动文件
- 需要执行终端命令

Codex 拥有完整文件系统访问权限，Claude 只需在对话中说明需求即可。
