#pragma once

#define CROWPANEL_LCD_SPI_HOST SPI2_HOST
#define CROWPANEL_LCD_PIN_CS GPIO_NUM_15
#define CROWPANEL_TOUCH_PIN_CS GPIO_NUM_12
#define CROWPANEL_LCD_PIN_DC GPIO_NUM_2 // RS
#define CROWPANEL_LCD_HORIZONTAL_RESOLUTION 320
#define CROWPANEL_LCD_VERTICAL_RESOLUTION 240
#define CROWPANEL_LCD_SPI_TRANSFER_HEIGHT (CROWPANEL_LCD_VERTICAL_RESOLUTION / 10)
