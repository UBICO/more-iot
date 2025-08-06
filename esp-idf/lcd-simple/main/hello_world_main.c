/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"

#include "bsp/esp32_s3_eye.h"

void app_main(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),   /* See LVGL Port for more info */
        .buffer_size = BSP_LCD_V_RES * BSP_LCD_H_RES,   /* Screen buffer size in pixels */
        .double_buffer = true,                          /* Allocate two buffers if true */
        .flags = {
            .buff_dma = true,                           /* Use DMA-capable LVGL buffer */
            .buff_spiram = false,                       /* Allocate buffer in PSRAM if true */
        }
    };
    cfg.lvgl_port_cfg.task_stack = 10000;   /* Example: change LVGL task stack size */
    /* Initialize display, touch, and LVGL */
    lv_display_t *display = bsp_display_start_with_config(&cfg);
    
    if (display == NULL) {
        printf("Error: Failed to initialize display!\n");
        return;
    }
    
    printf("Display initialized successfully\n");

    bsp_display_backlight_on();
    printf("Display backlight turned on\n");

    /* Wait until other tasks finish screen operations */
    bsp_display_lock(0);
    printf("Display locked, creating UI elements\n");
    
    /* Get the screen from the display handle */
    lv_obj_t * screen = lv_disp_get_scr_act(display);
    printf("Got screen object: %p\n", screen);
    
    /* Create a simple label - much easier for Hello World */
    lv_obj_t * label = lv_label_create(screen);
    lv_label_set_text(label, "Hello, World!");
    lv_obj_center(label);

    // set label font
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, LV_PART_MAIN);

    printf("Label created and centered\n");
    
    /* Create a simple rectangle for visual effect */
    lv_obj_t * rect = lv_obj_create(screen);
    lv_obj_set_size(rect, 0, 40);
    lv_obj_set_pos(rect, 0, 0);  /* Position below the text */
    lv_obj_set_style_bg_color(rect, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_border_color(rect, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(rect, 5, LV_PART_MAIN);
    printf("Rectangle created\n");

    /* Create a simple rectangle for visual effect */
    lv_obj_t * rect2 = lv_obj_create(screen);
    lv_obj_set_size(rect2, 0, 40);
    lv_obj_set_pos(rect2, 0, 45);  /* Position below the text */
    lv_obj_set_style_bg_color(rect2, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_set_style_border_color(rect2, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_width(rect2, 5, LV_PART_MAIN);
    printf("Rectangle created\n");
    
    /* Force screen update */
    lv_refr_now(display);
    printf("Screen refresh forced\n");

    /* Unlock after screen operations are done */
    bsp_display_unlock();
    printf("Display unlocked\n");

    /* Animation variables */
    int rect_width = 0;
    int rect2_width = 0;
    
    /* Main loop */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(20));  /* Slower animation for visibility */

        /* Animate first rectangle width */
        rect_width += 2;
        if (rect_width > 200) {
            rect_width = 0;  /* Reset width */
        }
        lv_obj_set_size(rect, rect_width, 40);  /* Use proper LVGL API */

        /* Animate second rectangle width */
        rect2_width += 1;
        if (rect2_width > 200) {
            rect2_width = 0;  /* Reset width */
        }
        lv_obj_set_size(rect2, rect2_width, 40);  /* Use proper LVGL API */
        
        /* Force refresh after changes */
        lv_refr_now(display);
    }
}
