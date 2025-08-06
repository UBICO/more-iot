idf_component_register(SRCS "hello_world_main.c"
                       PRIV_REQUIRES spi_flash esp_lvgl_port esp_lcd esp_timer lvgl esp_driver_spi esp_driver_gpio esp32_s3_eye
                       INCLUDE_DIRS "")
