#pragma once

#include "esp_log.h"

#include "led_strip.h"
#include "math.h"


#define LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define LED_STRIP_GPIO_NUM      11
#define LED_STRIP_NUMBERS       36

// 定义最大亮度值
#define MAX_BRIGHTNESS 100
// 定义亮度衰减系数
#define BRIGHTNESS_DECAY 0.5

typedef enum {
  LED_RING_SINGLE_LIGHT = 0, // 单灯模式
  LED_RING_SINGLE_BREATH_LIGHT, // 呼吸灯模式
  LED_RING_BREATH_LIGHT, // 呼吸灯模式
  LED_RING_GRADIENT_SINGLE_COLOR, // 单色渐变模式
  LED_RING_GRADIENT_DOUBLE_COLOR, // 双色渐变模式

  LED_RING_CENTER_GRADIENT, // 中心渐变模式
  LED_RING_SNAKE_GRADIENT,  // 蛇形渐变模式

} led_ring_mode_t;

// RGB色值结构体
typedef struct{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} led_ring_color_t;

// RGB色值+位置结构体（用于配置渐变色）
typedef struct{
  unsigned char r;
  unsigned char g;
  unsigned char b;
  float pos;
} led_ring_color_pos_t;

typedef struct{
  led_strip_handle_t led_strip;
  int led_index;
  float angle;
  uint32_t brightness;
  led_ring_mode_t mode;
  led_ring_color_pos_t colors[2];
} led_ring_on_config_t;


void led_ring_init(led_strip_handle_t *led_strip);

void led_ring_on(const led_ring_on_config_t *config);

void led_ring_clear(led_strip_handle_t led_strip);