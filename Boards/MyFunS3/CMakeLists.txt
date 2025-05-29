idf_component_register(
    SRC_DIRS "Source" "Source/hal" "Source/SDMMCDevice" "Source/LEDRing"
    INCLUDE_DIRS "Source"
    REQUIRES Tactility esp_lvgl_port esp_lcd_gc9a01 led_strip MAX17048 CST816S GC9A01 TCA6408A QMI8658A PwmBacklight button driver
)
