# 智慧大棚多模态测控与病虫害边缘预警系统

> 🎓 大创项目 + 毕业设计 | 梧州学院 23通信工程 陈德源

## 架构

```
传感器(SHT30/BH1750/土壤) → STM32F103C8T6 → ESP8266 → 阿里云IoT → 微信小程序
                                ↓
                             K210 (边缘AI)
                           OV2640 摄像头
```

## 开发进度

| 步骤 | 内容 | 状态 |
|------|------|------|
| T-00 | 时钟+调度器+串口 | ✅ |
| T-01 | GPIO 点灯 | ✅ |
| T-02 | I2C 总线扫描 | ✅ |
| T-03 | SHT30 + BH1750 传感器 | ✅ |
| T-04 | ADC 土壤湿度 | ✅ |
| T-05 | OLED 显示（中文GBK） | ✅ |
| T-06 | 继电器 + 蜂鸣器 | ✅ |
| T-07 | 自动控制逻辑 | ✅ |
| T-08 | K210 UART 帧协议 | ✅ |
| T-09 | 多模态融合 | ✅ |
| T-10 | ESP8266 + 阿里云 MQTT | 🔲 进行中 |
| T-11 | 全链路 + 离线降级 | 🔲 |

## 仓库结构

```
.
├── CLAUDE.md              → 项目主文档（AI协作规范）
├── README.md              → 本文件
├── .gitignore
├── 毕业设计/code/stm32/STM32_CDY/   → Keil 工程（标准外设库）
│   ├── Driver/            → 驱动层（传感器/通信/控制）
│   ├── User/main.c        → 主程序
│   └── Tests/             → 诊断测试代码
├── 毕业设计/code/CanMV/code/main.py → K210 识别程序
└── tools/                 → MCP 工具
```

## 硬件清单

| 模块 | 型号 | 接口 |
|------|------|------|
| MCU | STM32F103C8T6 | - |
| 温湿度 | SHT30 | I2C1 (PB6/PB7) |
| 光照 | BH1750 | I2C1 (PB6/PB7) |
| 土壤湿度 | 电容式 | ADC (PA0) |
| 显示 | 0.96" OLED | 模拟I2C (PB12/PB13) |
| AI视觉 | K210 + OV2640 | UART2 (PA2/PA3) |
| WiFi | ESP8266 (ESP-01S) | UART3 (PB10/PB11) |
| 执行 | 4路继电器 + 蜂鸣器 | PA4/PA5/PA6 + PA8 |
| 风扇 | 直流电机 PWM | PA1 |

## 技术栈

- **STM32**: 标准外设库 (StdPeriph), Keil5, ARMCC v5, GBK编码
- **K210**: CanMV / MicroPython, MaixPy
- **通信协议**: K210→STM32: 4字节帧 [0xAA][类别][置信度][校验和]
- **编码**: 所有 .c/.h 文件必须 GBK 编码（ARMCC v5 限制）

## 提交规范

```
bash scripts/commit-stage.sh <step号> "<简短描述>"
```