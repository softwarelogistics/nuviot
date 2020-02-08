#ifndef Display_h
#define Display_h

#include <Arduino.h>
#include <M5Stack.h>
#include <U8g2lib.h>
#include <U8x8lib.h>

#define DISPLAY_U8G 100
#define DISPLAY_M5STACK 101

class Display
{
private:
    byte m_displayType;

    U8G2_SSD1306_128X64_NONAME_F_SW_I2C *u8g2;

public:
    Display(byte displayType)
    {
        m_displayType = displayType;

        u8g2 = new U8G2_SSD1306_128X64_NONAME_F_SW_I2C(U8G2_R0, /* clock=*/15, /* data=*/4, /* reset=*/16);
    }

    void begin()
    {
        if (m_displayType == DISPLAY_U8G)
        {
            u8g2->begin();
        }
    }

    void prepare()
    {
        if (m_displayType == DISPLAY_U8G)
        {
            u8g2->begin();
            u8g2->setFont(u8g2_font_6x10_tf);
            u8g2->setFontRefHeightExtendedText();
            u8g2->setDrawColor(1);
            u8g2->setFontPosTop();
            u8g2->setFontDirection(0);

            u8g2->drawStr(0, 0, "Online");
        }
    }

    void setTextColor(unsigned long color)
    {
        if (m_displayType == DISPLAY_M5STACK)
        {
            M5.Lcd.setTextColor(color);
        }
    }

    void setTextSize(int textSize)
    {
        if (m_displayType == DISPLAY_M5STACK)
        {
            M5.Lcd.setTextSize(textSize);
        }
    }

    void clearBuffer(unsigned long color)
    {
        if (m_displayType == DISPLAY_U8G)
        {
            u8g2->clearBuffer();
        }

        if (m_displayType == DISPLAY_M5STACK)
        {
            M5.Lcd.fillScreen(color);
        }
    }

    void clearBuffer()
    {
        if (m_displayType == DISPLAY_U8G)
        {
            u8g2->clearBuffer();
        }

        if (m_displayType == DISPLAY_M5STACK)
        {
            M5.Lcd.fillScreen(WHITE);
        }
    }

    void sendBuffer()
    {
        if (m_displayType == DISPLAY_U8G)
        {
            u8g2->sendBuffer();
        }
    }

    void drawStr(const char *str1)
    {
        clearBuffer();
        drawString(0, 0, str1);
        sendBuffer();
    }

    void drawStr(const char *str1, const char *str2)
    {
        clearBuffer();
        drawString(0, 0, str1);
        drawString(0, 32, str2);
        sendBuffer();
    }

    void drawStr(const char *str1, const char *str2, const char *str3)
    {
        clearBuffer();
        drawString(0, 0, str1);
        drawString(0, 16, str2);
        drawString(0, 32, str3);
        sendBuffer();
    }

    void drawStr(const char *str1, const char *str2, const char *str3, const char *str4)
    {
        clearBuffer();
        drawString(0, 0, str1);
        drawString(0, 16, str2);
        drawString(0, 32, str3);
        drawString(0, 48, str4);
        sendBuffer();
    }

    void drawString(int x, int y, String str)
    {
        drawString(x, y, str.c_str());
    }

    void drawString(int x, int y, const char *str)
    {
        if (m_displayType == DISPLAY_U8G)
        {
            u8g2->drawStr((u8g2_uint_t)x, (u8g2_uint_t)y, str);
        }

        if (m_displayType == DISPLAY_M5STACK)
        {
            M5.Lcd.setCursor(x, y);
            M5.Lcd.print(str);
        }
    }
};
#endif