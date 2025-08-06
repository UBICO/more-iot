/*
 * Simple ESP32-S3-EYE LCD Hello World
 * Based on ESP-IDF LVGL example - minimal approach
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_disp.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "lvgl.h"
#include "esp_random.h"

static const char *TAG = "simple_lcd";

// ESP32-S3-EYE LCD pins (from official ESP-WHO repository)
#define LCD_HOST        SPI2_HOST
#define LCD_H_RES       240
#define LCD_V_RES       240
#define PIN_MOSI        47
#define PIN_CLK         21
#define PIN_CS          44
#define PIN_DC          43
#define PIN_RST         -1    // Not connected
#define PIN_BK_LIGHT    48

void app_main(void)
{
    ESP_LOGI(TAG, "Starting simple ESP32-S3-EYE LCD example");
    
    // 1. Setup backlight (ESP32-S3-EYE has INVERTED/ACTIVE-LOW backlight)
    gpio_set_direction(PIN_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BK_LIGHT, 0);  // 0 = ON for ESP32-S3-EYE (active-low)
    ESP_LOGI(TAG, "Backlight enabled (active-low)");
    
    // 2. Initialize SPI bus
    spi_bus_config_t bus_config = {
        .sclk_io_num = PIN_CLK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "SPI bus initialized");
    
    // 3. Setup LCD panel IO
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_DC,
        .cs_gpio_num = PIN_CS,
        .pclk_hz = 20 * 1000 * 1000,  // 20MHz - conservative speed
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    ESP_LOGI(TAG, "LCD panel IO created");
    
    // 4. Create ST7789 panel
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_LOGI(TAG, "ST7789 panel created");
    
    // 5. Initialize the panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    ESP_LOGI(TAG, "Panel initialized and turned on");
    
    // 6. Initialize LVGL
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
    ESP_LOGI(TAG, "LVGL initialized");
    
    // 7. Attach the LCD to LVGL
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 40,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    lv_display_t *display = lvgl_port_add_disp(&disp_cfg);
    ESP_LOGI(TAG, "Display attached to LVGL");
    
    // 8. Create simple UI (working version)
    if (lvgl_port_lock(0)) {
        lv_obj_t *label = lv_label_create(lv_scr_act());
        lv_label_set_text(label, "Hello ESP32-S3-EYE!\nMORE-IOT Class");
        // lv_obj_center(label);
        lv_obj_set_pos(label, 0, 0);
        
        // Make text bigger by scaling the object
        lv_obj_set_style_transform_scale(label, 400, 0); 

        // Make the text bold using text decoration
        // lv_obj_set_style_text_decor(label, LV_TEXT_DECOR_NONE, 0);
        // lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
        
        // Create bold effect by adding shadow/outline
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_style_outline_color(label, lv_color_white(), 0);
        lv_obj_set_style_outline_width(label, 1, 0);
        // lv_obj_set_style_outline_opa(label, LV_OPA_70, 0);
        
        // White text on black background (working colors) 
        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
        
        lvgl_port_unlock();
    }
    ESP_LOGI(TAG, "UI created successfully");
    
    // 9. Simple main loop - just keep alive
    while (1) {
        ESP_LOGI(TAG, "System running, heap: %d KB", (int)(esp_get_free_heap_size() / 1024));

        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(esp_random() % 255, esp_random() % 255, esp_random() % 255), 0);
        // lv_obj_set_style_text_color(lv_scr_act(), lv_color_make(random(), random(), random()), 0);

        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}
