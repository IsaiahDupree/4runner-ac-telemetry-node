idf_component_register(SRCS "main.c" "diagnostic_math.c" "display.c"
    INCLUDE_DIRS "." REQUIRES esp_driver_i2c esp_driver_gpio esp_driver_spi esp_driver_ledc esp_timer esp_system)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter)
