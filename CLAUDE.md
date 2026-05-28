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
├── 大创/                   ← 申报书、参考资料
├── 互联网+/               ← 互联网+ 比赛材料
└── 毕业设计/
    └── code/stm32/
        ├── greenhouse/    ← ★ 当前工程（CDY调度器 + 大棚驱动 合并版）
        │   ├── Driver/    ← 所有驱动
        │   │   ├── pin_config.h    ← 集中引脚定义（唯一真相源）
        │   │   ├── scheduler.c/h   ← 时间片轮询调度器
        │   │   ├── uart.c/h        ← UART1(K210帧) + UART2(ESP8266)
        │   │   ├── sht30.c/h       ← 硬件I2C1
        │   │   ├── bh1750.c/h      ← 硬件I2C1 (已加超时保护)
        │   │   ├── soil.c/h        ← ADC
        │   │   ├── relay.c/h       ← 4路继电器
        │   │   ├── buzzer_pwm.c/h  ← TIM3 CH2 PWM
        │   │   ├── OLED.c/h        ← 硬件I2C2 + UTF-8中文引擎
        │   │   ├── OLED_Font.h     ← 13个汉字字模
        │   │   └── delay.c/h
        │   ├── User/
        │   │   └── main.c          ← 完整版主程序
        │   ├── Tests/
        │   │   └── step0_main.c    ← 第0步极简测试版
        │   └── Library/            ← STM32 标准外设库 v3.5.0
        └── STM32_CDY/              ← 原始机房监测项目（参考用）
```

---

## 三、引脚分配总表

| 引脚 | 功能 | 设备 | 注意事项 |
|------|------|------|---------|
| PB6 | I2C1_SCL | SHT30 + BH1750 | 4.7kΩ 上拉至 3.3V |
| PB7 | I2C1_SDA | SHT30 + BH1750 | 同上 |
| PB10 | I2C2_SCL | OLED | 4.7kΩ 上拉至 3.3V |
| PB11 | I2C2_SDA | OLED | 同上 |
| PA0 | ADC1_IN0 | 土壤湿度 | VCC 接 3.3V（不是 5V！） |
| PA9 | USART1_TX | K210 | 115200 8N1, 3.3V 直连 |
| PA10 | USART1_RX | K210 | 同上 |
| PA2 | USART2_TX | ESP8266 | 115200 8N1 |
| PA3 | USART2_RX | ESP8266 | 同上 |
| PA4 | GPIO | 风机继电器 | 低电平 ON, 串 1kΩ |
| PA5 | GPIO | 水阀继电器 | 同上 |
| PA6 | GPIO | 补光灯继电器 | 同上 |
| PA7 | TIM3_CH2 | 蜂鸣器 PWM | 2kHz |
| PA13 | SWDIO | ST-Link | ⛔ 禁止占用 |
| PA14 | SWCLK | ST-Link | ⛔ 禁止占用 |

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

**增量集成测试，禁止一口吃成胖子：**

```
第0步  时钟+调度器+串口         ← 地基
第1步  GPIO 点灯
第2步  I2C 总线扫描
第3步  SHT30 + BH1750
第4步  ADC 土壤湿度
第5步  OLED 显示
第6步  继电器 + 蜂鸣器
第7步  控制逻辑 (传感器→执行器闭环)
        ───── 以上 = 大棚基础版可演示 ─────
第8步  K210 UART 帧协议
第9步  多模态融合
第10步 ESP8266 + 阿里云 MQTT
第11步 全链路 + 离线降级
```

每步验证标准：串口 printf 可见 + 肉眼可确认。

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

## 八、当前状态（2026-05-28）

- ✅ 代码合并完成（greenhouse/ = CDY调度器 + 大棚驱动）
- ✅ 4 个已知 bug 已修复
- ✅ 第 0 步测试代码就绪（Tests/step0_main.c）
- ⬜ 待烧录验证第 0 步
- OLED 汉字字模仅 13 个（机房监控温度湿状态正常警告），后续需扩充

---

## 九、Codex 召唤方式

当 Claude 遇到以下情况时，用户可召唤 Codex：
- 需要批量修改多个文件
- 需要创建/重命名/移动文件
- 需要执行终端命令

Codex 拥有完整文件系统访问权限，Claude 只需在对话中说明需求即可。
