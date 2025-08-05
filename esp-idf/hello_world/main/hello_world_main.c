/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#define TAG "HELLO_WORLD"
#define RESTART_COUNTDOWN 3
#define BYTES_TO_MB(bytes) ((bytes) / (1024 * 1024))
#define DELAY_MS(ms) ((ms) / portTICK_PERIOD_MS)

typedef struct {
    esp_chip_info_t info;
    uint32_t flash_capacity;
    uint32_t heap_minimum;
} system_status_t;

static esp_err_t gather_system_info(system_status_t* status)
{
    if (!status) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_chip_info(&status->info);
    status->heap_minimum = esp_get_minimum_free_heap_size();
    
    esp_err_t flash_result = esp_flash_get_size(NULL, &status->flash_capacity);
    if (flash_result != ESP_OK) {
        ESP_LOGE(TAG, "Failed to retrieve flash size: %s", esp_err_to_name(flash_result));
        return flash_result;
    }
    
    return ESP_OK;
}

static void display_system_details(const system_status_t* status)
{
    const char* flash_type = (status->info.features & CHIP_FEATURE_EMB_FLASH) ? 
                            "embedded" : "external";
    printf("%" PRIu32 "MB %s flash\n", BYTES_TO_MB(status->flash_capacity), flash_type);

    printf("Minimum free heap size: %" PRIu32 " bytes\n", status->heap_minimum);
}

static void execute_restart_sequence(void)
{
    for (int countdown = RESTART_COUNTDOWN - 1; countdown >= 0; countdown--) {
        printf("Restarting in %d seconds...\n", countdown);
        vTaskDelay(DELAY_MS(1000));
    }
    
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

void app_main(void)
{
    printf("Hello world!\n");

    system_status_t system_status;
    
    esp_err_t result = gather_system_info(&system_status);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "System information gathering failed");
        return;
    }
    
    display_system_details(&system_status);
    execute_restart_sequence();
}
