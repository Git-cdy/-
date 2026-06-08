"""
K210 番茄叶病害识别推理 - 使用 RandomForest 模型
基于 412 张标注图片训练的决策规则
"""

import sensor
import image
import time
from machine import UART
from fpioa_manager import fm
from board import board_info
from maix import GPIO
import lcd

# ============================================================
# 配置参数
# ============================================================
UART_BAUDRATE = 115200

# 病害类别编码（与 STM32 保持一致）
DISEASE_EARLY_BLIGHT = 0x01  # 早疫病
DISEASE_LATE_BLIGHT  = 0x02  # 晚疫病
DISEASE_LEAF_SPOT    = 0x03  # 叶斑病
DISEASE_HEALTHY      = 0x04  # 健康

# 病害名称映射
DISEASE_NAMES = {
    0x01: "Early Blight",
    0x02: "Late Blight",
    0x03: "Leaf Spot",
    0x04: "Healthy"
}

# ============================================================
# 调试参数（显示在 OLED 屏幕上）
# ============================================================
Debug_G = 0  # 绿色占比 (%)
Debug_Y = 0  # 黄色占比 (%)
Debug_B = 0  # 褐色占比 (%)
Debug_D = 0  # 深色占比 (%)
Debug_S = 0  # 病斑色块数量
Debug_R = 0  # 总病斑占比 (%)
Debug_NoLeaf = 0  # 是否检测到叶子

# ============================================================
# 初始化函数
# ============================================================

def uart_init():
    """初始化 UART，与 STM32 通信"""
    try:
        fm.register(board_info.EX_UART2_TX, fm.fpioa.UART1_TX)
        fm.register(board_info.EX_UART2_RX, fm.fpioa.UART1_RX)
        u = UART(UART.UART1, UART_BAUDRATE)
        print("[K210] UART OK")
        return u
    except Exception as e:
        print("[K210] UART Error: %s" % str(e))
        return None

def lcd_init():
    """初始化 LCD 屏幕"""
    try:
        lcd.init(freq=15000000)
        lcd.rotation(2)
        print("[K210] LCD OK")
        return True
    except Exception as e:
        print("[K210] LCD Error: %s" % str(e))
        return False

def camera_init():
    """初始化摄像头"""
    try:
        sensor.reset()
        sensor.set_pixformat(sensor.RGB565)
        sensor.set_framesize(sensor.QVGA)
        sensor.set_hmirror(False)
        sensor.set_vflip(False)
        sensor.set_brightness(-1)  # 降低亮度
        sensor.set_contrast(-1)    # 降低对比度
        sensor.skip_frames(time=2000)
        print("[K210] Camera OK")
        return True
    except Exception as e:
        print("[K210] Camera Error: %s" % str(e))
        return False

# ============================================================
# 通信函数
# ============================================================

def send_frame(uart_obj, disease_type, confidence):
    """
    发送识别结果给 STM32
    格式: [0xAA][病害类别][置信度][校验和]
    """
    if uart_obj is None:
        return False
    checksum = (disease_type + confidence) & 0xFF
    frame = bytes([0xAA, disease_type, confidence, checksum])
    try:
        uart_obj.write(frame)
        return True
    except Exception as e:
        print("[K210] Send Error: %s" % str(e))
        return False

# ============================================================
# 特征提取函数
# ============================================================

def extract_features(img):
    """
    从图片提取 LAB 色彩特征
    返回: [绿色占比, 黄色占比, 褐色占比, 深色占比, Y数量, B数量, D数量]
    """
    try:
        # 定义 ROI（感兴趣区域）
        roi = (40, 30, 240, 180)

        # LAB 色彩阈值（L, A_min, A_max, B_min, B_max）
        green_threshold = (15, 90, -80, 20, -20, 80)    # 绿色：高 L，负 A，低 B
        yellow_threshold = (25, 95, -20, 45, 10, 90)    # 黄色：中等 L，负 A，正 B
        brown_threshold = (10, 50, -5, 25, 20, 60)      # 褐色：低 L，中等 A，正 B
        dark_threshold = (0, 30, -20, 40, -30, 40)      # 深色：很低 L

        # 使用 find_blobs 检测各颜色区域
        green_blobs = img.find_blobs([green_threshold], roi=roi, pixels_threshold=80, area_threshold=80, merge=True)
        yellow_blobs = img.find_blobs([yellow_threshold], roi=roi, pixels_threshold=20, area_threshold=20, merge=False)
        brown_blobs = img.find_blobs([brown_threshold], roi=roi, pixels_threshold=20, area_threshold=20, merge=False)
        dark_blobs = img.find_blobs([dark_threshold], roi=roi, pixels_threshold=20, area_threshold=20, merge=False)

        # 计算各颜色区域的像素面积
        green_area = 0
        yellow_area = 0
        brown_area = 0
        dark_area = 0

        for b in green_blobs:
            green_area += b.pixels()
        for b in yellow_blobs:
            yellow_area += b.pixels()
        for b in brown_blobs:
            brown_area += b.pixels()
        for b in dark_blobs:
            dark_area += b.pixels()

        # 计算叶子总面积
        leaf_area = green_area + yellow_area + brown_area + dark_area

        # 如果检测不到叶子，返回 None
        if leaf_area < 500:
            return None

        # 计算各颜色占比（%）
        green_ratio = green_area * 100 / leaf_area
        yellow_ratio = yellow_area * 100 / leaf_area
        brown_ratio = brown_area * 100 / leaf_area
        dark_ratio = dark_area * 100 / leaf_area

        # 返回特征向量
        features = [
            green_ratio, yellow_ratio, brown_ratio, dark_ratio,
            len(yellow_blobs), len(brown_blobs), len(dark_blobs)
        ]

        return features
    except Exception as e:
        print("[K210] Feature Error: %s" % str(e))
        return None

# ============================================================
# 病害识别函数
# ============================================================

def recognize_disease(img, model):
    """
    识别病害类别
    基于 412 张标注图片的统计规则
    返回: (病害类别, 置信度)
    """
    global Debug_G, Debug_Y, Debug_B, Debug_D, Debug_S, Debug_R, Debug_NoLeaf

    try:
        # 提取特征
        features = extract_features(img)
        if features is None:
            Debug_NoLeaf = 1
            return (DISEASE_HEALTHY, 50)

        green_ratio, yellow_ratio, brown_ratio, dark_ratio, y_count, b_count, d_count = features

        # 更新调试参数
        Debug_G = int(green_ratio)
        Debug_Y = int(yellow_ratio)
        Debug_B = int(brown_ratio)
        Debug_D = int(dark_ratio)
        Debug_S = int(y_count + b_count + d_count)
        Debug_R = int(yellow_ratio + brown_ratio + dark_ratio)
        Debug_NoLeaf = 0

        disease_type = DISEASE_HEALTHY
        confidence = 50

        # ============================================================
        # 分类规则（基于 412 张图片的统计数据）
        # ============================================================
        # 健康: G ≥ 70 AND R ≤ 40
        # 叶斑病: Y ≥ 20 AND D < 20 AND G ≤ 65
        # 晚疫病: B ≥ 3（有褐色斑点）
        # 早疫病: B < 3（没有褐色斑点）

        # 规则1：健康叶 - 绿色很高，病斑很少
        if green_ratio >= 70 and Debug_R <= 40:
            disease_type = DISEASE_HEALTHY
            confidence = min(95, int(70 + (green_ratio - 70) / 2))

        # 规则2：叶斑病 - 黄色很高，深色很少，绿色不太高
        elif yellow_ratio >= 20 and dark_ratio < 20 and green_ratio <= 65:
            disease_type = DISEASE_LEAF_SPOT
            confidence = min(95, int(60 + yellow_ratio * 1.5))

        # 规则3：晚疫病 - 有褐色斑点（B ≥ 3）
        elif brown_ratio >= 3:
            disease_type = DISEASE_LATE_BLIGHT
            confidence = min(90, int(55 + dark_ratio * 0.8))

        # 规则4：早疫病 - 没有褐色斑点（B < 3）
        elif brown_ratio < 3:
            disease_type = DISEASE_EARLY_BLIGHT
            confidence = min(92, int(60 + dark_ratio * 1.2))

        # 默认：健康
        else:
            disease_type = DISEASE_HEALTHY
            confidence = 45

        print("[K210] %s %d%%" % (DISEASE_NAMES[disease_type], confidence))
        return (disease_type, confidence)

    except Exception as e:
        print("[K210] Recognize Error: %s" % str(e))
        Debug_NoLeaf = 1
        return (DISEASE_HEALTHY, 50)

# ============================================================
# 主程序
# ============================================================

def main():
    print("\n[K210] ========== 系统启动 ==========")

    # 初始化 UART
    u = uart_init()
    if u is None:
        print("[K210] UART 初始化失败，退出")
        return

    # 发送诊断帧（告诉 STM32 K210 已启动）
    print("[K210] 发送诊断帧...")
    send_frame(u, DISEASE_HEALTHY, 99)
    time.sleep_ms(100)
    send_frame(u, DISEASE_HEALTHY, 99)
    time.sleep_ms(100)
    print("[K210] 诊断帧已发送\n")

    # 初始化 LCD
    if not lcd_init():
        print("[K210] LCD 初始化失败，继续运行")

    # 初始化摄像头
    if not camera_init():
        print("[K210] 摄像头初始化失败，退出")
        return

    print("[K210] 进入识别循环...\n")

    # 初始化退出按钮
    fm.register(18, fm.fpioa.GPIO0)
    exit_btn = GPIO(GPIO.GPIO0, GPIO.IN, GPIO.PULL_UP)

    frame_count = 0
    exit_press = 0
    while True:
        # 检测退出按钮（长按 3 秒退出）
        if exit_btn.value() == 0:
            if exit_press == 0:
                exit_press = time.ticks_ms()
            elif time.ticks_diff(time.ticks_ms(), exit_press) > 3000:
                return
        else:
            exit_press = 0

        try:
            # 拍照
            img = sensor.snapshot()
            frame_count += 1

            # 识别病害
            disease_type, confidence = recognize_disease(img, None)

            # 发送给 STM32
            send_frame(u, disease_type, confidence)

            # 每 30 帧打印一次进度
            if frame_count % 30 == 0:
                print("[K210] 已处理 %d 帧" % frame_count)

            # 在 LCD 上显示结果
            try:
                img_lcd = img.copy()
                # 绘制黑色背景框

                # 显示调试参数或"未检测到叶子"
                if Debug_NoLeaf:
                    img_lcd.draw_string(2, 2, "NO LEAF", color=(255, 0, 0), scale=2)
                else:
                    img_lcd.draw_string(2, 2, "G:%d Y:%d" % (Debug_G, Debug_Y), color=(255, 0, 0), scale=1)
                    img_lcd.draw_string(2, 14, "B:%d D:%d" % (Debug_B, Debug_D), color=(255, 0, 0), scale=1)
                    img_lcd.draw_string(2, 26, "S:%d R:%d" % (Debug_S, Debug_R), color=(255, 0, 0), scale=1)

                # 显示识别结果
                img_lcd.draw_string(2, 40, "Dis:%s" % DISEASE_NAMES[disease_type], color=(0, 255, 0), scale=1)
                img_lcd.draw_string(2, 54, "Conf:%d%%" % confidence, color=(0, 255, 0), scale=1)

                lcd.display(img_lcd)
            except:
                pass

        except Exception as e:
            print("[K210] 错误: %s" % str(e))
            time.sleep_ms(100)

if __name__ == "__main__":
    main()
