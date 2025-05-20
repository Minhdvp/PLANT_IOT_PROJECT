// oled_display.c
#include "oled_display.h"
#include "ssd1306.h"
#include "fonts.h"
#include <stdio.h>

void oled_display_init(void)
{
    SSD1306_Init();
    SSD1306_Fill(SSD1306_COLOR_BLACK);
    SSD1306_UpdateScreen();
}

void display_sensor_data(float temperature, float humidity, int soil_moisture, int rain_level)
{
    char line[32];

    SSD1306_Fill(SSD1306_COLOR_BLACK);

    snprintf(line, sizeof(line), "Temp: %.1f C", temperature);
    SSD1306_GotoXY(0, 0);
    SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

    snprintf(line, sizeof(line), "Humi: %.1f %%", humidity);
    SSD1306_GotoXY(0, 12);
    SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

    snprintf(line, sizeof(line), "Soil: %d %%", soil_moisture);
    SSD1306_GotoXY(0, 24);
    SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

    snprintf(line, sizeof(line), "Rain: %d %%", rain_level);
    SSD1306_GotoXY(0, 36);
    SSD1306_Puts(line, &Font_7x10, SSD1306_COLOR_WHITE);

    SSD1306_UpdateScreen();
}
