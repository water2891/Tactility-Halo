#include "LEDRing.h"
#include "esp_check.h"

static const char *TAG = "led_ring";

// 比较函数
int compare_colors(const void *a, const void *b) {
  led_ring_color_pos_t *ca = (led_ring_color_pos_t *)a;
  led_ring_color_pos_t *cb = (led_ring_color_pos_t *)b;
  if (ca->pos < cb->pos) return -1;
  if (ca->pos > cb->pos) return 1;
  return 0;
}

// 线性插值函数
unsigned char interpolate(unsigned char start, unsigned char end, float ratio) {
  return (unsigned char)((float)start + ratio * ((float)end - (float)start));
}

void interpolate_color(led_ring_color_pos_t *result, led_ring_color_pos_t *start, led_ring_color_pos_t *end, float pos) {
  float ratio = (pos - start->pos) / (end->pos - start->pos);
  result->r = interpolate(start->r, end->r, ratio);
  result->g = interpolate(start->g, end->g, ratio);
  result->b = interpolate(start->b, end->b, ratio);
  result->pos = pos;
}

// 主要函数
led_ring_color_pos_t get_interpolated_color(led_ring_color_pos_t *colors, int length, float pos) {
  if (length == 0 || pos < 0.0f || pos > 1.0f) {
      led_ring_color_pos_t invalid = {0, 0, 0, 0};
      return invalid;
  }

  // 如果只有一个颜色，则直接返回该颜色
  if (length == 1) {
      return colors[0];
  }

  // 排序数组
  qsort(colors, length, sizeof(led_ring_color_pos_t), compare_colors);

  // 查找第一个比pos大的位置
  int index = 0;
  while (index < length && colors[index].pos < pos) {
      index++;
  }

  // 如果pos小于第一个元素的位置，或者等于最后一个元素的位置
  if (index == 0) {
      return colors[0];
  }
  if (index >= length) {
      return colors[length - 1];
  }

  // 计算插值
  led_ring_color_pos_t interpolated_color;
  interpolate_color(&interpolated_color, &colors[index - 1], &colors[index], pos);
  return interpolated_color;
}


led_ring_color_t interpolate_rgb(led_ring_color_t start, led_ring_color_t end, double ratio) {
  led_ring_color_t result;
  result.r = (uint8_t)(start.r + ratio * (end.r - start.r));
  result.g = (uint8_t)(start.g + ratio * (end.g - start.g));
  result.b = (uint8_t)(start.b + ratio * (end.b - start.b));
  return result;
}

void led_ring_init(led_strip_handle_t *led_strip)
{
  /// LED strip common configuration
  led_strip_config_t strip_config = {
      .strip_gpio_num = LED_STRIP_GPIO_NUM,                        // The GPIO that connected to the LED strip's data line
      .max_leds = LED_STRIP_NUMBERS,                               // The number of LEDs in the strip,
      .led_model = LED_MODEL_WS2812,                               // LED strip model, it determines the bit timing
      .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color component format is G-R-B
      .flags = {
          .invert_out = false, // don't invert the output signal
      }};

  /// RMT backend specific configuration
  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,           // different clock source can lead to different power consumption
      .resolution_hz = LED_STRIP_RESOLUTION_HZ, // RMT counter clock frequency: 10MHz
      .mem_block_symbols = 64,                   // the memory size of each RMT channel, in words (4 bytes)
      .flags = {
          .with_dma = true, // DMA feature is available on chips like ESP32-S3/P4
      }};

  /// Create the LED strip object
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, led_strip));
}

led_ring_color_t led_ring_color_with_brightness(led_ring_color_t color, uint32_t brightness)
{
  led_ring_color_t newColor = {
    .r = color.r * brightness / 255,
    .g = color.g * brightness / 255,
    .b = color.b * brightness / 255
  };
  return newColor;
}

void led_ring_single_on(led_strip_handle_t led_strip, int led_index, float angle, led_ring_color_t color, uint32_t brightness)
{
  for(int i = 0; i < LED_STRIP_NUMBERS; i++)
  {
    led_strip_set_pixel(led_strip, i, 0, 0, 0);
  };

  led_ring_color_t newColor = led_ring_color_with_brightness(color, brightness);

  led_strip_set_pixel(led_strip, led_index, newColor.r, newColor.g, newColor.b);
  led_strip_refresh(led_strip);
}

void led_ring_single_breath_on(led_strip_handle_t led_strip, int led_index)
{
}

void led_ring_gradient_on(led_strip_handle_t led_strip, int led_index, float angle, led_ring_color_t color, uint32_t brightness)
{
  //led_strip_clear(led_strip);

  int left_count = (LED_STRIP_NUMBERS - 1) / 2;
  int right_count = LED_STRIP_NUMBERS - 1 - left_count;

  //ESP_LOGI(TAG, "%d, %f", led_index, angle);

  // 中间亮、两边渐暗
  for (int offset = -left_count; offset <= right_count; offset++)
  {
    int led_cur_index = (led_index + offset + LED_STRIP_NUMBERS) % LED_STRIP_NUMBERS; // 确保索引在 0~11 范围内
    double angle_diff = fabs(angle - led_cur_index * 30.0 - 15.0); // 计算角度差
    double brightness = (MAX_BRIGHTNESS - 10) * (1 - sin(angle_diff / 2.0 * (M_PI / 180.0))) + 10; // 亮度递减，保证最低亮度是30

    //double led_count = (double)LED_STRIP_NUMBERS;
    //double brightness = (MAX_BRIGHTNESS - 20.0) * (1.0 - fabs(offset)/led_count*2.0) + 20.0;

    //ESP_LOGI(TAG, "%d, %d, %f, %f", led_cur_index, offset, angle_diff, brightness);

    led_ring_color_t newColor = led_ring_color_with_brightness(color, brightness);

    led_strip_set_pixel(led_strip, led_cur_index, newColor.r, newColor.g, newColor.b);
  }

  //ESP_LOGI(TAG, "\n");

  led_strip_refresh(led_strip);
}


void led_ring_gradient_double_on(led_strip_handle_t led_strip, int led_index, float angle, led_ring_color_t color_start, led_ring_color_t color_end, uint32_t brightness){
  //led_strip_clear(led_strip);

  int left_count = (LED_STRIP_NUMBERS - 1) / 2;
  int right_count = LED_STRIP_NUMBERS - 1 - left_count;

  //ESP_LOGI(TAG, "%d, %f", led_index, angle);

  // 中间亮、两边渐暗
  for (int offset = -left_count; offset <= right_count; offset++)
  {
    int led_cur_index = (led_index + offset + LED_STRIP_NUMBERS) % LED_STRIP_NUMBERS; // 确保索引在 0~11 范围内
    double angle_diff = fabs(angle - led_cur_index * 30.0 - 15.0); // 计算角度差
    double brightness = (MAX_BRIGHTNESS - 1) * (1 - sin(angle_diff / 2.0 * (M_PI / 180.0))) + 1; // 亮度递减，保证最低亮度是30

    //double led_count = (double)LED_STRIP_NUMBERS;
    //double brightness = (MAX_BRIGHTNESS - 20.0) * (1.0 - fabs(offset)/led_count*2.0) + 20.0;

    //ESP_LOGI(TAG, "%d, %d, %f, %f", led_cur_index, offset, angle_diff, brightness);

    double ratio = (double)led_cur_index / LED_STRIP_NUMBERS;
    led_ring_color_t newColor = interpolate_rgb(color_start, color_end, ratio);
    newColor = led_ring_color_with_brightness(newColor, brightness);

    led_strip_set_pixel(led_strip, led_cur_index, newColor.r, newColor.g, newColor.b);
  }

  //ESP_LOGI(TAG, "\n");

  led_strip_refresh(led_strip);
}

void led_ring_rainbow_on(led_strip_handle_t led_strip, int cur_led, int angle)
{
  led_strip_clear(led_strip);

  // for (int led = 0; led < LED_STRIP_NUMBERS; led++) {
  //     float rgb_angle = angle * (M_PI / 180.0) + (led * 0.3);
  //     const float color_off = (M_PI * 2) / 3;
  //     uint8_t r = (sin(rgb_angle + color_off * 0) * 127 + 128) * 0.1;
  //     uint8_t g = (sin(rgb_angle + color_off * 1) * 127 + 128) * 0.1;
  //     uint8_t b = (sin(rgb_angle + color_off * 2) * 117 + 128) * 0.1;

  //     led_strip_set_pixel(led_strip, led, r, g, b);
  // }

  // 计算颜色插值
  double color_ratio = angle / 359.0; // clock_angle 范围是 0~359
  uint8_t r = 255 * color_ratio;
  uint8_t g = 0 * color_ratio;
  uint8_t b = 255 - 255 * color_ratio;

  // 更新当前 LED 及其左右 LED 的亮度和颜色
  for (int offset = -5; offset <= 6; offset++)
  {
    int led_index = (cur_led + offset + LED_STRIP_NUMBERS) % LED_STRIP_NUMBERS; // 确保索引在 0~11 范围内
    double angle_diff = fabs(angle - (cur_led + offset) * 30.0);                // 计算角度差
    if (angle_diff > 180)
      angle_diff = 360 - angle_diff;                                                  // 确保角度差不超过 180 度
    double brightness = MAX_BRIGHTNESS * (1 - BRIGHTNESS_DECAY * angle_diff / 180.0); // 亮度递减

    // 设置 LED 颜色和亮度
    r = (uint8_t)(r * brightness / MAX_BRIGHTNESS);
    g = (uint8_t)(g * brightness / MAX_BRIGHTNESS);
    b = (uint8_t)(b * brightness / MAX_BRIGHTNESS);
    led_strip_set_pixel(led_strip, led_index, r, g, b);
  }

  led_strip_refresh(led_strip);
}

void led_ring_snake_gradient(led_strip_handle_t led_strip, int led_index, int angle, const led_ring_color_t *colors, int color_length, uint32_t brightness)
{

  ESP_LOGI(TAG, "%d", color_length);
  
  for (int i = 0; i < LED_STRIP_NUMBERS; i++)
  {
    int led_cur_index = (led_index + i) % LED_STRIP_NUMBERS;

    //led_ring_color_t newColor = led_ring_color_with_brightness(color, brightness);
    //led_strip_set_pixel(led_strip, led_cur_index, newColor.r, newColor.g, newColor.b);
  }

  //led_strip_refresh(led_strip);
}

void led_ring_clear(led_strip_handle_t led_strip)
{
  led_strip_clear(led_strip);
  led_strip_refresh(led_strip);
}

void led_ring_on(const led_ring_on_config_t *config)
{
  switch (config->mode)
  {
    /*
  case LED_RING_SINGLE_LIGHT:
    led_ring_single_on(config->led_strip, config->led_index, config->angle, config->color[0], config->brightness);
    break;
  case LED_RING_SINGLE_BREATH_LIGHT:
    led_ring_single_breath_on(config->led_strip, config->led_index);
    break;
  case LED_RING_BREATH_LIGHT:
    
    break;
  case LED_RING_GRADIENT_SINGLE_COLOR:
    led_ring_gradient_on(config->led_strip, config->led_index, config->angle, config->color[0], config->brightness);
  case LED_RING_GRADIENT_DOUBLE_COLOR:
    led_ring_gradient_double_on(config->led_strip, config->led_index, config->angle, config->color[0], config->color[1], config->brightness);
  */
  case LED_RING_SNAKE_GRADIENT:
    //led_ring_snake_gradient(config->led_strip, config->led_index, config->angle, &config->color, sizeof(config->color), config->brightness);
    break;
  case LED_RING_CENTER_GRADIENT:
    //led_ring_center_gradient(config->led_strip, config->led_index, config->angle, &config->gradient_color, config->brightness);
    break;
  default:
    break;
  }
}