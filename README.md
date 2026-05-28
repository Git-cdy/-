# 智慧大棚多模态测控与病虫害边缘预警系统

> 大创项目 + 毕业设计双重用途 | 梧州学院 23 通信工程
> 详细技术文档见 [CLAUDE.md](./CLAUDE.md)

## 架构

```
传感器 (SHT30/BH1750/土壤) → STM32F103 → ESP8266 → 阿里云 IoT → 微信小程序
                              ↓
                            K210 (边缘 AI)
                            OV2640 摄像头
```

## 开发进度（11 步增量测试）

| 步骤 | 内容 | 状态 |
|------|------|------|
| 0 | 时钟+调度器+串口 | 🔵 待烧录验证 |
| 1 | GPIO 点灯 | ⬜ |
| 2 | I2C 总线扫描 | ⬜ |
| 3 | SHT30 + BH1750 | ⬜ |
| 4 | ADC 土壤湿度 | ⬜ |
| 5 | OLED 显示 | ⬜ |
| 6 | 继电器 + 蜂鸣器 | ⬜ |
| 7 | 控制逻辑 | ⬜ |
| 8 | K210 UART 帧协议 | ⬜ |
| 9 | 多模态融合 | ⬜ |
| 10 | ESP8266 + 阿里云 MQTT | ⬜ |
| 11 | 全链路 + 离线降级 | ⬜ |

## 仓库结构

```
.
├── CLAUDE.md              ← 项目主文档
├── README.md              ← 本文件
├── .gitignore
├── 毕业设计/code/stm32/greenhouse/   ← 当前 Keil 工程
└── scripts/
    └── commit-stage.sh    ← 一键提交脚本
```

## 提交规范

每完成 11 步中的一步并通过烧录验证后，运行：

```bash
bash scripts/commit-stage.sh <step号> "<简短描述>"
# 示例：
bash scripts/commit-stage.sh 0 "时钟+调度器+串口 验证通过"
```

脚本会自动 commit、打 tag `step0-passed`、push 到 GitHub。

回滚到任意阶段：

```bash
git checkout step0-passed   # 切到第 0 步通过时的快照
git checkout main           # 回到最新
```

## 烧录与硬件参考

引脚分配、电源拓扑、通信协议均见 [CLAUDE.md](./CLAUDE.md)。
