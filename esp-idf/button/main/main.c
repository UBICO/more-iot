/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "bsp/esp32_s3_eye.h"
#include "iot_button.h"

static const char* TAG = "BUTTON_EXAMPLE";

/* Called on button press */
static void btn_handler(void *button_handle, void *usr_data)
{
    int button_index = (int)usr_data;
    ESP_LOGI(TAG, "Button %d pressed", button_index);
    
    // Toggle LED on button press
    static bool led_state = false;
    led_state = !led_state;
    bsp_led_set(BSP_LED_GREEN, led_state);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32-S3-EYE button example...");
    
    // Initialize LEDs
    bsp_leds_init();
    
    /* Initialize all available buttons */
    button_handle_t btns[BSP_BUTTON_NUM] = {NULL};
    bsp_iot_button_create(btns, NULL, BSP_BUTTON_NUM);

    /* Register a callback for button press */
    for (int i = 0; i < BSP_BUTTON_NUM; i++) {
        iot_button_register_cb(btns[i], BUTTON_PRESS_DOWN, NULL, btn_handler, (void *) i);
    }

    ESP_LOGI(TAG, "System initialized. Press buttons to test!");

    // Main loop - just monitor the system
    while(1) {
        printf("System running... %d buttons available\n", BSP_BUTTON_NUM);
        vTaskDelay(pdMS_TO_TICKS(10000));  // Log status every 10 seconds
    }
}
