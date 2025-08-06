#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_camera.h"

#include "bsp/esp32_s3_eye.h"



static const char *TAG = "ESP32-camera";

static inline void swap_rgb565_bytes(uint16_t *buf, size_t pixel_count) {
    for (size_t i = 0; i < pixel_count; i++) {
        buf[i] = (buf[i] >> 8) | (buf[i] << 8);  // Swap high and low bytes
    }
}

void app_main(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),   /* See LVGL Port for more info */
        .buffer_size = BSP_LCD_V_RES * BSP_LCD_H_RES,   /* Screen buffer size in pixels */
        .double_buffer = true,                          /* Allocate two buffers if true */
        .flags = {
            .buff_dma = true,                           /* Use DMA-capable LVGL buffer */
            .buff_spiram = true,                       /* Allocate buffer in PSRAM if true */
        }
    };
    cfg.lvgl_port_cfg.task_stack = 10000;   /* Example: change LVGL task stack size */
    /* Initialize display, touch, and LVGL */
    lv_display_t *display = bsp_display_start_with_config(&cfg);
    
    bsp_i2c_init();

    // Initialize the camera with RGB565 format for direct display
    camera_config_t camera_config = BSP_CAMERA_DEFAULT_CONFIG;
    camera_config.pixel_format = PIXFORMAT_RGB565;  // Force RGB565 format
    camera_config.frame_size = FRAMESIZE_240X240;   // Match display resolution
    camera_config.jpeg_quality = 50;
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return;
    }
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, BSP_CAMERA_VFLIP);
    s->set_hmirror(s, BSP_CAMERA_HMIRROR);
    ESP_LOGI(TAG, "Camera Init done");

    uint32_t cam_buff_size = BSP_LCD_H_RES * BSP_LCD_V_RES * 2;
    uint8_t *cam_buff = heap_caps_malloc(cam_buff_size, MALLOC_CAP_SPIRAM);
    assert(cam_buff);

    // Create LVGL canvas for camera image
    bsp_display_lock(0);
    lv_obj_t *camera_canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(camera_canvas, cam_buff, BSP_LCD_H_RES, BSP_LCD_V_RES, LV_COLOR_FORMAT_RGB565);
    assert(camera_canvas);
    lv_obj_center(camera_canvas);
    bsp_display_unlock();

    bsp_display_backlight_on();


    camera_fb_t *pic;
    while (1) {
        ESP_LOGI(TAG, "Taking picture...");
        pic = esp_camera_fb_get();
        if (pic) {
            ESP_LOGI(TAG, "Image size: %d bytes, format: %d, width: %d, height: %d", 
                     pic->len, pic->format, pic->width, pic->height);
            
            bsp_display_lock(0);
            
            // Copy camera data, but don't exceed buffer size
            uint32_t copy_size = (pic->len < cam_buff_size) ? pic->len : cam_buff_size;
            memcpy(cam_buff, pic->buf, copy_size);
            
            // Handle RGB565 byte order - ESP32-S3-EYE might need byte swapping
            if (BSP_LCD_BIGENDIAN) {
                // Swap bytes in RGB565 for correct color display
                uint16_t *rgb565_buf = (uint16_t *)cam_buff;
                uint32_t pixel_count = copy_size / 2;
                swap_rgb565_bytes(rgb565_buf, pixel_count);
            }
            
            esp_camera_fb_return(pic);
            
            // Update canvas and invalidate to refresh display
            lv_canvas_set_buffer(camera_canvas, cam_buff, BSP_LCD_H_RES, BSP_LCD_V_RES, LV_COLOR_FORMAT_RGB565);
            lv_obj_invalidate(camera_canvas);
            
            bsp_display_unlock();
            printf("Camera buffer copied and displayed\n");
        } else {
            ESP_LOGE(TAG, "Get frame failed");
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);  // Take picture every second
    }
}