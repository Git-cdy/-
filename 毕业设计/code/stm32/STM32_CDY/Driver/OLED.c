#include "stm32f10x.h"
#include "OLED_Font.h"

// ================== 底层引脚配置与 I2C 模拟 ==================
#define OLED_W_SCL(x)       GPIO_WriteBit(GPIOB, GPIO_Pin_13, (BitAction)(x))
#define OLED_W_SDA(x)       GPIO_WriteBit(GPIOB, GPIO_Pin_12, (BitAction)(x))

void OLED_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);
    OLED_W_SCL(0);
}

void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        OLED_W_SDA(Byte & (0x80 >> i));
        OLED_W_SCL(1);
        OLED_W_SCL(0);
    }
    OLED_W_SCL(1);  // 额外的一个时钟，不处理应答
    OLED_W_SCL(0);
}

// ================== OLED 硬件指令层 ==================
void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);        // 从机地址
    OLED_I2C_SendByte(0x00);        // 写命令
    OLED_I2C_SendByte(Command); 
    OLED_I2C_Stop();
}

void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);        // 从机地址
    OLED_I2C_SendByte(0x40);        // 写数据
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);                    // 设置 Y 位置
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));    // 设置 X 位置高 4 位
    OLED_WriteCommand(0x00 | (X & 0x0F));           // 设置 X 位置低 4 位
}

void OLED_Clear(void)
{  
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for(i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

// ================== OLED 字符与数字显示层 ==================
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);       // 设置光标位置在上半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);          // 显示上半部分内容
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);   // 设置光标位置在下半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);      // 显示下半部分内容
    }
}

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)                            
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

// ================== ✨ 终极版：UTF-8 纯中文解析与排版引擎 ✨ ==================
void OLED_ShowChinese(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i = 0, j, k;
    uint8_t char_count = 0; 
    const uint8_t *font_data;
    
    while (String[i] != '\0')
    {
        font_data = 0; // 核心修复：使用 0 替代 NULL，防止编译报错

        // 1. 去字典里比对 UTF-8 的前两个字节暗号
        for (j = 0; j < Chinese_Font_Map_Size; j++)
        {
            // 核心修复：(uint8_t) 强转，防止汉字高位变成负数导致永远匹配失败
            if ((uint8_t)String[i] == Chinese_Font_Map[j].code_h && 
                (uint8_t)String[i + 1] == Chinese_Font_Map[j].code_l)
            {
                font_data = Chinese_Font_Map[j].font;
                break;
            }
        }
        
        // 2. 如果找到了字模，就精准画图
        if (font_data != 0) 
        {
            // X坐标：基础列(1个单位=8像素) + 已经画了的汉字个数 * 16像素(汉字宽16)
            uint8_t X_Pos = (Column - 1) * 8 + char_count * 16;
            
            OLED_SetCursor((Line - 1) * 2, X_Pos);            
            for (k = 0; k < 16; k++)
            {
                OLED_WriteData(font_data[k]);                 
            }
            
            OLED_SetCursor((Line - 1) * 2 + 1, X_Pos);        
            for (k = 0; k < 16; k++)
            {
                OLED_WriteData(font_data[k + 16]);            
            }
            
            char_count++; 
        }
        
        // 3. 指针往后跳 3 个字节（精准适配 UTF-8 中文步长！）
        i += 3;  
    }
}

// ================== OLED 初始化 ==================
void OLED_Init(void)
{
    uint32_t i, j;
    for (i = 0; i < 1000; i++)          // 上电延时
    {
        for (j = 0; j < 1000; j++);
    }
    
    OLED_I2C_Init();            

    OLED_WriteCommand(0xAE);    // 关闭显示
    OLED_WriteCommand(0xD5);    // 设置显示时钟分频比/振荡器频率
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8);    // 设置多路复用率
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3);    // 设置显示偏移
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);    // 设置显示开始行
    OLED_WriteCommand(0xA1);    // 设置左右方向，0xA1正常 0xA0左右反置
    OLED_WriteCommand(0xC8);    // 设置上下方向，0xC8正常 0xC0上下反置
    OLED_WriteCommand(0xDA);    // 设置COM引脚硬件配置
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81);    // 设置对比度控制
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9);    // 设置预充电周期
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB);    // 设置VCOMH取消选择级别
    OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4);    // 设置整个显示打开/关闭
    OLED_WriteCommand(0xA6);    // 设置正常/倒转显示
    OLED_WriteCommand(0x8D);    // 设置充电泵
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);    // 开启显示
        
    OLED_Clear();               // OLED清屏
}
