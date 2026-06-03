# ============================================================
#  K210 病虫害识别系统 —— 智慧大棚边缘AI节点
# ============================================================
#  硬件：K210 + OV2640摄像头 + LCD屏
#  功能：拍照 → 识别病虫害 → LCD显示结果 → UART发给STM32
#  连接：K210的J3(IO6/IO8) → STM32的UART2(PA3/PA2)
#  帧格式：[0xAA] [病害类别] [置信度] [校验和]  共4字节
# ============================================================

# ---------- 导入库 ----------
import sensor      # 摄像头驱动：拍照、设置分辨率等
import image       # 图像处理：画框、缩放、保存
import time        # 延时、计时
from machine import UART        # 串口通信
from fpioa_manager import fm    # K210特有：引脚功能映射
from board import board_info    # 板子引脚定义（EX_UART2_TX等）
import lcd         # LCD屏幕显示

# ============================================================
#  一、配置参数（按需修改）
# ============================================================
UART_BAUDRATE = 115200          # 串口波特率（必须与STM32一致）

# ---------- 病害类别编号（与STM32端 uart_frame.h 保持一致）----------
DISEASE_EARLY_BLIGHT = 0x01     # 早疫病
DISEASE_LATE_BLIGHT  = 0x02     # 晚疫病
DISEASE_LEAF_SPOT    = 0x03     # 叶斑病
DISEASE_HEALTHY      = 0x04     # 健康

# ---------- 编号 → 英文名（显示在LCD上）----------
DISEASE_NAMES = {
    0x01: "Early Blight",
    0x02: "Late Blight",
    0x03: "Leaf Spot",
    0x04: "Healthy"
}

# ============================================================
#  二、初始化函数
# ============================================================

def uart_init():
    """初始化 UART1，J3排针(IO6=TX, IO8=RX)，连STM32 UART2"""
    try:
        fm.register(board_info.EX_UART2_TX, fm.fpioa.UART1_TX)  # IO6→UART1_TX
        fm.register(board_info.EX_UART2_RX, fm.fpioa.UART1_RX)  # IO8→UART1_RX
        u = UART(UART.UART1, UART_BAUDRATE)
        print("[K210] UART1初始化成功，波特率=%d" % UART_BAUDRATE)
        print("[K210] J3: IO6→STM32 PA3, IO8→STM32 PA2")
        return u
    except Exception as e:
        print("[K210] UART1初始化失败: %s" % str(e))
        return None

def lcd_init():
    """初始化LCD屏幕"""
    try:
        lcd.init(freq=15000000)
        lcd.rotation(2)
        print("[K210] LCD初始化成功")
        return True
    except Exception as e:
        print("[K210] LCD初始化失败: %s" % str(e))
        return False

def camera_init():
    """初始化OV2640摄像头"""
    try:
        sensor.reset()
        sensor.set_pixformat(sensor.RGB565)
        sensor.set_framesize(sensor.QVGA)
        sensor.set_hmirror(False)
        sensor.set_vflip(False)
        sensor.skip_frames(time=2000)
        print("[K210] 摄像头初始化成功")
        return True
    except Exception as e:
        print("[K210] 摄像头初始化失败: %s" % str(e))
        return False

# ============================================================
#  三、通信函数
# ============================================================

def send_frame(uart_obj, disease_type, confidence):
    """发送4字节帧给STM32: [0xAA][类别][置信度][校验和]"""
    if uart_obj is None:
        return False
    checksum = (disease_type + confidence) & 0xFF
    frame = bytes([0xAA, disease_type, confidence, checksum])
    try:
        uart_obj.write(frame)
        return True
    except Exception as e:
        print("[K210] 发送失败: %s" % str(e))
        return False

# ============================================================
#  四、识别函数（占位符——等模型训练完替换）
# ============================================================

def recognize_disease(img):
    """病虫害识别——当前占位符，永远返回"健康100%"
    未来替换：加载.kmodel → KPU推理 → 返回(类别, 置信度)"""
    return (DISEASE_HEALTHY, 100)

# ============================================================
#  五、主程序
# ============================================================

def main():
    print("\n[K210] ========== 系统启动 ==========")

    # 1. 初始化串口
    u = uart_init()
    if u is None:
        print("[K210] UART初始化失败，退出")
        return

    # 2. 发送诊断帧（告诉STM32 K210还活着）
    print("[K210] 发送诊断帧到STM32...")
    send_frame(u, DISEASE_HEALTHY, 99)
    time.sleep_ms(100)
    send_frame(u, DISEASE_HEALTHY, 99)
    time.sleep_ms(100)
    print("[K210] 诊断帧已发送\n")

    # 3. 初始化LCD
    if not lcd_init():
        print("[K210] LCD失败，继续运行（仅串口通信）")

    # 4. 初始化摄像头
    if not camera_init():
        print("[K210] 摄像头失败，退出")
        return

    print("[K210] 进入识别循环...\n")

    frame_count = 0
    while True:
        try:
            img = sensor.snapshot()
            frame_count += 1
            disease_type, confidence = recognize_disease(img)
            send_frame(u, disease_type, confidence)
            if frame_count % 30 == 0:
                print("[K210] FPS: %d, 已发送%d帧" % (
                    frame_count // max(1, time.ticks_ms() // 1000), frame_count))

            # 在LCD上显示摄像头画面+识别结果
            try:
                img_lcd = img.copy()
                img_lcd.draw_string(10, 10, "Disease: %s" % DISEASE_NAMES[disease_type], color=(0,255,0), scale=2)
                img_lcd.draw_string(10, 40, "Conf: %d%%" % confidence, color=(0,255,0), scale=2)
                lcd.display(img_lcd)
            except Exception as lcd_e:
                pass  # LCD显示失败不中断主循环
        except Exception as e:
            print("[K210] 错误: %s" % str(e))
            time.sleep_ms(100)

if __name__ == "__main__":
    main()