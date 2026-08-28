idf_component_register(
    SRCS "main.c" "temperature_channels.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        app_update
        esp_driver_tsens
        esp_hw_support
        esp_psram
        esp_timer
        nvs_flash
        spi_flash
)
