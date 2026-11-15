idf_component_register(
    SRCS "main.c" "battery.c" "battery_wire.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES bt esp_driver_gpio esp_driver_i2c esp_hw_support esp_timer nvs_flash
)
