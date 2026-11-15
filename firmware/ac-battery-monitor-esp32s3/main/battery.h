#pragma once

#include "battery_wire.h"
#include "esp_err.h"

esp_err_t battery_bus_start(void);
battery_snapshot_t battery_poll(void);
